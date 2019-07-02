/**
 * @file iogen.c
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * i/o workload generator
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "iogen.h"
#include "iover.h"
#include "bit.h"
#include "hash.h"
#include "timespec_sub.h"
#include "log.h"
#include "por.h"

#define USEC_PER_SEC     (1000000ULL)
//#define PAGE_ALIGNED_BUFFER

extern int iotrace_enabled;
u32 iosize_max;

#if (SUPPORT_SPOR_LEVEL_3 == 1)
void commit_block_state_unwritten(struct iover *iover, struct bio *bio);
#endif

static int iogen_cancel(struct iogen* iogen);

int iogen_job_reinit(struct iogen_job *job)
{
    int bio_idx;
    struct bio *bio;
    int res;

    res = job->engine.ops->reinit(&job->engine);
    if (res) {
        return -1;
    }

    INIT_LIST_HEAD(&job->list);
    INIT_LIST_HEAD(&job->bio_free);
    INIT_LIST_HEAD(&job->bio_inflight);

    for (bio_idx = 0; bio_idx < job->params->iodepth; bio_idx++) {
        bio = &job->bio_pool[bio_idx];
        INIT_LIST_HEAD(&bio->list);

        
        // memset(bio->data, 0, job->params->iosize_max);
        //memset(bio->block_desc, 0, job->lbas.max_lbas * sizeof (struct block_desc));
        memset(bio->data, 0, iosize_max);
        memset(bio->block_desc, 0, iosize_max >> job->ns_info->lba_shift * sizeof(struct block_desc));

        list_add_tail(&bio->list, &job->bio_free);
    }
    job->num_inflight = 0;
    if (job->params->slba_type == slba_type_seq) {
        job->slba_next[0] = job->params->slba.fixed.lba;
        job->slba_next[1] = job->params->slba.fixed.lba;
    }

    return 0;
}
void destroy_bio_pool(struct iogen_job *job)
{
    int bio_idx;
    struct bio *bio;

    if (job->bio_pool) {
        for (bio_idx = 0; bio_idx < job->params->iodepth; bio_idx++) {
            bio = &job->bio_pool[bio_idx];
            if (bio->data_buf) {
                if (job->engine.ops->dma_free) {
                    job->engine.ops->dma_free(&job->engine, bio->data_buf);
                } else {
                    free(bio->data_buf);
                }
                bio->data_buf = NULL;
            }
            if (bio->block_desc) {
                free(bio->block_desc);
                bio->block_desc = NULL;
            }
        }
        free(job->bio_pool);
        job->bio_pool = NULL;
    }
}

int create_bio_pool(struct iogen_job *job)
{
    int bio_idx;
#ifdef PAGE_ALIGNED_BUFFER
    int res;
    int pagesize;
#endif
    u32 data_buf_size;
    u32 meta_buf_size;
    struct bio *bio;

    job->bio_pool = (struct bio *)calloc(job->params->iodepth, sizeof (struct bio));
    if (job->bio_pool == NULL) {
        return -1;
    }

    data_buf_size = MAX(iosize_max, 4096); /* identify/firmware image download */
#ifdef PAGE_ALIGNED_BUFFER
    pagesize = getpagesize();
