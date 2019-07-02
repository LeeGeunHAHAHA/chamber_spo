/*
 * file spdkio.c
 * data 2018. 05. 28
 * author mkpark
 *
 * spdk i/o engine for testing
 */

#include <stdio.h>
#include <stdlib.h>

#include "ioengine.h"
#include "iogen.h"

#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/env.h"
#include "spdk/log.h"
#include "spdk/util.h"

#define NAME_LEN        (512)
#define SGL_ENTRY_SIZE  (4096)
#define APPMASK_SHIFT	(16)

struct ctrlr_entry {
    struct spdk_nvme_ctrlr  *ctrlr;
    struct ctrlr_entry      *next;
    char                    name[NAME_LEN];
    char                    pci_addr[NAME_LEN];
};

struct global_data {
    int                     nr_instances;
    struct spdk_env_opts    opts;
};

struct instance_data {
    struct spdk_nvme_ctrlr  *ctrlr;
    struct spdk_nvme_ns     *ns;
    struct spdk_nvme_qpair  *qpair;
    uint32_t                nsid;
    int                     done;
    int                     sgl;
    int                     wrru;
    int                     qprio;
    int                     cmb_sqs;
};

static struct ctrlr_entry   *g_controllers;
static struct global_data   gdata;

static bool set_idata(void *ctx, const char *pci_addr, uint32_t nsid)
{
    struct ctrlr_entry *ctrlr;
    struct instance_data *idata;

    idata = (struct instance_data *)ctx;
    ctrlr = g_controllers;

    while (ctrlr != NULL) {
        if (strcmp(ctrlr->pci_addr, pci_addr) == 0) {
            if (spdk_nvme_ctrlr_is_active_ns(ctrlr->ctrlr, nsid) == true) {
                idata->ctrlr = ctrlr->ctrlr;
                idata->ns = spdk_nvme_ctrlr_get_ns(ctrlr->ctrlr, nsid);
                idata->nsid = nsid;
                return true;
            }
        }
        ctrlr = ctrlr->next;
    }
    return false;
}
static bool set_idata_qpair(void *ctx)
{
    struct instance_data *idata;
    struct spdk_nvme_io_qpair_opts opts;

    idata = (struct instance_data *)ctx;

    spdk_nvme_ctrlr_get_default_io_qpair_opts(idata->ctrlr, &opts, sizeof(opts));
    opts.io_queue_size = 1024;
    opts.io_queue_requests = 2048;
    if (idata->wrru) {
        switch (idata->qprio) {
            case 0:
                opts.qprio = SPDK_NVME_QPRIO_URGENT;
                break;
            case 1:
                opts.qprio = SPDK_NVME_QPRIO_HIGH;
                break;
            case 2:
                opts.qprio = SPDK_NVME_QPRIO_MEDIUM;
                break;
            case 3:
                opts.qprio = SPDK_NVME_QPRIO_LOW;
                break;
        }
    }
    idata->qpair = spdk_nvme_ctrlr_alloc_io_qpair(idata->ctrlr, &opts, sizeof(opts));

    if(idata->qpair == NULL){
        return false;
    }

    return true;
}
static void complete(void *arg, const struct spdk_nvme_cpl *completion)
{
    struct bio *bio = (struct bio *)arg;
    if (spdk_nvme_cpl_is_error(completion)) {
        fprintf(stderr, "sc %xh, sct %xh\n", completion->status.sc, completion->status.sct);
    }
    bio->result = spdk_nvme_cpl_is_error(completion) ? -1 : 0;
    bio->end(bio);
}

static void completion_cb(void *cb_ctx, const struct spdk_nvme_cpl *cpl)
{
    struct instance_data *idata;

    idata = (struct instance_data *)cb_ctx;

    if (spdk_nvme_cpl_is_error(cpl)) {
        printf("get log page complete failed\n");
    }

    idata->done = 1;
}

static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr_opts *opts) 
{
    struct instance_data *idata = (struct instance_data *)cb_ctx;
    opts->use_cmb_sqs = idata->cmb_sqs;
    if (idata->wrru) {
        opts->arb_mechanism = SPDK_NVME_CC_AMS_WRR;
    }
    return true;
}

