/**
 * @file libaio.c
 * @date 2016. 05. 23.
 * @author kmsun
 *
 * libaio-based nvme i/o engine 
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <libaio.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <malloc.h>
#include <limits.h>
#include "ioengine.h"
#include "memory.h"
#include "bit.h"
#include "iogen.h"

#ifndef BLKDISCARD
#define BLKDISCARD      _IO(0x12,119)
#endif

struct nvme_passthru_cmd {
    u8  opcode;
    u8  flags;
    u16 rsvd1;
    u32 nsid;
    u32 cdw2;
    u32 cdw3;
    u64 metadata;
    u64 addr;
    u32 metadata_len;
    u32 data_len;
    u32 cdw10;
    u32 cdw11;
    u32 cdw12;
    u32 cdw13;
    u32 cdw14;
    u32 cdw15;
    u32 timeout_ms;
    u32 result;
};

#define NVME_ADMIN_GET_LOG_PAGE     (0x02)
#define NVME_ADMIN_IDENTIFY         (0x06)
#define NVME_ADMIN_GET_FEATURES     (0x0A)
#define NVME_CMD_FLUSH              (0x00)
#define NVME_IOCTL_ID               _IO('N', 0x40)
#define NVME_IOCTL_ADMIN_CMD        _IOWR('N', 0x41, struct nvme_passthru_cmd)
#define NVME_IOCTL_IO_CMD           _IOWR('N', 0x43, struct nvme_passthru_cmd)
#define FLUSH_IO_PASSTHRU           1

struct libaio_data {
    io_context_t io_context;
    struct io_event *io_events;
    int fd;
    u16 nsid;
    struct iocb **iocbs;
    u32 iodepth;
    u32 tail;
    u32 head;
    u32 nr_enqueued;
};

static int libaio_identify(struct ioengine *engine, u32 cdw10, u32 data_len, void *data);
static int libaio_submit(struct ioengine *engine);

static int libaio_init(struct ioengine *engine, const char *controller, u32 nsid, u32 iodepth, const char *param)
{
    struct nvme_id_ns *id_ns = NULL;
    struct libaio_data *ld;
    int res;
    char *namespace = NULL;

    ld = (struct libaio_data *)calloc(1, sizeof (struct libaio_data));
    if (ld == NULL) {
        fprintf(stderr, "not enough memory\n");
        res = -1;
        goto error;
    }
    memset(ld, 0, sizeof (struct libaio_data));
    engine->ctx = ld;
    ld->fd = -1;
    ld->iodepth = iodepth;

    res = io_setup(ld->iodepth, &ld->io_context);
    if (res != 0) {
        fprintf(stderr, "io_setup() failed(%d, %s)!\n", res, strerror(-res));
        goto error;
    }

    ld->io_events = calloc(ld->iodepth, sizeof (struct io_event));
    if (ld->io_events == NULL) {
        fprintf(stderr, "not enough memory\n");
        res = -1;
        goto error;
    }

    ld->iocbs = calloc(ld->iodepth, sizeof (struct iocb *));
    if (ld->iocbs == NULL) {
        fprintf(stderr, "not enough memory\n");
        res = -1;
        goto error;
    }

    namespace = malloc(strlen(controller) + 8);
    if (namespace == NULL) {
        fprintf(stderr, "not enough memory\n");
        res = -1;
        goto error;
    }
    sprintf(namespace, "%sn%u", controller, nsid);
    ld->fd = open(namespace, O_RDWR | O_DIRECT);
    if (ld->fd < 0) {
        fprintf(stderr, "cannot open '%s'\n", namespace);
        res = ld->fd;
        goto error;
    }

    ld->nsid = ioctl(ld->fd, NVME_IOCTL_ID);
    if (ld->nsid <= 0) {
        res = -1;
        goto error;
    }

    return 0;

error:
    if (id_ns) {
        free(id_ns);
    }

    if (ld->fd >= 0) {
        close(ld->fd);
        ld->fd = -1;
    }

    if (namespace) {
        free(namespace);
        namespace = NULL;
    }
    if (ld->iocbs) {
        free(ld->iocbs);
        ld->iocbs = NULL;
    }

    if (ld->io_events) {
        free(ld->io_events);
        ld->io_events = NULL;
    }
    io_destroy(ld->io_context);
    if (engine->ctx) {
        free(engine->ctx);
        engine->ctx = NULL;
    }

    return res;
}

static void libaio_exit(struct ioengine *engine)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;

    if (ld->fd >= 0) {
        close(ld->fd);
        ld->fd = -1;
    }

    if (ld->iocbs) {
        free(ld->iocbs);
        ld->iocbs = NULL;
    }

    if (ld->io_events) {
        free(ld->io_events);
        ld->io_events = NULL;
    }

    io_destroy(ld->io_context);

    free(engine->ctx);
    engine->ctx = NULL;
}

static int libaio_flush(struct ioengine *engine)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    struct nvme_passthru_cmd cmd = {
        .opcode     = NVME_CMD_FLUSH,
        .nsid       = ld->nsid,
    };

    return ioctl(ld->fd, NVME_IOCTL_IO_CMD, &cmd);
}

static int libaio_enqueue(struct ioengine *engine, struct bio *bio)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    u64 range[2];

    if (ld->nr_enqueued >= ld->iodepth) {
        fprintf(stderr, "queueu is full\n");
        return -1;
    }

    switch (bio->opcode) {
        case bio_read:
            io_prep_pread(&bio->iocb, ld->fd, bio->data, bio->iosize, bio->offset);
            break;
        case bio_write:
            io_prep_pwrite(&bio->iocb, ld->fd, bio->data, bio->iosize, bio->offset);
            break;
        case bio_flush:
            libaio_submit(engine);
            if (engine->flush_passthru) {
                return libaio_flush(engine);
            } else {
                return fsync(ld->fd);
            }
        case bio_deallocate:
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)
            /* kernel 4.7 support async discard */
            libaio_submit(engine);