#endif
    meta_buf_size = job->params->ns_info->meta_size * (iosize_max >> job->params->ns_info->lba_shift);
    if (job->params->ns_info->extended_lba) {
        data_buf_size += meta_buf_size;
        meta_buf_size = 0;
    }
    for (bio_idx = 0; bio_idx < job->params->iodepth; bio_idx++) {
        bio = &job->bio_pool[bio_idx];
        INIT_LIST_HEAD(&bio->list);
        bio->job = job;
        bio->id = bio_idx;
        bio->ctx = job;

        if (job->engine.ops->dma_alloc) {
            bio->data_buf = NULL;
#ifdef PAGE_ALIGNED_BUFFER
            job->engine.ops->dma_alloc(&job->engine, data_buf_size, pagesize, &bio->data_buf);
	    if (bio->data_buf == NULL) {
		    destroy_bio_pool(job);
		    return -1;
	    }
            bio->data = bio->data_buf;
#else
            job->engine.ops->dma_alloc(&job->engine, data_buf_size + 16, 16, &bio->data_buf);
	    if (bio->data_buf == NULL) {
		    destroy_bio_pool(job);
		    return -1;
	    }
            bio->data = bio->data_buf + ((rand48(&job->rng) & 0x3) << 2);
#endif
        } else {
#ifdef PAGE_ALIGNED_BUFFER
            res = posix_memalign(&bio->data_buf, pagesize, data_buf_size);
            if (res) {
                destroy_bio_pool(job);
                return -1;
            }
            bio->data = bio->data_buf;
#else
            bio->data_buf = malloc(data_buf_size + 16);     /* allocate extra space to adjust offset of the buffer */
            if (bio->data_buf == NULL) {
                destroy_bio_pool(job);
                return -1;
            }
            bio->data = bio->data_buf + ((rand48(&job->rng) & 0x3) << 2);
#endif
        }
        memset(bio->data, 0, data_buf_size);

        if (meta_buf_size > 0) {
            if (job->engine.ops->dma_alloc) {
                bio->meta = NULL;
                job->engine.ops->dma_alloc(&job->engine, meta_buf_size, 0, &bio->meta);
            } else {
                bio->meta = malloc(meta_buf_size);
            }
            if (bio->meta == NULL) {
                destroy_bio_pool(job);
                return -1;
            }
            memset(bio->meta, 0, meta_buf_size);
        } else {
            bio->meta = NULL;
        }

        bio->block_desc = (struct block_desc *)calloc(iosize_max >> job->ns_info->lba_shift, sizeof (struct block_desc));
        if (bio->block_desc == NULL) {
            destroy_bio_pool(job);
            return -1;
        }
        list_add_tail(&bio->list, &job->bio_free);
    }

    return 0;
}

static void create_opr_lottery(struct iogen_job *job)
{
    u32 op_idx;
    u32 lot_idx;
    u32 weight;

    weight = 0;
    lot_idx = 0;
    for (op_idx = 0; op_idx < bio_max; op_idx++) {
        weight += job->params->operations[op_idx];
        while (lot_idx < weight) {
            job->opr_lottery[lot_idx++] = op_idx;
        }
    }
}

int iogen_job_init(struct iogen_job *job, u32 job_id, struct job_params *params)
{
    int res;
    int i;

    job->id = job_id;
    job->state = iogen_job_state_init;
    job->params = params;

    job->ns_info = params->ns_info;
    job->engine.ops = ioengine_find(job->params->ioengine);
    if (job->engine.ops == NULL) {
        fprintf(stderr, "unknown i/o engine '%s'\n", job->params->ioengine);
        return -1;
    }
    job->engine.flush_passthru = job->params->flush_passthru;
    res = job->engine.ops->init(&job->engine, job->params->controller, job->params->nsid, job->params->iodepth, job->params->ioengine_param);
    if (res) {
        fprintf(stderr, "failed to initialize ioengine(%s, %d)\n", strerror(res), res);
        job->engine.ops = NULL;
        return res;
    }

    if (strlen(job->params->name) > 15) {
        strncpy(job->display_name, job->params->name, 12);
        strcat(job->display_name, "...");
    } else {
        strcpy(job->display_name, job->params->name);
    }
    rng_init(&job->rng, job->params->seed + job_id);
    if (job->params->slba_type == slba_type_zipf) {
        rng_zipf_set(&job->rng, job->ns_info->nlbs, 0.1);
    }

    create_opr_lottery(job);

    INIT_LIST_HEAD(&job->list);
    INIT_LIST_HEAD(&job->bio_free);
    INIT_LIST_HEAD(&job->bio_inflight);

	
    for (i = 0; i < iosize_num_max; i++) {
        if (job->params->iosizes[i].iosize_min == 0 && job->params->iosizes[i].iosize_max == 0) {
            job->lbas[i].min_lbas = job->lbas[iosize_total].min_lbas;
            job->lbas[i].max_lbas = job->lbas[iosize_total].max_lbas; 
            job->lbas[i].lbas_dist = job->lbas[iosize_total].lbas_dist;
        } else {
            job->lbas[i].min_lbas = job->params->iosizes[i].iosize_min >> job->ns_info->lba_shift;
            job->lbas[i].max_lbas = job->params->iosizes[i].iosize_max >> job->ns_info->lba_shift;
            job->lbas[i].lbas_dist = job->lbas[i].max_lbas - job->lbas[i].min_lbas;
        }
		if (job->lbas[i].min_lbas == 0 || job->lbas[i].max_lbas == 0) {
			fprintf(stderr, "invalid iosize, min iosize or max iosize is zero\n");
			return -1;
		}
    }
    job->slba_align = job->params->slba_align >> job->ns_info->lba_shift;
    job->lba_range_begin = job->params->lba_range_begin >> job->ns_info->lba_shift;
    job->lba_range_size = job->params->lba_range_size >> job->ns_info->lba_shift;
    job->acct.ops[bio_read].latency.min = U64_MAX;
    job->acct.ops[bio_write].latency.min = U64_MAX;
    job->reset_dist = job->params->reset_max - job->params->reset_min;

    res = create_bio_pool(job);
    if (res) {
        fprintf(stderr, "not enough memory\n");
        return res;
    }

    if (job->engine.ops->dma_alloc) {
        job->engine.ops->dma_alloc(&job->engine, 4096, getpagesize(), &job->identify);
    } else {
        posix_memalign(&job->identify, getpagesize(), 4096);
    }
    if (job->identify == NULL) {
        fprintf(stderr, "not enough memory\n");
        return res;
    }

    pthread_mutex_init(&job->mutex, NULL);
    pthread_cond_init(&job->cond, NULL);

    return res;
}

