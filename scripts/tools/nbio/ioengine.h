/**
 * @file ioengine.h
 * @date 2016. 05. 13.
 * @author kmsun
 *
 * I/O engine interface definition
 */
#ifndef __NBIO_IOENGINE_H__
#define __NBIO_IOENGINE_H__

#include <pthread.h>
#include "list.h"
#include "params.h"
#include "nvme.h"

struct ioengine_ops;
struct ioengine {
    void *ctx;
    bool flush_passthru;
    struct ioengine_ops *ops;
};

struct ioengine_ops
{
    struct list_head list;

    char *name;
    /* per job instance */
    int (*init)(struct ioengine *engine, const char *device, u32 nsid, u32 iodepth, const char *param);
    int (*reinit)(struct ioengine *engine);
    void (*exit)(struct ioengine *engine);
    int (*begin)(struct ioengine *engine);
    void (*end)(struct ioengine *engine);
    int (*enqueue)(struct ioengine *engine, struct bio *bio);
    int (*submit)(struct ioengine *engine);
    int (*poll)(struct ioengine *engine, u32 min, u32 max);
    int (*identify)(struct ioengine *engine, u32 cdw10, u32 data_len, void *data);
    int (*get_log_page)(struct ioengine *engine, u8 log_id, u32 data_len, void *data);
    int (*get_features)(struct ioengine *engine, u32 cdw10, u32 data_len, void *data, u32 *result);
    int (*dma_alloc)(struct ioengine *engine, u32 size, u32 align, void **addr);
    void (*dma_free)(struct ioengine *engine, void *addr);

    /* reset */
    int (*reset)(struct ioengine *engine);
};

int ioengine_register(struct ioengine_ops *engine);
int ioengine_unregister(struct ioengine_ops *engine);
struct ioengine_ops *ioengine_find(char *name);

#endif /* __NBIO_IOENGINE_H__ */

