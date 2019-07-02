/**
 * @file main.c
 * @date 2016. 05. 16.
 * @author kmsun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <locale.h>
#include <time.h>
#include "ioengine.h"
#include "display.h"
#include "iogen.h"
#include "iover.h"
#include "bio.h"
#include "parser.h"
#include "bit.h"
#include "log.h"
#include "timespec_sub.h"
#include "powerctl.h"
#include "por.h"

#define MAJOR_VER       (0)
#define MINOR_VER       (1)

#define DISP_PERIOD_US  (100)

struct global_params params;
struct iogen iogen;
struct iover *iovers;
struct display disp;

struct iolog_writer iolog_writer;
struct smartlog_writer smartlog_writer;
iotrace_writer_t g_iotrace_writer;

pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;

struct ioacct *prev_accts;
struct ioacct *cur_accts;
struct ioacct *sub_accts;
nvme_smart_log_t *smartlog;

int aborted = 0;
int quiet = 0;
int iolog_enabled = 0;
int smartlog_enabled = 0;
int iotrace_enabled = 0;

int g_powerctl_dev_fd = -1;

int received_signal = 0;

u32 iosize_max;
u32 iosize_min;

static void signal_handler(int signo)
{
    if (iogen_running(&iogen)) {
        switch (signo) {
            case SIGINT:
            //printf("SIGINT signal processing %d\n", signo);
            iogen_stop(&iogen);
            break;

            case SIGUSR1:
            //printf("SIGUSR1 signal processing %d\n", signo);
            iogen_abort(&iogen);
            break;

            default:
            printf("unknown signal processing %d\n", signo);
            break;
        }
        
        aborted = 1;
    } else {
        aborted = -1;
    }

    received_signal = 1;
}

static void setup_signal_handler(void)
{
    struct sigaction sig_act;
    sigset_t newmask;

    memset(&sig_act, 0, sizeof (struct sigaction));
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    sigaddset(&newmask, SIGUSR1);
    sig_act.sa_handler = signal_handler;
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGUSR1, &sig_act, NULL);
    pthread_sigmask(SIG_UNBLOCK, &newmask, NULL);
    sigemptyset(&newmask);
}

static void wait_until(struct timespec *ts)
{
    pthread_mutex_lock(&timer_lock);
    pthread_cond_timedwait(&timer_cond, &timer_lock, ts);
    pthread_mutex_unlock(&timer_lock);
}

static void summarize_accts(struct ioacct *accts, int njobs, struct ioacct *sum_acct)
{
    int job_idx;
    int opcode_idx;

    memset(sum_acct, 0, sizeof (struct ioacct));

    for (job_idx = 0; job_idx < njobs; job_idx++){
        for (opcode_idx = 0; opcode_idx < bio_max; opcode_idx++) {
            sum_acct->ops[opcode_idx].ops += accts[job_idx].ops[opcode_idx].ops;
            sum_acct->ops[opcode_idx].bytes += accts[job_idx].ops[opcode_idx].bytes;
            sum_acct->ops[opcode_idx].latency.sum += accts[job_idx].ops[opcode_idx].latency.sum;
        }
    }
}

static void substract_acct(const struct ioacct *acct1, const struct ioacct *acct2, struct ioacct *res_acct)
{
    int opcode_idx;

    for (opcode_idx = 0; opcode_idx < bio_max; opcode_idx++) {
        res_acct->ops[opcode_idx].ops = acct1->ops[opcode_idx].ops - acct2->ops[opcode_idx].ops;
        res_acct->ops[opcode_idx].bytes = acct1->ops[opcode_idx].bytes - acct2->ops[opcode_idx].bytes;
        res_acct->ops[opcode_idx].latency.sum = acct1->ops[opcode_idx].latency.sum - acct2->ops[opcode_idx].latency.sum;
    }
}

static void update_acct(void)
{
    int job_idx;

    /* copy accounts for each job */
    for (job_idx = 0; job_idx < iogen.njobs; job_idx++) {
        cur_accts[job_idx + 1] = iogen.jobs[job_idx].acct;
    }
    /* summarize the accounts */
    summarize_accts(&cur_accts[1], iogen.njobs, &cur_accts[0]);

    for (job_idx = 0; job_idx <= iogen.njobs; job_idx++) {
        /* index 0 for summary, others for jobs  */
        substract_acct(&cur_accts[job_idx], &prev_accts[job_idx], &sub_accts[job_idx]);
    }
    if (!quiet) {
        /* update accounts display */
        disp.ops->update(&disp, sub_accts);
    }
    memcpy(prev_accts, cur_accts, sizeof(struct ioacct) * (iogen.njobs + 1));
}