int iogen_init(struct iogen* iogen, struct global_params *params)
{
    int res;
    int job_idx;
    struct iogen_job *job;

    iogen->params = params;
    iogen->running_job_mask = 0;
    iogen->finished_job_mask = 0;
    iogen->njobs = params->njobs;
    iogen->jobs = calloc(iogen->njobs, sizeof (struct iogen_job));
    if (iogen->jobs == NULL) {
        fprintf(stderr, "not enough memory\n");
        res = -1;
        goto error;
    }

    for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
        job = &iogen->jobs[job_idx];
        job->iogen = iogen;
        job->priv = (void *)iogen;
        res = iogen_job_init(job, job_idx + 1, &params->job_params[job_idx]);
        if (res) {
            goto error;
        }
    }

    iogen->result = 0;
    iogen->state = iogen_state_init;
    return 0;

error:
    if (iogen->jobs) {
        for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
            job = &iogen->jobs[job_idx];
            destroy_bio_pool(job);
            if (job->engine.ops) {
                job->engine.ops->exit(&job->engine);
            }
        }

        free(iogen->jobs);
        iogen->jobs = NULL;
    }
    return res;
}

void iogen_exit(struct iogen* iogen)
{
    u32 job_idx;
    struct iogen_job *job;

    if (iogen->jobs) {
        for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
            job = &iogen->jobs[job_idx];
            destroy_bio_pool(job);
            if (job->identify) {
                if (job->engine.ops->dma_free) {
                    job->engine.ops->dma_free(&job->engine, job->identify);
                } else {
                    free(job->identify);
                }
            }
            if (job->engine.ops) {
                job->engine.ops->exit(&job->engine);
            }
            pthread_mutex_destroy(&job->mutex);
            pthread_cond_destroy(&job->cond);
        }

        free(iogen->jobs);
        iogen->jobs = NULL;
    }
}

static void latency_acct_add(struct latency_acct_bucket *buckets, u64 lat_ns)
{
    u32 idx;

    if (lat_ns < 9000) {
        idx = lat_ns / 1000;
    } else if (lat_ns < 90000) {
        idx = 9 + lat_ns / 10000;
    } else if (lat_ns < 900000) {
        idx = 18 + lat_ns / 100000;
    } else if (lat_ns < 9000000) {
        idx = 27 + lat_ns / 1000000;
    } else if (lat_ns < 90000000) {
        idx = 36 + lat_ns / 10000000;
    } else if (lat_ns < 900000000) {
        idx = 45 + lat_ns / 100000000;
    } else {
        idx = 54;
    }
    buckets[idx].count++;
    buckets[idx].sum += lat_ns;
}

