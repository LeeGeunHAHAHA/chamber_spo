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
#define NR_NS               (128)

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
    struct nvme_completion_poll_status status[NR_NS * 2];
    int nr_cpl;
    int res;
    struct spdk_nvme_cmd cmd;
    struct spdk_nvme_format format;
    u32 i;
    u32 j;

    setup();

    memset(&format, 0, sizeof (format));
    memset(&cmd, 0, sizeof (cmd));

    cmd.opc = SPDK_NVME_OPC_IDENTIFY;

    for (i = 0; i < 10; i++) {
        memset(status, 0, sizeof (status));

        for (j = 0; j < NR_NS/2; j++) {
            nvme_ctrlr_cmd_format(ctrlr, j + 1, &format, nvme_completion_poll_cb, &status[j]);
        }

        for (j = 0; j < NR_NS/2; j++) {
            cmd.nsid = j + 1;
            res = spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, payload, 4096, nvme_completion_poll_cb, &status[j + NR_NS]);
            if (res) {
                printf("j %u, res %d\n", j, res);
                goto exit;
            }
        }

        for (j = NR_NS/2; j < NR_NS; j++) {
            nvme_ctrlr_cmd_format(ctrlr, j + 1, &format, nvme_completion_poll_cb, &status[j]);
        }

        for (j = NR_NS/2; j < NR_NS; j++) {
            cmd.nsid = j + 1;
            res = spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, payload, 4096, nvme_completion_poll_cb, &status[j + NR_NS]);
            if (res) {
                printf("j %u, res %d\n", j, res);
                goto exit;
            }
        }

        do {
            nr_cpl = 0;
            spdk_nvme_qpair_process_completions(ctrlr->adminq, 0);

            for (j = 0; j < (NR_NS * 2); j++) {
                if (status[j].done) {
                    nr_cpl++;
                    if (status[j].cpl.status.sc != 0) {
                        printf("status error cid %u sc %u\n", status[j].cpl.cid, status[j].cpl.status.sc);
                    }
                }
            }
        } while (nr_cpl != (NR_NS * 2));
    }

exit:
    teardown();

    return 0;
}