static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
    struct ctrlr_entry *entry;
    const struct spdk_nvme_ctrlr_data *cdata;
    struct spdk_pci_addr pci_addr;
    struct spdk_pci_device *pci_dev;

    cdata = spdk_nvme_ctrlr_get_data(ctrlr);

    entry = malloc(sizeof(struct ctrlr_entry));
    if (entry == NULL) {
        perror("ctrlr_entry calloc");
        exit(1);
    }

    if (spdk_pci_addr_parse(&pci_addr, trid->traddr)) {
        return;
    }
    
    pci_dev = spdk_nvme_ctrlr_get_pci_device(ctrlr);
    if (!pci_dev) {
        return;
    }

    snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);
    snprintf(entry->pci_addr, sizeof(entry->pci_addr), "%s", trid->traddr);

    entry->ctrlr = ctrlr;
    entry->next = g_controllers;
    g_controllers = entry;
}

static int parse_param(struct instance_data *idata, const char *param)
{
    const char *str;
    const char *c;
    int len = 0;

    str = c = param;

    while (1) {
        if (((*c == ',') || (*c == '\0')) && (len != 0)) {
            if (strncmp(str, "sgl", len) == 0) {
                idata->sgl = 1;
            } else if (strncmp(str, "wrru", len) == 0) {
                idata->wrru = 1;
            } else if (strncmp(str, "qprio_urgent", len) == 0) {
                idata->qprio = 0;
            } else if (strncmp(str, "qprio_high", len) == 0) {
                idata->qprio = 1;
            } else if (strncmp(str, "qprio_medium", len) == 0) {
                idata->qprio = 2;
            } else if (strncmp(str, "qprio_low", len) == 0) {
                idata->qprio = 3;
            } else if (strncmp(str, "cmb_sqs", len) == 0) {
                idata->cmb_sqs = 1;
            } else {
                fprintf(stderr, "spdkio: invalid parameter '%s'\n", str);
            }
            len = 0;
            str = c + 1;
        } else {
            len++;
        }
        if (*c == '\0') {
            break;
        }
        c++;
    }

    return 0;
}

int spdkio_init(struct ioengine *engine, const char *controller, u32 nsid, u32 iodepth, const char *param)
{
    struct instance_data *idata;
    int rc;
    char *value;

    idata = malloc(sizeof(struct instance_data));
    if (idata == NULL) {
        fprintf(stderr, "not enough memory\n");
        goto error;
    }
    memset(idata, 0, sizeof(struct instance_data));

    engine->ctx = idata;

    parse_param(idata, param);

    if (gdata.nr_instances == 0) {
        spdk_env_opts_init(&gdata.opts);

        gdata.opts.name = "spdkio";
        gdata.opts.shm_id = -1;
        gdata.opts.mem_size = 2048;

        if ((value = getenv("FQA_SPDK_SHM_ID")) != NULL) {
            gdata.opts.shm_id = atoi(value);
        }

        if ((value = getenv("FQA_SPDK_MEM_SIZE")) != NULL) {
            gdata.opts.mem_size = atoi(value);
        }

        if (spdk_env_init(&gdata.opts) < 0) {
            fprintf(stderr, "Unable to initialize SPDK env\n");
            goto error; 
        }

        rc = spdk_nvme_probe(NULL, idata, probe_cb, attach_cb, NULL);
        if (rc != 0) {
            fprintf(stderr, "spdk_nvme_probe() failed\n");
            goto error; 
        }

        if (g_controllers == NULL) {
            fprintf(stderr, "no NVMe controllers founded\n");
            goto error; 
        }
        spdk_unaffinitize_thread();

        spdk_log_set_level(SPDK_LOG_ERROR);
        spdk_log_set_print_level(SPDK_LOG_ERROR);
    }
    gdata.nr_instances++;

    if ((nsid == 0) || (nsid == 0xFFFFFFFF)) {
        fprintf(stderr, "ERROR: invalid namespace id(%x)\n", nsid);
        goto error;
    }

    if(!set_idata(engine->ctx, controller, nsid)){
        fprintf(stderr, "ERROR: namespace(controller %s nsid %u) is not exist\n", controller, nsid);
        goto error;
    }
    if ((idata->sgl) &&
        (spdk_nvme_ctrlr_get_data(idata->ctrlr)->sgls.supported == SPDK_NVME_SGLS_NOT_SUPPORTED)) {
        fprintf(stderr, "ERROR: controller %s does not support sgl\n", controller);
    }

//    if (1) {    // harry TODO - command line option
//        spdk_nvme_ctrlr_cmd_enable_streams(idata->ctrlr);
//    }
    return 0;

error:
    if (idata) {
        free(idata);
    }
    return -1;
}

