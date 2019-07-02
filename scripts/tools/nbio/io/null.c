/**
 * @file null.c
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * null i/o engine for testing purpose
 */

#include <malloc.h>
#include "ioengine.h"
#include "iogen.h"

struct null_io_queue {
    struct bio **elements;
    unsigned int size;
    unsigned int head;
    unsigned int tail;
};

struct null_io_data {
    struct null_io_queue queue;
};

int null_init(struct ioengine *engine, const char *device, u32 nsid, u32 iodepth, const char *param)
{
    struct null_io_data *nd;

    nd = (struct null_io_data *)calloc(1, sizeof (struct null_io_data));
    if (nd == NULL) {
        return -1;
    }
    nd->queue.elements = (struct bio **)calloc(iodepth + 1, sizeof (struct bio *));
    if (nd->queue.elements == NULL) {
        /* not enough memory! */
        return -1;
    }
    nd->queue.size = iodepth + 1;
    engine->ctx = nd;

    return 0;
}

void null_exit(struct ioengine *engine)
{
    struct null_io_data *nd;

    nd = (struct null_io_data *)engine->ctx;

    if (nd) {
        if (nd->queue.elements) {
            free(nd->queue.elements);
        }
        free(nd);
        engine->ctx = NULL;
    }
}

int null_enqueue(struct ioengine *engine, struct bio *bio)
{
    struct null_io_data *nd;
    unsigned int next_tail;
    
    nd = (struct null_io_data *)engine->ctx;

    next_tail = (nd->queue.tail + 1) % nd->queue.size;
    if (nd->queue.head == next_tail) {
        /* queue full, return busy */
        return -1;
    }
    nd->queue.elements[nd->queue.tail] = bio;
    nd->queue.tail = next_tail;

    return 0;
}

int null_submit(struct ioengine *engine)
{
    return 0;
}

int null_poll(struct ioengine *engine, u32 min, u32 max)
{
    struct null_io_data *nd;
    struct bio *bio;
    u32 nr_completion = 0;
    
    nd = (struct null_io_data *)engine->ctx;

    while (nd->queue.head != nd->queue.tail) {
        bio = nd->queue.elements[nd->queue.head];
        bio->result = 0;
        bio->end(bio);

        nd->queue.head = (nd->queue.head + 1) % nd->queue.size;
        nr_completion++;
        if (nr_completion == max) {
            break;
        }
    }

    return 0;
}

int null_identify(struct ioengine *engine, u32 cdw10, u32 data_len, void *data)
{
    struct nvme_id_ns *id_ns;

    switch (cdw10) {
        case 0:     /* namespace */
            id_ns = (struct nvme_id_ns *)data;
            id_ns->nsze = 10000000;
            id_ns->nlbaf = 1;
            id_ns->flbas = 0;
            id_ns->lbaf[0].ms = 0;
            id_ns->lbaf[0].lbads = 12;
            id_ns->lbaf[0].rp = 0;
            break;
        case 1:     /* controller */
            break;
        case 2:     /* namespace id list */
            break;
        default:
            break;
    }

    return 0;
}

int null_get_log_page(struct ioengine *engine, u8 log_id, u32 data_len, void *data)
{
    return 0;
}

int null_get_features(struct ioengine *engine, u32 cdw10, u32 data_len, void *data, u32 *result)
{
    return 0;
}

static struct ioengine_ops null_engine = {
    .name = "null",
    .init = null_init,
    .exit = null_exit,
    .enqueue = null_enqueue,
    .submit = null_submit,
    .poll = null_poll,
    .identify = null_identify,
    .get_log_page = null_get_log_page,
    .get_features = null_get_features,
};

void __attribute__((constructor)) null_register(void)
{
    ioengine_register(&null_engine);
}

void __attribute__((destructor)) null_unregister(void)
{
    ioengine_unregister(&null_engine);
}