static void bio_end(struct bio *bio)
{
    struct iogen_job *job;
    struct opacct *opacct;
    u64 lat_ns;
    u32 offset;
    int res;

    clock_gettime(CLOCK_MONOTONIC, &bio->end_ts);

    job = bio->job;
    if (bio->opcode <= bio_identify) {
        lat_ns = timespec_sub(&bio->end_ts, &bio->begin_ts);
        opacct = &job->acct.ops[bio->opcode];
        if (opacct->latency.min > lat_ns) {
            opacct->latency.min = lat_ns;
        }
        if (opacct->latency.max < lat_ns) {
            opacct->latency.max = lat_ns;
        }
        opacct->ops++;
        opacct->bytes += bio->iosize;
        opacct->latency.sum += lat_ns;
        latency_acct_add(opacct->latency.buckets, lat_ns);

        job->acct.sum.latency.sum += lat_ns;

        if (bio->opcode == bio_read || bio->opcode == bio_identify) {
            if (job->params->inject_miscompare) {
                if (rng_uniform_real(&job->rng) < job->params->inject_miscompare) {
                    offset = rng_uniform_u48(&job->rng, job->ns_info->data_size * bio->nlbs);
                    ((u8 *)bio->data)[offset] ^= 0x77;
                }
            }
        }

        res = iover_submit(job->ns_info->iover, bio);
        if (res) {
            job->iogen->result = res;
            iogen_cancel(job->iogen);
#if (SUPPORT_SPOR_DBG_MSG == 1)
            printf("%s: iover_submit error detected\n", job->params->name);
#endif
            exit(1);
        }
    }

    list_del(&bio->list);
    list_add(&bio->list, &job->bio_free);
    job->num_inflight--;
}

static void history_slba(struct iogen_job *job, struct bio *bio)
{
    struct journal_header *jh = job->ns_info->iover->journal.header;
    struct hist_desc *hd = job->ns_info->iover->hist_desc + job->replay_hist_idx;
    
    if (hd->lba == 0xFFFFFFFFFFFFFFFF || job->replay_hist_cnt == 0) {
        job->iogen->state = iogen_state_done;
        bio->opcode = bio_flush;

        // clear_hist(job->ns_info->iover);
        return;
    }

    bio->opcode = bio_read;
    bio->nlbs = 1;
    bio->slba = hd->lba;
    bio->hist_idx = job->replay_hist_idx;

    // printf("%-16s: [%4d] lba=%16llu curr state %d next state %d\n",
    //     __func__, job->replay_hist_idx, hd->lba, hd->bd[BD_CURR].state, hd->bd[BD_NEXT].state);

    if (job->replay_hist_idx == 0)
        job->replay_hist_idx = jh->nhist - 1;
    else
        job->replay_hist_idx--;

    job->replay_hist_cnt--;
    if (job->replay_hist_cnt == 0)
        job->iogen->state = iogen_job_state_done;
}

static struct bio *generate_bio(struct iogen_job *job)
{
    u32 lba_shift;
    int res = 0;
    int try = 0;
    u64 rand;
    struct bio *bio;

    bio = list_first_entry(&job->bio_free, struct bio, list);
    bio->hist_idx = 0xFFFFFFFF;

    rand = rand48(&job->rng);
    bio->opcode = job->opr_lottery[rand % 100];
    if (bio->opcode == bio_flush) {
        bio->nlbs = 0;
        bio->iosize = 0;
        goto exit;
    }
    rand >>= 7;
    lba_shift = job->ns_info->lba_shift;
	if (bio->opcode == bio_write_uncorrect && lba_shift == 9) {
		bio->opcode = bio_write;
	}

    if (job->params->slba_type == slba_type_jesd219) {
        jesd219_gen(job);

        bio->nlbs = jesd219_nlbs(&job->jesd219);
    }
    else {
        if (job->lbas[bio->opcode].lbas_dist > 0) {
            bio->nlbs = job->lbas[bio->opcode].min_lbas + (rand % (job->lbas[bio->opcode].lbas_dist + 1));
        } else {
            bio->nlbs = job->lbas[bio->opcode].min_lbas;
        }
		if (job->params->iosizes[bio->opcode].iosize_align != 0 ) {
			bio->nlbs = ALIGN(bio->nlbs, job->params->iosizes[bio->opcode].iosize_align >> lba_shift);
		}
    }
    bio->iosize = bio->nlbs << job->ns_info->lba_shift;
    bio->pract = 0;
    bio->prchk = 0;
    if ((job->ns_info->pi_type != 0) && (job->ns_info->meta_size != 0)) {
        /* protection information enabled */
        if (rng_uniform_real(&job->rng) < job->params->pract) {
            bio->pract = 1;
        }
        bio->prchk = job->params->prchk;
    }