static void free_accts(void)
{
    if (prev_accts) {
        free(prev_accts);
        prev_accts = NULL;
    }
    if (cur_accts) {
        free(cur_accts);
        cur_accts = NULL;
    }
    if (sub_accts) {
        free(sub_accts);
        sub_accts = NULL;
    }
}

static int alloc_accts(void)
{
    prev_accts = (struct ioacct *)calloc(iogen.njobs + 1, sizeof (struct ioacct));
    if (prev_accts == NULL) {
        fprintf(stderr, "memory not enough\n");
        goto error;
    }
    cur_accts = (struct ioacct *)calloc(iogen.njobs + 1, sizeof (struct ioacct));
    if (cur_accts == NULL) {
        fprintf(stderr, "memory not enough\n");
        goto error;
    }
    sub_accts = (struct ioacct *)calloc(iogen.njobs + 1, sizeof (struct ioacct));
    if (sub_accts == NULL) {
        fprintf(stderr, "memory not enough\n");
        goto error;
    }

    return 0;
error:
    free_accts();
    return -1;
}

static int get_smart_log(struct ioengine *engine, nvme_smart_log_t *smart_log)
{
    const u32 data_len = sizeof (nvme_smart_log_t);
    return engine->ops->get_log_page(engine, NVME_LOG_SMART, data_len, smart_log);
}

static u32 nr_nsis;
static struct namespace_info nsis[256];

static struct namespace_info *namespace_find(const nvme_id_ns_t *id_ns)
{
    u32 ns_idx;
    struct namespace_info *ns_info;

    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        if ((memcmp(ns_info->eui64, id_ns->eui64, sizeof (id_ns->eui64)) == 0) &&
            (memcmp(ns_info->nguid, id_ns->nguid, sizeof (id_ns->nguid)) == 0)) {
            return ns_info;
        }
    }

	return NULL;
}

static struct namespace_info *namespace_add(const nvme_id_ns_t *id_ns)
{
    struct namespace_info *ns_info;
    u8 idx;

    if (nr_nsis >= countof (nsis)) {
        return NULL;
    }

    ns_info = &nsis[nr_nsis++];

    memcpy(ns_info->eui64, id_ns->eui64, sizeof (id_ns->eui64));
    memcpy(ns_info->nguid, id_ns->nguid, sizeof (id_ns->nguid));

    idx = id_ns->flbas & 0xF;
    ns_info->lba_shift = id_ns->lbaf[idx].lbads;
    if (ns_info->lba_shift < 9) {
        fprintf(stderr, "nbio: invalid lba size(lbads %u)\n", ns_info->lba_shift);
        return NULL;
    }
    ns_info->data_size = 1 << id_ns->lbaf[idx].lbads;
    ns_info->meta_size = id_ns->lbaf[idx].ms;
    ns_info->block_size = ns_info->data_size;
    if (ns_info->meta_size) {
        ns_info->extended_lba = (id_ns->flbas >> 4) & 1;
        if (ns_info->extended_lba) {
            ns_info->block_size += ns_info->meta_size;
        }
    }
    ns_info->nlbs = id_ns->nsze;
    ns_info->pi_loc = (id_ns->dps >> 3) & 1;
    ns_info->pi_type = id_ns->dps & 0x7;

    return ns_info;
}

