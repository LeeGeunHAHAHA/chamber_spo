/**
 * @file atomic_write.c
 * @date 2018. 10. 10
 * @author kmsun
 */

#include "spdk/nvme.h"

#define NS_ALLOC_UNIT       (1UL << 30)

#define IO_DEPTH            (32)
#define IO_SIZE             (512 << 10)     /* 512KB */
#define SLBA                (0)
#define TIME_IN_SEC         (30)

typedef struct __io_ctx io_ctx_t;

struct __io_ctx {
    int opcode;
    uint32_t id;

    io_ctx_t *next;
};

io_ctx_t io_ctx_pool[IO_DEPTH];
io_ctx_t *free_io_ctx;

enum {
    OPCODE_READ     = 1,
    OPCODE_WRITE    = 2,
};


struct ctrlr_entry {
    struct spdk_nvme_ctrlr  *ctrlr;
    struct ctrlr_entry      *next;
    char                    name[1024];
};

static struct ctrlr_entry *g_controllers = NULL;
static struct spdk_nvme_ctrlr* ctrlr;

static const struct spdk_nvme_ctrlr_data *cdata;
static struct spdk_nvme_qpair *ioqpair;
static struct spdk_nvme_ns *ns;

static uint64_t *payloads[IO_DEPTH];
static uint64_t write_timestamp;
static uint32_t nlb;
static uint32_t lba_size;
static uint32_t nr_inflight_io;
static uint32_t found_error;

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
}

static void teardown()
{
    spdk_nvme_detach(ctrlr);
}

static void io_cb(void *arg, const struct spdk_nvme_cpl *cpl)
{
    io_ctx_t *io_ctx = (io_ctx_t *)arg;
    uint64_t read_timestamp;
    uint64_t *payload;
    uint32_t i;

    if (spdk_nvme_cpl_is_error(cpl)) {
        fprintf(stderr, "I/O error occured!\n");
        found_error = 1;
        goto exit;
    }

    if (found_error) {
        goto exit;
    }

    if (io_ctx->opcode == OPCODE_READ) {
        /* verify the read data */

        payload = payloads[io_ctx->id];
        read_timestamp = payload[0];
        for (i = 1; i < nlb; i++) {
            if (payload[(i * lba_size) >> 3] != read_timestamp) {
                fprintf(stderr, "inconsistent read data detected!\n");
                found_error = 1;
                break;
            }
        }
    }

exit:
    /* append the entry into the free context list */
    io_ctx->next = free_io_ctx;
    free_io_ctx = io_ctx;

    nr_inflight_io--;
}

static int submit_read(io_ctx_t *io_ctx)
{
    uint64_t *payload;

    io_ctx->opcode = OPCODE_READ;
    payload = payloads[io_ctx->id];

    return spdk_nvme_ns_cmd_read(ns, ioqpair, payload, SLBA, nlb, io_cb, io_ctx, 0);
}

static int submit_write(io_ctx_t *io_ctx)
{
    int i;
    uint64_t *payload = payloads[io_ctx->id];

    io_ctx->opcode = OPCODE_WRITE;
    for (i = 0; i < nlb; i++) {
        payload[(i * lba_size) >> 3] = write_timestamp;
    }
    write_timestamp++;
    return spdk_nvme_ns_cmd_write(ns, ioqpair, payload, SLBA, nlb, io_cb, io_ctx, 0);
}

void run()
{
    io_ctx_t *io_ctx;
    uint32_t i;
    uint64_t tsc_end;

    ioqpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, NULL, 0);

    ns = spdk_nvme_ctrlr_get_ns(ctrlr, 1);
    if (spdk_nvme_ns_is_active(ns) == 0) {
        fprintf(stderr, "inactive namespace!");
        return;
    }
    lba_size = spdk_nvme_ns_get_sector_size(ns);
    nlb = IO_SIZE / lba_size;

    /* initialize */
    free_io_ctx = NULL;
    for (i = 0; i < IO_DEPTH; i++) {
        payloads[i] = (uint64_t*)spdk_dma_malloc(IO_SIZE, 0, NULL);
        io_ctx_pool[i].id = i;
        io_ctx_pool[i].next = free_io_ctx;
        free_io_ctx = &io_ctx_pool[i];
    }
    write_timestamp = 0;

	tsc_end = spdk_get_ticks() + TIME_IN_SEC * spdk_get_ticks_hz();

    do {
        while (free_io_ctx != NULL) {
            /* remove an entry from the free context list */
            io_ctx = free_io_ctx;
            free_io_ctx = io_ctx->next;

            if ((rand() % 10) < 8) {
                submit_read(io_ctx);
            } else {
                submit_write(io_ctx);
            }
            nr_inflight_io++;
        }
        spdk_nvme_qpair_process_completions(ioqpair, 0);
        if (found_error) {
            break;
        }
    } while (spdk_get_ticks() < tsc_end);

    do {
        spdk_nvme_qpair_process_completions(ioqpair, 0);
    } while (nr_inflight_io != 0);

    spdk_nvme_ctrlr_free_io_qpair(ioqpair);
}

int main(int argc, char *argv[])
{
    setup();

    run();

    teardown();

    return found_error;
}