    if (!((bio->pract) && (job->ns_info->meta_size == 8))) {
        if (job->params->ns_info->extended_lba) {
            bio->iosize += bio->nlbs * job->ns_info->meta_size;
            bio->meta_size = 0;
        } else {
            bio->meta_size = bio->nlbs * job->ns_info->meta_size;
        }
    } else {
        bio->meta_size = 0;
    }

    do {
        try++;
        if (try >= 4) {
            return NULL;
        }
        switch (job->params->slba_type) {
            case slba_type_seq:
                if ((job->params->separate_stream) && (bio->opcode == bio_read)) {
                    bio->slba = job->slba_next[1];
                    if (bio->slba + bio->nlbs > job->lba_range_size) {
                        bio->slba = 0;
                    }
                    if (bio->slba + bio->nlbs >= job->slba_next[0]) {
                        return NULL;
                    }
                    job->slba_next[1] = bio->slba + bio->nlbs;
                } else {
                    bio->slba = job->slba_next[0];
                    if (bio->slba + bio->nlbs > job->lba_range_size) {
                        bio->slba = 0;
                    }
                    job->slba_next[0] = bio->slba + bio->nlbs;
                }
                break;
            case slba_type_uniform:
                bio->slba = rng_uniform_u48(&job->rng, job->lba_range_size - bio->nlbs);
                break;
            case slba_type_zipf:
                bio->slba = hash64(rng_zipf_int(&job->rng)) % (job->lba_range_size - bio->nlbs);
                break;
            case slba_type_pareto:
                break;
            case slba_type_fixed:
                bio->slba = job->params->slba.fixed.lba;
                break;
            case slba_type_jesd219:
                bio->slba = jesd219_slba(&job->jesd219);
                break;
            case slba_type_replay:
                history_slba(job, bio);
                if (bio->opcode == bio_flush) {
                    bio->nlbs = 0;
                    bio->iosize = 0;
                    goto exit;
                }
                break;
            default:
                fprintf(stderr, "invalid slba type\n");
                exit(-1);
                break;
        }
        if (job->params->slba_align > job->ns_info->data_size) {
            bio->slba = ALIGN(bio->slba, job->slba_align);
        }
        bio->slba += job->lba_range_begin;
        res = iover_generate_data(job->ns_info->iover, bio);
    } while (res);
    bio->offset = bio->slba << lba_shift;
    job->acct.sum.bytes += bio->iosize;
    bio->data_size = bio->iosize;

    if (bio->opcode == bio_write) {
        u32 num_streams = job->params->num_streams;
        if (num_streams == 0) {
            bio->stream_id = 0;
        } else {
            rand >>= 10;
            bio->stream_id = 1 + rand%num_streams;
        }
    }

exit:
    job->acct.sum.ops++;
    bio->end = bio_end;
    bio->timeout = false;

    bio->expected_result = 0;
    if ((bio->opcode < bio_identify) && (job->params->ns_info->pi_type) && (job->params->inject_tag_error)) {
        if (rng_uniform_real(&job->rng) < job->params->inject_tag_error) {
            if ((bio->pract == 0) || (bio->opcode == bio_read)) {
                bio->apptag ^= 0x5555;
                //bio->reftag ^= 0xAAAAAAAA;
                bio->expected_result = -1;
            }
        }
    }

    list_del(&bio->list);
    return bio;
}

static u32 iogen_job_eta(struct iogen_job *job)
{
    struct timespec now_ts;
    u32 elapsed_seconds;
    u64 iops;
    u64 throughput;

    if (job->state == iogen_job_state_running) {
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        elapsed_seconds = timespec_sub(&now_ts, &job->begin_ts) / NSEC_PER_SEC;

        switch (job->params->amount_type) {
            case amount_type_time:
                if (elapsed_seconds > job->params->amount.seconds) {
                    return U32_MAX;
                }
                return job->params->amount.seconds - elapsed_seconds;
            case amount_type_io:
                if (elapsed_seconds > 0) {
                    iops = job->acct.sum.ops / elapsed_seconds;
                    if (iops > 0) {
                        return (u32)((job->params->amount.ios - job->acct.sum.ops) / iops);
                    }
                }
                return U32_MAX;
            case amount_type_byte:
                if (elapsed_seconds > 0) {
                    throughput = job->acct.sum.bytes / elapsed_seconds;
                    if (throughput > 0) {
                        return (u32)((job->params->amount.bytes - job->acct.sum.bytes) / throughput);
                    }
                }
                return U32_MAX;
            case amount_type_infinite:
                return U32_MAX;
        }
    }
    return U32_MAX;
}