static int bind_params(struct job_params* params, struct ioengine *engine)
{
    int res;
    u32 feature_result;
    u64 ns_size;

    ns_size = params->ns_info->nlbs << params->ns_info->lba_shift;

    if (params->lba_range_begin_percent_enable) {
        params->lba_range_begin = (ns_size * params->lba_range_begin_percent / 100);
    }
    if (params->lba_range_size_percent_enable) {
        params->lba_range_size = (ns_size * params->lba_range_size_percent / 100);
    }
    if (params->amount_bytes_percent_enabled) {
        params->amount.bytes = (params->lba_range_size * params->amount_bytes_percent / 100);
    }
    if (params->lba_range_begin + params->lba_range_size > ns_size) {
        fprintf(stderr, "[%s] lba range out of namespace size\n", params->name);
        return -1;
    }

    switch (params->amount_type) {
        case amount_type_time:
            params->amount.seconds *= params->amount_scale;
            break;
        case amount_type_io:
            params->amount.ios *= params->amount_scale;
            break;
        case amount_type_byte:
            params->amount.bytes *= params->amount_scale;
            break;
        default:
            break;
    }

    if ((params->dealloc_read_result_type == dealloc_read_result_type_auto) ||
        (params->unwr_read_result_type == unwr_read_result_type_auto)) {
        res = engine->ops->get_features(engine, 5, 0, NULL, &feature_result);
        if (res != 0) {
            return res;
        }
        if (params->dealloc_read_result_type == dealloc_read_result_type_auto) {
            if (feature_result & (1 << 15)) {
                params->dealloc_read_result_type = dealloc_read_result_type_error;
            } else {
                params->dealloc_read_result_type = dealloc_read_result_type_value;
            }
        }
        if (params->unwr_read_result_type == unwr_read_result_type_auto) {
            if (feature_result & (1 << 15)) {
                params->unwr_read_result_type = unwr_read_result_type_error;
            } else {
                params->unwr_read_result_type = unwr_read_result_type_value;
            }
        }
    }

    return 0;
}

static int job_find_next(struct global_params *params, u32 idx, const char *name)
{
    u32 job_idx;

    for (job_idx = idx + 1; job_idx < params->njobs; job_idx++) {
        if (strcmp(params->job_params[job_idx].name, name) == 0) {
            return job_idx;
        }
    }
    return -1;
}

static int job_find(struct global_params *params, const char *name)
{
    u32 job_idx;

    for (job_idx = 0; job_idx < params->njobs; job_idx++) {
        if (strcmp(params->job_params[job_idx].name, name) == 0) {
            return job_idx;
        }
    }
    return -1;
}