#endif
            range[0] = bio->offset;
            range[1] = bio->iosize;
            return ioctl(ld->fd, BLKDISCARD, &range);
        case bio_write_zeroes:
        case bio_write_uncorrect:
            fprintf(stderr, "not supported opcode\n");
            return -1;
        default:
            fprintf(stderr, "invalid opcode\n");
            return -1;
    }

    ld->iocbs[ld->tail] = &bio->iocb;
    ld->tail = (ld->tail + 1) % ld->iodepth;
    ld->nr_enqueued++;

    return 0;
}

static int libaio_submit(struct ioengine *engine)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    int res;
    u32 nr;

    while (ld->nr_enqueued) {
        nr = ld->nr_enqueued;
        if (nr > ld->iodepth - ld->head) {
            nr = ld->iodepth - ld->head;
        }

	    res = io_submit(ld->io_context, nr, &ld->iocbs[ld->head]);
        if (res > 0) {
            ld->head = (ld->head + res) % ld->iodepth;
            ld->nr_enqueued -= res;
        } else {
            fprintf(stderr, "libaio_submit() failure(%d, %s)\n", res, strerror(-res));
            return res;
        }
    }
	
    return 0;
}

static int libaio_poll(struct ioengine *engine, u32 min, u32 max)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    struct io_event *event;
    struct bio *bio;
    int nevents;
    int event_idx;
	struct timespec ts_zero = { .tv_sec = 0, .tv_nsec = 0, };
    struct timespec *pts;

    if (min) {
        pts = NULL;
    } else {
        pts = &ts_zero;
    }

	pts = NULL;
	
    nevents = io_getevents(ld->io_context, min, max, ld->io_events, pts);

    for (event_idx = 0; event_idx < nevents; event_idx++) {
        event = &ld->io_events[event_idx];
        bio = container_of(event->obj, struct bio, iocb);
        if (event->res != bio->iosize) {
            bio->result = event->res;
        } else {
            bio->result = 0;
        }
        bio->end(bio);
    }

    return nevents;
}

static int libaio_identify(struct ioengine *engine, u32 cdw10, u32 data_len, void *data)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    struct nvme_passthru_cmd cmd = {
        .opcode     = NVME_ADMIN_IDENTIFY,
        .nsid       = ld->nsid,
        .addr       = (u64)data,
        .data_len   = data_len,
        .cdw10      = cdw10,
    };

    return ioctl(ld->fd, NVME_IOCTL_ADMIN_CMD, &cmd);
}

static int libaio_get_log_page(struct ioengine *engine, u8 log_id, u32 data_len, void *data)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    struct nvme_passthru_cmd cmd = {
        .opcode     = NVME_ADMIN_GET_LOG_PAGE,
        .nsid       = ld->nsid,
        .addr       = (u64)data,
        .data_len   = data_len,
        .cdw10      = log_id | (((data_len >> 2) - 1) << 16),
    };

    return ioctl(ld->fd, NVME_IOCTL_ADMIN_CMD, &cmd);
}

static int libaio_get_features(struct ioengine *engine, u32 cdw10, u32 data_len, void *data, u32 *result)
{
    struct libaio_data *ld = (struct libaio_data *)engine->ctx;
    struct nvme_passthru_cmd cmd = {
        .opcode     = NVME_ADMIN_GET_FEATURES,
        .nsid       = ld->nsid,
        .addr       = (u64)data,
        .data_len   = data_len,
        .cdw10      = cdw10,
    };
    int res;

    res = ioctl(ld->fd, NVME_IOCTL_ADMIN_CMD, &cmd);
    if (!res && result) {
        *result = cmd.result;
    }

    return res;
}

static struct ioengine_ops libaio_engine = {
    .name = "libaio",
    .init = libaio_init,
    .exit = libaio_exit,
    .enqueue = libaio_enqueue,
    .submit = libaio_submit,
    .poll = libaio_poll,
    .identify = libaio_identify,
    .get_log_page = libaio_get_log_page,
    .get_features = libaio_get_features,
};

void __attribute__((constructor)) libaio_register(void)
{
    ioengine_register(&libaio_engine);
}

void __attribute__((destructor)) libaio_unregister(void)
{
    ioengine_unregister(&libaio_engine);
}