static int iogen_job_should_stop(struct iogen_job *job)
{
    if (job->iogen->state != iogen_state_running) {
        job->state = iogen_job_state_aborting;
        return 1;
    }

    switch (job->params->amount_type) {
        case amount_type_time:
            if (job->timeout) {
                return 1;
            }
            break;
        case amount_type_io:
            if (job->acct.sum.ops >= job->params->amount.ios) {
                return 1;
            }
            break;
        case amount_type_byte:
            if (job->acct.sum.bytes >= job->params->amount.bytes) {
                return 1;
            }
            break;
        case amount_type_infinite:
            break;
    }

    return 0;
}

static void iogen_job_wait(struct iogen_job* job)
{
    struct iogen_job *depend_job;
    job->state = iogen_job_state_waiting;
    depend_job = &job->iogen->jobs[job->params->dependency_job_idx];

    pthread_mutex_lock(&depend_job->mutex);
    pthread_cond_wait(&depend_job->cond, &depend_job->mutex);
    pthread_mutex_unlock(&depend_job->mutex);
}

static void *iogen_job_timer(void *arg)
{
    struct iogen_job_timer *timer = (struct iogen_job_timer *)arg;

    pthread_mutex_lock(&timer->mutex);
    pthread_cond_timedwait(&timer->cond, &timer->mutex, &timer->ts);
    pthread_mutex_unlock(&timer->mutex);

    timer->job->timeout = 1;

    return (void *)0;
}

static int iogen_job_setup_timer(struct iogen_job *job)
{
    int res;
    struct iogen_job_timer *timer = &job->timer;

    timer->job = job;
    pthread_mutex_init(&timer->mutex, NULL);
    pthread_cond_init(&timer->cond, NULL);

    clock_gettime(CLOCK_REALTIME, &timer->ts);
    timer->ts.tv_sec += job->params->amount.seconds;

    res = pthread_create(&timer->thread, NULL, iogen_job_timer, timer);
    if (res != 0) {
        fprintf(stderr, "pthread_create() failed with %d\n", res);
    }
    return res;
}

static int iogen_job_begin(struct iogen_job* job)
{
    int res;

    __sync_or_and_fetch(&job->iogen->running_job_mask, 1ULL << (job->id - 1));

    if (job->engine.ops->begin) {
        res = job->engine.ops->begin(&job->engine);
        if (res) {
            job->state = iogen_job_state_aborting;
            job->iogen->result = res;
            return res;
        }
    }

    if (job->params->amount_type == amount_type_time) {
        iogen_job_setup_timer(job);
    }

    clock_gettime(CLOCK_MONOTONIC, &job->begin_ts);
    job->state = iogen_job_state_running;

    if (job->ns_info->nhist > 0) {
        job->replay_hist_idx = job->ns_info->iover->journal.header->hist_idx;
        job->replay_hist_cnt = job->ns_info->iover->journal.header->nhist;
    }
    
    return 0;
}

static void iogen_job_end(struct iogen_job* job)
{
#if (SUPPORT_SPOR_LEVEL_3 == 1)
    struct iogen *iogen = NULL;
#endif

    if (job->engine.ops->end) {
#if (SUPPORT_SPOR_LEVEL_3 == 1)
        iogen = (struct iogen *)job->priv;

        if (iogen->params->spor_level < SPOR_LEVEL_3) {
            job->engine.ops->end(&job->engine);
        } else {
#if (SUPPORT_SPOR_LEVEL_3 == 1)
            printf("%s - skipped calling engine.ops->end() due to SPOR Level 3 Test\n", __func__);
#endif
        }
#else
        job->engine.ops->end(&job->engine);
#endif
    }

    pthread_mutex_lock(&job->mutex);
    pthread_cond_broadcast(&job->cond);
    pthread_mutex_unlock(&job->mutex);

    clock_gettime(CLOCK_MONOTONIC, &job->end_ts);
    if (job->state == iogen_job_state_aborting) {
        job->state = iogen_job_state_aborted;
    } else {
        job->state = iogen_job_state_done;
    }
    __sync_or_and_fetch(&job->iogen->finished_job_mask, 1ULL << (job->id - 1));
    __sync_and_and_fetch(&job->iogen->running_job_mask, ~(1ULL << (job->id - 1)));
    if (popcount(job->iogen->finished_job_mask) == job->iogen->njobs) {
        clock_gettime(CLOCK_MONOTONIC, &job->iogen->end_ts);
        job->iogen->state = iogen_state_done;
    }
}