static int validate(struct global_params *params)
{
    struct ioengine engine;
    struct job_params *job_params;
    nvme_id_ns_t *id_ns;
    void *id_ns_buf = NULL;
    struct namespace_info *ns_info;
    u32 job_idx;
    int depend_job_idx;
    int res;

    id_ns_buf = malloc(sizeof (nvme_id_ns_t) << 1);
    if (id_ns_buf == NULL) {
        fprintf(stderr, "nbio: not enough memory(%s:%d)\n", __FILE__, __LINE__);
        goto exit;
    }

    for (job_idx = 0; job_idx < params->njobs; job_idx++) {
        id_ns = (nvme_id_ns_t *)(id_ns_buf + ((rand() & 0x3FF) << 2));
        memset(id_ns, 0, sizeof (nvme_id_ns_t));

        job_params = &params->job_params[job_idx];
        if (strlen(job_params->ioengine) == 0) {
            fprintf(stderr, "please specify 'IOENGINE' property\n");
            res = -1;
            goto exit;
        }
        engine.ops = ioengine_find(job_params->ioengine);
        if (engine.ops == NULL) {
            fprintf(stderr, "unknown i/o engine '%s'\n", job_params->ioengine);
            res = -1;
            goto exit;
        }
        res = engine.ops->init(&engine, job_params->controller, job_params->nsid, 1, job_params->ioengine_param);
        if (res) {
            goto exit;
        }
        res = engine.ops->identify(&engine, 0, sizeof (nvme_id_ns_t), id_ns);
        if (res) {
            goto exit;
        }
        ns_info = namespace_find(id_ns);
        if (ns_info == NULL) {
            ns_info = namespace_add(id_ns);
            if (job_params->journal_file[0]) {
                ns_info->journal_file = job_params->journal_file;
            } else {
                ns_info->journal_file = NULL;
            }
            ns_info->journal_file_mmap = job_params->journal_file_mmap;
            ns_info->journal_shm = job_params->journal_shm;
            ns_info->nhist = job_params->history;
            ns_info->engine.ops = engine.ops;
            res = ns_info->engine.ops->init(&ns_info->engine, job_params->controller, job_params->nsid, 1, job_params->ioengine_param);
            if (res) {
                goto exit;
            }

            if (ns_info->nhist > 0) {
                fprintf(stdout, "\n");
                fprintf(stdout, "nbio: tracing ns_info changes\n");
                fprintf(stdout, "nbio: ns_info->lba_shift       = %d\n", ns_info->lba_shift);
                fprintf(stdout, "nbio: ns_info->data_size       = %d\n", ns_info->data_size);
                fprintf(stdout, "nbio: ns_info->meta_size       = %d\n", ns_info->meta_size);
                fprintf(stdout, "nbio: ns_info->block_size      = %d\n", ns_info->block_size);
                fprintf(stdout, "nbio: ns_info->nlbs            = %lld\n", ns_info->nlbs);
                fprintf(stdout, "\n");
            }
        } else {
            if ((ns_info->journal_file != NULL) || (job_params->journal_file[0])) {
                if ((ns_info->journal_file == NULL) ||
                    (strcmp(ns_info->journal_file, job_params->journal_file) != 0)) {
                    fprintf(stderr, "'JOURNAL_FILE' property redefined on the same namespace");
                    res = -1;
                    goto exit;
                }
            }
        }
        job_params->ns_info = ns_info;
		if (job_params->iosizes[iosize_total].iosize_min == 0 || job_params->iosizes[iosize_total].iosize_max == 0) {
			job_params->iosizes[iosize_total].iosize_min = iosize_min;
			job_params->iosizes[iosize_total].iosize_max = iosize_max;
		}

		if (job_params->iosizes[iosize_total].iosize_min == 0) {
            fprintf(stderr, "[%s] IO_SIZE_MIN(%llu) must be equal to or larger than lba size(%u)\n",
                    job_params->name, job_params->iosizes[iosize_total].iosize_min, ns_info->data_size);
            res = -1;
            goto exit;
        }
        if (job_params->iosizes[iosize_total].iosize_min % ns_info->data_size != 0) {
            fprintf(stderr, "[%s] IO_SIZE_MIN(%llu) must be aligned to lba size(%u) boundary\n",
                    job_params->name, job_params->iosizes[iosize_total].iosize_min, ns_info->data_size);
            res = -1;
            goto exit;
        }
        if (job_params->iosizes[iosize_total].iosize_max % ns_info->data_size != 0) {
            fprintf(stderr, "[%s] IO_SIZE_MAX(%llu) must be aligned to lba size(%u) boundary\n",
                    job_params->name, job_params->iosizes[iosize_total].iosize_max, ns_info->data_size);
            res = -1;
            goto exit;
        }

				
        if (job_params->dependency[0]) {
            depend_job_idx = job_find(params, job_params->dependency);
            if (depend_job_idx < 0) {
                fprintf(stderr, "'DEPENDENCY' property referenced undefined job '%s'\n", job_params->dependency);
                res = -1;
                goto exit;
            } else {
                if (job_find_next(params, depend_job_idx, job_params->dependency) >= 0) {
                    fprintf(stderr, "duplicated job name '%s'\n", job_params->dependency);
                    res = -1;
                    goto exit;
                } else {
                    job_params->dependency_job_idx = depend_job_idx;
                }
            }
        } else {
            job_params->dependency_job_idx = job_idx;
        }
        bind_params(job_params, &engine);
        engine.ops->exit(&engine);
    }

exit:
    if (id_ns_buf) {
        free(id_ns_buf);
    }
    return res;
}

static void append_smart_log(void)
{
    u32 ns_idx;
    struct namespace_info *ns_info;
    int res;

    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        res = get_smart_log(&ns_info->engine, smartlog);
        if (res == 0) {
            smartlog_append(&smartlog_writer, smartlog);
        } else {
            fprintf(stderr, "get_log_page() failed(%d(%x), %s)\n", res, res & 0x3FF, strerror(-res));
        }
    }
}