void spdkio_exit(struct ioengine *engine)
{
    struct instance_data *idata;
    struct ctrlr_entry *ctrlr;
    struct ctrlr_entry *ctrlr_next;

    idata = (struct instance_data *)engine->ctx;
    gdata.nr_instances--;
    if (gdata.nr_instances == 0) {
        ctrlr = g_controllers;

        while (ctrlr != NULL) {
            spdk_nvme_detach(ctrlr->ctrlr);

            ctrlr_next = ctrlr->next;
            free(ctrlr);
            ctrlr = ctrlr_next;
        }
        g_controllers = NULL;
    }
    
    free(idata);
}

int spdkio_begin(struct ioengine *engine)
{
    struct instance_data *idata = (struct instance_data *)engine->ctx;

    if (!set_idata_qpair(engine->ctx)) {
        printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
        return -1;
    }

    if (idata->wrru) {
        idata->done = 0;
        spdk_nvme_ctrlr_cmd_set_feature(idata->ctrlr, 1, 0x40201001, 0, NULL, 0, completion_cb, idata);
        while(idata->done == 0) {
            spdk_nvme_ctrlr_process_admin_completions(idata->ctrlr);
        }
    }

    return 0;
}

void spdkio_end(struct ioengine *engine)
{
    struct instance_data *idata = (struct instance_data *)engine->ctx;
    spdk_nvme_ctrlr_free_io_qpair(idata->qpair);
}

static int spdkio_dealloc(struct instance_data *idata, struct bio *bio) {
    struct spdk_nvme_dsm_range *range;
    uint64_t offset, remaining;
    uint64_t num_ranges_u64;
    uint16_t num_ranges;

    num_ranges_u64 = ((u64)bio->nlbs + SPDK_NVME_DATASET_MANAGEMENT_RANGE_MAX_BLOCKS - 1) / SPDK_NVME_DATASET_MANAGEMENT_RANGE_MAX_BLOCKS;
    
    if (num_ranges_u64 > 64) {
        fprintf(stderr, "ERROR: Unmap request for %" PRIu32 " blocks is too large\n", bio->nlbs);
        return -EINVAL;
    }
    num_ranges = (uint16_t)num_ranges_u64;

    offset = bio->slba;
    remaining = bio->nlbs;
    range = bio->data;

    while(remaining > SPDK_NVME_DATASET_MANAGEMENT_RANGE_MAX_BLOCKS) {
        range->attributes.raw = 0;
        range->length = SPDK_NVME_DATASET_MANAGEMENT_RANGE_MAX_BLOCKS;
        range->starting_lba = offset;

        offset += SPDK_NVME_DATASET_MANAGEMENT_RANGE_MAX_BLOCKS;
        remaining -= SPDK_NVME_DATASET_MANAGEMENT_RANGE_MAX_BLOCKS;
        range++;
    }

    range->attributes.raw = 0;
    range->length = remaining;
    range->starting_lba = offset;

    return spdk_nvme_ns_cmd_dataset_management(idata->ns, idata->qpair, SPDK_NVME_DSM_ATTR_DEALLOCATE, bio->data, num_ranges, complete, bio);
}