static void *iogen_job_runner(void *arg)
{
    struct iogen_job *job;
    struct bio *bio;
    u32 nr_enqueued = 0;
    u32 batch_submit;
    u32 iodepth;
    u64 rand;
    int res;
    struct timespec now_ts;
    struct iogen *iogen;

    job = (struct iogen_job *)arg;
    iodepth = job->params->iodepth;
    if (job->params->batch_submit) {
        batch_submit = job->params->batch_submit;
    } else {
        batch_submit = iodepth;
    }

    if (job->params->dependency_job_idx != job->id - 1) {
        iogen_job_wait(job);
    }

    if (job->params->delay) {
        sleep(job->params->delay);
    }

    if ((job->params->reset_min != 0) || (job->params->reset_max != 0)) {
        rand = rand48(&job->rng);
        job->reset_next = time(NULL) + job->params->reset_min + (rand % (job->reset_dist + 1));
        printf("%s: first function level reset will occur at S%08u\n", job->params->name, job->reset_next);
    }
    res = iogen_job_begin(job);
    if (res) {
        iogen_job_end(job);
        return (void *)0;
    }

    while (!iogen_job_should_stop(job)) {
        if (job->params->timeout && job->num_inflight) {
            clock_gettime(CLOCK_MONOTONIC, &now_ts);
            list_for_each_entry(bio, &job->bio_inflight, list) {
                if ((bio->timeout == false) && (timespec_sub(&now_ts, &bio->begin_ts) / NSEC_PER_SEC >= job->params->timeout)) {
                    bio->timeout = true;
                    printf("%s: timeout(opcode %u, slba %llu, nlbs %u, data 0x%p, meta 0x%p)\n",
                            job->params->name, bio->opcode, bio->slba, bio->nlbs, bio->data, bio->meta);
                    if (job->params->stop_on_timeout) {
                        exit(1);
                    }
                }
            }
        }
        if ((job->reset_next > 0) && (job->reset_next < time(NULL))) {
            job->state = iogen_job_state_reset;
            printf("%s: initiate reset(%u times)\n", job->params->name, ++job->nr_reset);
            job->engine.ops->reset(&job->engine);
            res = iogen_job_reinit(job);
            if (res) {
                exit(0);
            }

            job->state = iogen_job_state_running;
            nr_enqueued = 0;
            rand = rand48(&job->rng);
            job->reset_next = time(NULL) + job->params->reset_min + (rand % (job->reset_dist + 1));
            printf("%s: next reset will occur at S%08u\n", job->params->name, job->reset_next);
        }
        if (job->num_inflight < iodepth) {
            bio = generate_bio(job);
            if (bio == NULL) {
                /* try again later */
                if (nr_enqueued) {
                    res = job->engine.ops->submit(&job->engine);
                    if (res) {
                        fprintf(stderr, "iogen: i/o submit failed(%d, nr_enqueued %u)\n", res, nr_enqueued);
                        job->iogen->result = res;
                        break;
                    }
                }
                job->engine.ops->poll(&job->engine, 0, job->params->batch_complete);
                nr_enqueued = 0;
                continue;
            }
            clock_gettime(CLOCK_MONOTONIC, &bio->begin_ts);
            res = job->engine.ops->enqueue(&job->engine, bio);      // spdkio.c: spdkio_enqueue() is called.
            if (res) {
                fprintf(stderr, "iogen: i/o enqueue failed(%d)\n", res);
                job->iogen->result = res;
                break;
            }
            /* libaio quirk */
            if ((strcmp(job->engine.ops->name, "libaio") == 0) && (bio->opcode == bio_flush)) {
                job->num_inflight++;
                list_add(&bio->list, &job->bio_inflight);
                bio->end(bio);
                continue;
            }
            nr_enqueued++;
            job->num_inflight++;
            list_add(&bio->list, &job->bio_inflight);
            backup_hist_state(job->ns_info->iover, bio);
            if ((iotrace_enabled == 1) && (bio->opcode <= bio_write)) {
                iotrace_append(bio);
            }

#if (SUPPORT_SPOR_LEVEL_3 == 1)
            // Set block state to unwritten temporarily until the write operation is complete
            iogen = (struct iogen *)job->priv;
            if ((iogen->params->spor_level >= SPOR_LEVEL_3) && (bio->opcode == bio_write)) {
                commit_block_state_unwritten(job->ns_info->iover, bio);
            }
#endif

            if ((nr_enqueued == batch_submit) || (job->num_inflight == iodepth)) {
                res = job->engine.ops->submit(&job->engine);
                if (res) {
                    printf("iogen: i/o submit failed(%d, nr_enqueued %u, num_inflight %u)\n", res, nr_enqueued, job->num_inflight);
                    job->iogen->result = res;
                    break;
                }
                nr_enqueued = 0;
            }
        } else {
            job->engine.ops->poll(&job->engine, 0, job->params->batch_complete);
        }
    }

    job->engine.ops->submit(&job->engine);
    while (job->num_inflight > 0) {
#if (SUPPORT_SPOR_LEVEL_3 == 1)
        if (iogen->params->spor_level < SPOR_LEVEL_3) {
            job->engine.ops->poll(&job->engine, 1, job->params->batch_complete);
        } else {
            printf("%s - skipped polling for not-yet-completed commands due for SPOR Level 3 Test\n", __func__);
            break;
        }
#else
        job->engine.ops->poll(&job->engine, 1, job->params->batch_complete);
#endif
    }
    iogen_job_end(job);

    return (void *)0;
}