static void summarize_lat_accts(void)
{
    int job_idx;
    int op_idx;
    int bucket_idx;
    struct iogen_job *job;

    for (op_idx = 0; op_idx < bio_max; op_idx++) {
        memset(cur_accts[0].ops[op_idx].latency.buckets, 0, sizeof (cur_accts[0].ops[op_idx].latency.buckets));
        cur_accts[0].ops[op_idx].latency.min = U64_MAX;
        cur_accts[0].ops[op_idx].latency.max = 0;
    }
    /* copy accounts for each job */
    for (job_idx = 0; job_idx < iogen.njobs; job_idx++) {
        job = &iogen.jobs[job_idx];
        for (op_idx = 0; op_idx < bio_max; op_idx++) {
            if (cur_accts[0].ops[op_idx].latency.min > job->acct.ops[op_idx].latency.min) {
                cur_accts[0].ops[op_idx].latency.min = job->acct.ops[op_idx].latency.min;
            }

            if (cur_accts[0].ops[op_idx].latency.max < job->acct.ops[op_idx].latency.max) {
                cur_accts[0].ops[op_idx].latency.max = job->acct.ops[op_idx].latency.max;
            }

            for (bucket_idx = 0; bucket_idx < LATENCY_ACCT_NUM_BUCKETS; bucket_idx++) {
                cur_accts[0].ops[op_idx].latency.buckets[bucket_idx].count += job->acct.ops[op_idx].latency.buckets[bucket_idx].count;
                cur_accts[0].ops[op_idx].latency.buckets[bucket_idx].sum += job->acct.ops[op_idx].latency.buckets[bucket_idx].sum;
            }
        }
    }
}

static void write_iolog_summary(void)
{
    char line[128];
    struct opacct *opacct;
    u64 elapsed_time_ns;
    u64 read_throughput;
    u64 read_iops;
    u64 read_latency;
    u64 write_throughput;
    u64 write_iops;
    u64 write_latency;

    elapsed_time_ns = timespec_sub(&iogen.end_ts, &iogen.begin_ts);

    opacct = &cur_accts[0].ops[bio_read];
    read_throughput = (u64)((double)opacct->bytes * NSEC_PER_SEC / elapsed_time_ns);
    read_iops = (u64)((double)opacct->ops * NSEC_PER_SEC / elapsed_time_ns);
    if (opacct->ops == 0) {
        read_latency = 0;
    } else {
        read_latency = opacct->latency.sum / opacct->ops;
    }

    opacct = &cur_accts[0].ops[bio_write];
    write_throughput = (u64)((double)opacct->bytes * NSEC_PER_SEC / elapsed_time_ns);
    write_iops = (u64)((double)opacct->ops * NSEC_PER_SEC / elapsed_time_ns);
    if (opacct->latency.sum == 0) {
        write_latency = 0;
    } else {
        write_latency = opacct->latency.sum / opacct->ops;
    }
    sprintf(line, "# %.2lf, %llu, %llu, %llu, %llu, %llu, %llu", (double)elapsed_time_ns / NSEC_PER_SEC, read_throughput, read_iops, read_latency, write_throughput, write_iops, write_latency);
    iolog_append_line(&iolog_writer, line);
}

static int run(void)
{
    int res;
    struct timespec ts;

    res = iogen_start(&iogen);
    if (res) {
        return res;
    }

    clock_gettime(CLOCK_REALTIME, &ts);

    do {
        ts.tv_sec += 1;
        wait_until(&ts);

        update_acct();
        if (iolog_enabled) {
            iolog_append(&iolog_writer, sub_accts);
        }
        if (smartlog_enabled) {
            append_smart_log();
        }
    } while (iogen_running(&iogen));

    update_acct();

    if (!quiet) {
        summarize_lat_accts();
        if (iolog_enabled) {
            write_iolog_summary();
        }
        disp.ops->report(&disp, cur_accts);
    }

    return iogen.result;
}

