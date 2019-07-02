/**
 * @file test_ns_mgmt.c
 * @date 2018. 05. 24.
 * @author kmsun
 */

#include "types.h"
#include "testsuite.h"

#include "spdk/stdinc.h"
#include "spdk/nvme.h"
#include "spdk/env.h"
#include "spdk/log.h"
#include "nvme_internal.h"

#define NS_ALLOC_UNIT       (1UL << 30)
#define NR_NS               (10)

struct ctrlr_entry {
    struct spdk_nvme_ctrlr  *ctrlr;
    struct ctrlr_entry      *next;
    char                    name[1024];
};

static struct ctrlr_entry *g_controllers = NULL;
static struct spdk_nvme_ctrlr* ctrlr;

static u8 *payload;
static const struct spdk_nvme_ctrlr_data *cdata;

static bool probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr_opts *opts)
{
    return true;
}

static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
    struct ctrlr_entry *entry;

    entry = malloc(sizeof(struct ctrlr_entry));
    if (entry == NULL) {
        perror("ctrlr_entry malloc");
        exit(1);
    }

    cdata = spdk_nvme_ctrlr_get_data(ctrlr);
    snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

    entry->ctrlr = ctrlr;
    entry->next = g_controllers;
    g_controllers = entry;
}

static void setup()
{
    int rc;
    struct spdk_env_opts opts;

    g_spdk_log_print_level = SPDK_LOG_ERROR;

    spdk_env_opts_init(&opts);
    opts.name = "test_ns_mgmt";
    opts.shm_id = 0;
    if (spdk_env_init(&opts) < 0) {
        fprintf(stderr, "Unable to initialize SPDK env\n");
        return;
    }

    rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
    if (rc != 0) {
        fprintf(stderr, "spdk_nvme_probe() failed\n");
        return;
    }

    if (g_controllers == NULL) {
        fprintf(stderr, "no NVMe controllers found\n");
        return;
    }

    ctrlr = g_controllers->ctrlr;

    payload = spdk_dma_malloc(4096, 0, NULL);
}

static void teardown()
{
    spdk_nvme_detach(ctrlr);
}

int main(int argc, char *argv[])
{
    struct nvme_completion_poll_status status[NR_NS + 1];
    int res;
    int nr_cpl;
    struct spdk_nvme_ns_data *ndata;
    struct spdk_nvme_ctrlr_list *ctrl_list;
    u32 i;
    u32 j;

    setup();

    for (i = 0; i < 10; i++) {
        memset(status, 0, sizeof (status));

        ndata = (struct spdk_nvme_ns_data *)payload;
        memset(ndata, 0, 4096);
        ndata->nsze = 0x1000;
        ndata->ncap = 0x1000;
        ndata->flbas.format     = 0;        /* invalid lba format */
        ndata->dps.pit          = 0;
        ndata->dps.md_start     = 0;
        ndata->nmic.can_share   = 0;

        res = nvme_ctrlr_cmd_delete_ns(ctrlr, 0xFFFFFFFF, nvme_completion_poll_cb, &status[0]);
        for (j = 0; j < NR_NS; j++) {
            res = nvme_ctrlr_cmd_create_ns(ctrlr, ndata, nvme_completion_poll_cb, &status[j + 1]);
        }

        do {
            nr_cpl = 0;
            spdk_nvme_qpair_process_completions(ctrlr->adminq, 0);

            for (j = 0; j < (NR_NS + 1); j++) {
                if (status[j].cpl.status.sc != 0) {
                    printf("namespace management error cid %u sc %u\n", status[j].cpl.cid, status[j].cpl.status.sc);
                }
                if (status[j].done) {
                    nr_cpl++;
                }
            }
        } while (nr_cpl == (NR_NS + 1));

        ctrl_list = (struct spdk_nvme_ctrlr_list *)payload;
        memset(ctrl_list, 0, 4096);
        ctrl_list->ctrlr_count = 1;       /* number of identifiers */
        ctrl_list->ctrlr_list[0] = 1;     /* identifer 0 */

        for (j = 0; j < NR_NS; j++) {
            res = spdk_nvme_ctrlr_attach_ns(ctrlr, j + 1, ctrl_list);
            if (res != 0) {
                printf("failed to attach namespace %u\n", j + 1);
                break;
            }
        }
    }

    teardown();
}