int iogen_start(struct iogen* iogen)
{
    struct iogen_job *job;
    int job_idx;
    int res = 0;

    iogen->state = iogen_state_running;

    clock_gettime(CLOCK_MONOTONIC, &iogen->begin_ts);
    for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
        job = &iogen->jobs[job_idx];
        res = pthread_create(&job->thread, NULL, iogen_job_runner, job);
        if (res != 0) {
            fprintf(stderr, "pthread_create() failed with %d\n", res);
            break;
        }
    }

    return res;
}

int iogen_stop(struct iogen* iogen)
{
    struct iogen_job *job;
    int job_idx;
    void *res;

    iogen->state = iogen_state_aborting;

    for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
        job = &iogen->jobs[job_idx];
        if (job->thread) {
            pthread_join(job->thread, &res);
            job->thread = 0;
        }
    }

    iogen->state = iogen_state_aborted;

    return 0;
}

int iogen_abort(struct iogen* iogen)
{
    struct iogen_job *job;
    int job_idx;
    void *res;

    iogen->state = iogen_state_aborting;

    for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
        job = &iogen->jobs[job_idx];
        if (job->thread) {
            // pthread_join(job->thread, &res);

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;

            int ret = pthread_timedjoin_np(job->thread, &res, &ts);
            fprintf(stderr, "%s: pthread_timedjoin_np ret %d\n", __FUNCTION__, ret);

            job->thread = 0;
        }
    }

    iogen->state = iogen_state_aborted;

    iogen->params->skip_ioengine_exit = 1;
    fprintf(stderr, "%s: force to change skip_ioengin_exit = 1\n", __FUNCTION__);

    return 0;
}

int iogen_cancel(struct iogen* iogen)
{
    iogen->state = iogen_state_aborting;
    return 0;
}

int iogen_running(struct iogen* iogen)
{
    if ((iogen->state == iogen_state_running) ||
        (iogen->state == iogen_state_aborting)) {
        return 1;
    }

    return 0;
}

u32 iogen_eta(struct iogen* iogen)
{
    struct iogen_job *job;
    int job_idx;
    u32 eta;
    u32 max_eta = U32_MAX;

    for (job_idx = 0; job_idx < iogen->njobs; job_idx++) {
        job = &iogen->jobs[job_idx];
        if (job->state == iogen_job_state_running) {
            eta = iogen_job_eta(job);
            if ((max_eta == U32_MAX) || (eta > max_eta)) {
                max_eta = eta;
            }
        }
    }

    return max_eta;
}