static int prepare(void)
{
    u8 host_writes[16];
    u32 ns_idx;
    int res = 0;
    struct namespace_info *ns_info;

    /* io verfifier */
    iovers = calloc(nr_nsis, sizeof (struct iover));
    if (iovers == NULL) {
        goto iover_alloc_fail;
    }

    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        ns_info->iover = &iovers[ns_idx];
        res = iover_init(ns_info->iover, ns_info);
        if (res) {
            goto iover_init_fail;
        }
    }

    /* io generator */
    res = iogen_init(&iogen, &params);
    if (res) {
        goto iogen_init_fail;
    }

    res = posix_memalign((void **)&smartlog, getpagesize(), sizeof (nvme_smart_log_t));
    if (res) {
        fprintf(stderr, "posix_memalign() failed(%d)\n", res);
    }
    memset(smartlog, 0, sizeof (nvme_smart_log_t));
    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        if (ns_info->journal_file) {
            res = get_smart_log(&ns_info->engine, smartlog);
            if (res) {
                fprintf(stderr, "failed to get smart log\n");
            }
            iover_get_host_writes(ns_info->iover, host_writes);
#if 0
            if ((host_writes > 0) && (host_writes != smartlog->host_writes)) {
                fprintf(stderr, "namespace modification has been detected, data compare may be failed\n");
            }
#endif
        }
    }

    /* display*/
    res = disp.ops->init(&disp);
    if (res) {
        goto disp_init_fail;
    }

    /* iolog */
    if (iogen.params->iolog[0]) {
        iolog_enabled = 1;
        res = iolog_create(&iolog_writer, iogen.params->iolog, &iogen);
        if (res) {
            fprintf(stderr, "cannot create iolog '%s'\n", iogen.params->iolog);
            goto iolog_create_fail;
        }
    }

    /* smartlog */
    if (iogen.params->smartlog[0]) {
        smartlog_enabled = 1;
        res = smartlog_create(&smartlog_writer, iogen.params->smartlog);
        if (res) {
            fprintf(stderr, "cannot create smartlog '%s'\n", iogen.params->smartlog);
            goto smartlog_create_fail;
        }
    }

    if (iogen.params->errorlog[0]) {
        res = open(iogen.params->errorlog, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        dup2(res, STDERR_FILENO);
    }

    /* iotrace */
    if (iogen.params->iotrace[0]) {
        iotrace_enabled = 1;
        res = iotrace_create(&g_iotrace_writer, iogen.params->iotrace);
        if (res) {
            fprintf(stderr, "cannot create iotrace '%s'\n", iogen.params->iotrace);
            goto iotrace_create_fail;
        }
    }

#if (SUPPORT_SPOR_LEVEL_3 == 1)
    if (params.spor_level >= SPOR_LEVEL_2) {
        g_powerctl_dev_fd = powerctl_init(params.powerctl_dev);

        if (g_powerctl_dev_fd < 0) {
            printf("Failed PMU init\n");            
            res = -1;

            goto powerctl_init_fail;
        }

        printf("Initialized PMU\n");
    }
#endif

    /* account memory */
    res = alloc_accts();
    if (res) {
        goto alloc_accts_fail;
    }


    return res;

alloc_accts_fail:
    powerctl_exit(g_powerctl_dev_fd);

powerctl_init_fail:
    iotrace_close(&g_iotrace_writer);

iotrace_create_fail:    
    smartlog_close(&smartlog_writer);
smartlog_create_fail:
    iolog_close(&iolog_writer);
iolog_create_fail:
    disp.ops->exit(&disp);
disp_init_fail:
    iogen_exit(&iogen);
iogen_init_fail:
    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        iover_exit(ns_info->iover);
    }

iover_init_fail:
    free(iovers);
iover_alloc_fail:

    return res;
}

static void cleanup(void)
{
    u32 ns_idx;
    struct namespace_info *ns_info;

    if (iotrace_enabled) {
        iotrace_close(&g_iotrace_writer);
    }
    if (smartlog_enabled) {
        smartlog_close(&smartlog_writer);
    }
    if (iolog_enabled) {
        iolog_close(&iolog_writer);
    }

    free_accts();
    iogen_exit(&iogen);

    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        if (ns_info->journal_file) {
            get_smart_log(&ns_info->engine, smartlog);
            iover_set_host_writes(ns_info->iover, smartlog->host_writes);
        }

        if (params.skip_ioengine_exit == 0) {
            ns_info->engine.ops->exit(&ns_info->engine);
        } else {
            printf("%s: Skipped ioengine exit for SPO test, namespace: %u\n", __func__, ns_idx);
        }
        
        iover_exit(&iovers[ns_idx]);
    }

    free(iovers);
    free(smartlog);
    smartlog = NULL;

    if (params.job_params) {
        free(params.job_params);
        params.job_params = NULL;
    }

}