static int spdkio_write_uncorrect(struct instance_data *idata, struct bio *bio)
{
    struct spdk_nvme_cmd cmd;

    memset(&cmd, 0, sizeof (cmd));
    cmd.opc = SPDK_NVME_OPC_WRITE_UNCORRECTABLE;
    cmd.nsid = idata->nsid;
    *(uint64_t *)&cmd.cdw10 = bio->slba;
    cmd.cdw12 = bio->nlbs - 1;
    return spdk_nvme_ctrlr_cmd_io_raw(idata->ctrlr, idata->qpair, &cmd, NULL, 0, complete, bio);
}
static int spdkio_write_zeroes(struct instance_data *idata, struct bio *bio, uint32_t flag, uint16_t appmask, uint16_t apptag)
{
	struct spdk_nvme_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opc = SPDK_NVME_OPC_WRITE_ZEROES;
	cmd.nsid = idata->nsid;
	*(uint64_t *)&cmd.cdw10 = bio->slba;
	cmd.cdw12 = flag | (bio->nlbs - 1);
	*(uint64_t *)&cmd.cdw14 = bio->slba;
	cmd.cdw15 = (appmask << APPMASK_SHIFT) | apptag;
	return spdk_nvme_ctrlr_cmd_io_raw(idata->ctrlr, idata->qpair, &cmd, NULL, 0, complete, bio);
}

static void reset_sgl(void *cb_arg, uint32_t offset)
{
    struct bio *bio = (struct bio *)cb_arg;

    bio->spdk.sgl_offset = offset;
}

static int next_sge(void *cb_arg, void **address, uint32_t *length)
{
    struct bio *bio = (struct bio *)cb_arg;
    uint32_t len;
    uint64_t addr;

    *address = bio->data + bio->spdk.sgl_offset;
    addr = (uint64_t)*address;
    len = MIN(SGL_ENTRY_SIZE, bio->iosize - bio->spdk.sgl_offset);
    if ((len >= PAGE_SIZE) && (addr & (~PAGE_MASK)) && ((addr + len) & (~PAGE_MASK))) {
        len -= (addr + len) & (~PAGE_MASK);
    }
    bio->spdk.sgl_offset += len;
    *length = len;
    return 0;
}

int spdkio_enqueue(struct ioengine *engine, struct bio *bio)
{
    struct instance_data *idata;
    struct spdk_nvme_cmd cmd;
    uint32_t flag = 0;
    uint16_t appmask = 0;
    uint16_t apptag = 0;
    int res;

    idata = (struct instance_data *)engine->ctx;

    if (bio->job->params->ns_info->pi_type) {
		if (bio->opcode == bio_write_zeroes) {
			bio->pract = 1;
		} else {
        	flag = bio->prchk << 26;
		}
        flag |= bio->pract << 29;
        appmask = 0xFFFF;
        apptag = bio->apptag;
    }
    switch (bio->opcode) {
        case bio_read:
            if (idata->sgl) {
                res = spdk_nvme_ns_cmd_readv_with_md(idata->ns, idata->qpair, bio->slba, bio->nlbs, complete, bio, flag, reset_sgl, next_sge, bio->meta, appmask, apptag);
            } else {
                res = spdk_nvme_ns_cmd_read_with_md(idata->ns, idata->qpair, bio->data, bio->meta, bio->slba, bio->nlbs, complete, bio, flag, appmask, apptag);
            }
            break;
        case bio_write:
            if (idata->sgl) {
                if (bio->stream_id == 0) {
                    res = spdk_nvme_ns_cmd_writev_with_md(idata->ns, idata->qpair, bio->slba, bio->nlbs, complete, bio, flag, reset_sgl, next_sge, bio->meta, appmask, apptag);
                } else {
                    res = spdk_nvme_ns_cmd_writev_stream_with_md(idata->ns, idata->qpair, bio->slba, bio->nlbs, bio->stream_id, complete, bio, flag, reset_sgl, next_sge, bio->meta, appmask, apptag);
                }
            } else {
                if (bio->stream_id == 0) {
                    res = spdk_nvme_ns_cmd_write_with_md(idata->ns, idata->qpair, bio->data, bio->meta, bio->slba, bio->nlbs, complete, bio, flag, appmask, apptag);
                } else {
                    res = spdk_nvme_ns_cmd_write_stream_with_md(idata->ns, idata->qpair, bio->data, bio->meta, bio->slba, bio->nlbs, bio->stream_id, complete, bio, flag, appmask, apptag);
                }
            }
            break;
        case bio_flush:
            res = spdk_nvme_ns_cmd_flush(idata->ns, idata->qpair, complete, bio);
            break;
        case bio_deallocate:
            res = spdkio_dealloc(idata, bio);
            break;
        case bio_write_zeroes:
			res = spdkio_write_zeroes(idata, bio, flag, appmask, apptag);
            break;
        case bio_write_uncorrect:
		   	res = spdkio_write_uncorrect(idata, bio);
            break;
        case bio_identify:
            memset(&cmd, 0, sizeof (struct spdk_nvme_cmd));
            cmd.opc = SPDK_NVME_OPC_IDENTIFY;
            cmd.nsid = idata->nsid;
            res = spdk_nvme_ctrlr_cmd_admin_raw(idata->ctrlr, &cmd, bio->data, 4096, complete, bio);
            break;
        default:
            res = -1;
            break;
    }
    if (res) {
        fprintf(stderr, "error %d(%s)\n", res, strerror(res));
    }
    return res;
}