#if (SUPPORT_SPOR_LEVEL_3 == 1)
static void cleanup_spo(void)
{
    u32 ns_idx;
    struct namespace_info *ns_info;
    struct timespec ts_start, ts_end;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    // spo level 1, 2 waits completions for all write requests
    if (params.spor_level <= SPOR_LEVEL_2) {
        iogen_exit(&iogen);
    }

    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        ns_info = &nsis[ns_idx];
        if (ns_info->journal_file) {
            if (params.spor_level <= SPOR_LEVEL_1) {
                get_smart_log(&ns_info->engine, smartlog);
            }
            iover_set_host_writes(ns_info->iover, smartlog->host_writes);
        }
        printf("%s: Skipped ioengine exit for SPO test for SPO test, namespace: %u\n", __func__, ns_idx);
    }

    if ((params.spor_level >= SPOR_LEVEL_2) && (received_signal == 0)) {
        u64 diff_in_nsec;
        
        clock_gettime(CLOCK_MONOTONIC, &ts_end);        
        powerctl_power_off(g_powerctl_dev_fd);

        diff_in_nsec = timespec_sub(&ts_end, &ts_start);
        printf("Sent power off request\n");
        printf("Elapsed time from %s start and poweroff: %llu nano seconds\n", __func__, diff_in_nsec);
    }

    if (iotrace_enabled) {
        iotrace_close(&g_iotrace_writer);
    }
    if (smartlog_enabled) {
        smartlog_close(&smartlog_writer);
    }
    if (iolog_enabled) {
        iolog_close(&iolog_writer);
    }

    free_accts();


    for (ns_idx = 0; ns_idx < nr_nsis; ns_idx++) {
        iover_exit(&iovers[ns_idx]);
    }
    
    free(iovers);
    free(smartlog);
    smartlog = NULL;

    if (params.job_params) {
        free(params.job_params);
        params.job_params = NULL;
    }

    if (g_powerctl_dev_fd > 0) {
        close(g_powerctl_dev_fd);
    }
   
}
#endif

static void help(void)
{
    printf("nvme block i/o test v%d.%d\n", MAJOR_VER, MINOR_VER);
    printf("usage: nbio [OPTIONS] PARAMETER_FILE\n");
    printf("options:\n");
    printf("  -d [--debug]      : print debugging information\n");
    printf("  -q [--quiet]      : print nothing\n");
    printf("  --display ARG     : a display argument can be one of:\n");
    printf("                          standlone   print summary and job accounts(default)\n");
    printf("                          testcase    print summary only\n");
}

int main(int argc, char *argv[])
{
    char *param_file = NULL;
    char *arg;
    char *display = "standalone";
    int debug = 0;
    int arg_idx;
    int res;

    if (argc < 2) {
        help();
        return 0;
    }

    for (arg_idx = 1; arg_idx < argc; arg_idx++) {
        arg = argv[arg_idx];
        if ((strcmp(arg, "-d") == 0) ||
            (strcmp(arg, "--debug") == 0)) {
            debug = 1;
        } else if ((strcmp(arg, "-q") == 0) ||
                   (strcmp(arg, "--quiet") == 0)) {
            quiet = 1;
        } else if (strcmp(arg, "--display") == 0) {
            if (arg_idx + 1 >= argc) {
                help();
                return 0;
            }
            display = argv[++arg_idx];
        } else if ((strcmp(arg, "-s") == 0) ||
                   (strcmp(arg, "--shared") == 0)) {
        } else {
            if (arg[0] == '-' || param_file) {
                fprintf(stderr, "invalid option '%s'\n", arg);
                help();
                return 0;
            } else {
                param_file = arg;
            }
        }
    }

    if (param_file == NULL) {
        help();
        return 0;
    }

    params.spor_level = SPOR_LEVEL_0;
    params.skip_ioengine_exit = 0;
    res = parse_param(param_file, &params, debug);
    if (res) {
        return res;
    }

    setlocale(LC_ALL, "");
    setup_signal_handler();

    disp.ops = display_find(display);
    if (disp.ops == NULL) {
        fprintf(stderr, "unknown display mode '%s'\n", display);
        return -1;
    }
    disp.iogen = &iogen;

    res = validate(&params);
    if (res) {
        return res;
    }

    res = prepare();
    if (res) {
        return res;
    }

    res = run();

#if (SUPPORT_SPOR_LEVEL_3 == 1)
    if (params.spor_level >= SPOR_LEVEL_1) {
        cleanup_spo();
    } else {
        cleanup();
    }
#else
    cleanup();
#endif

    if (res) {
        return res;
    }

    return aborted;
}