int spdkio_submit(struct ioengine *engine)
{
    return 0;
}

int spdkio_poll(struct ioengine *engine, u32 min, u32 max)
{
    struct instance_data *idata;

    idata = (struct instance_data *)engine->ctx;

    spdk_nvme_qpair_process_completions(idata->qpair, max);
    //spdk_nvme_ctrlr_process_admin_completions(idata->ctrlr);

    return 0;
}

int spdkio_identify(struct ioengine *engine, u32 cdw10, u32 data_len, void *data)
{
    struct instance_data *idata;
    const struct spdk_nvme_ns_data *nsdata;
    struct spdk_nvme_ns *ns;

    idata = (struct instance_data *)engine->ctx;
    ns = idata->ns;

    nsdata = spdk_nvme_ns_get_data(ns);
    memcpy(data, nsdata, data_len);
    return 0;
}

int spdkio_alloc(struct ioengine *engine, u32 size, u32 align, void **addr){
    *addr = spdk_dma_zmalloc(size, align, NULL);
    if (*addr == NULL) {
        return -1;
    }
    return 0;
}

void spdkio_free(struct ioengine *engine, void *addr){
    spdk_dma_free(addr);
}

int spdkio_get_log_page(struct ioengine *engine, u8 log_id, u32 data_len, void *data)
{
    struct instance_data *idata;

    idata = (struct instance_data *)engine->ctx;

    if (spdk_nvme_ctrlr_is_log_page_supported(idata->ctrlr, log_id) == false) {
        return -1;
    }

    idata->done = 0;
    spdk_nvme_ctrlr_cmd_get_log_page(idata->ctrlr, log_id, idata->nsid, data, data_len, 0, completion_cb, idata);
    while(idata->done == 0) {
        spdk_nvme_ctrlr_process_admin_completions(idata->ctrlr);
    }

    return 0;
}

int spdkio_get_features(struct ioengine *engine, u32 cdw10, u32 data_len, void *data, u32 *result)
{
    struct instance_data *idata;
    uint8_t feature;

    idata = (struct instance_data *)engine->ctx;
    feature = cdw10 & 0xFF;

    if (spdk_nvme_ctrlr_is_feature_supported(idata->ctrlr, feature) == false) {
        return -1;
    }

    idata->done = 0;
    spdk_nvme_ctrlr_cmd_get_feature(idata->ctrlr, feature, 0, data, data_len, completion_cb, idata);
    while(idata->done == 0) {
        spdk_nvme_ctrlr_process_admin_completions(idata->ctrlr);
    }

    return 0;
}

static struct ioengine_ops spdkio_engine = {
    .name           = "spdk",
    .init           = spdkio_init,
    .exit           = spdkio_exit,
    .begin          = spdkio_begin,
    .end            = spdkio_end,
    .enqueue        = spdkio_enqueue,
    .submit         = spdkio_submit,
    .poll           = spdkio_poll,
    .identify       = spdkio_identify,
    .dma_alloc      = spdkio_alloc,
    .dma_free       = spdkio_free,
    .get_log_page   = spdkio_get_log_page,
    .get_features   = spdkio_get_features,
};

void __attribute__((constructor)) spdkio_register(void)
{
    ioengine_register(&spdkio_engine);
}

void __attribute__((destructor)) spdkio_unregister(void)
{
    ioengine_unregister(&spdkio_engine);
}
