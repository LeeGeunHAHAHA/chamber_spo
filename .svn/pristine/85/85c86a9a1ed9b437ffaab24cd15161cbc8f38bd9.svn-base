/**
 * @file bio.h
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * block i/o structure definition
 */
#ifndef __NBIO_BIO_H__
#define __NBIO_BIO_H__

#include <sys/time.h>
#include <libaio.h>
#include "list.h"
#include "types.h"
#include "block.h"

enum bio_opcode {
    bio_read = 0,
    bio_write,
    bio_flush,
    bio_deallocate,
    bio_write_zeroes,
    bio_write_uncorrect,
    bio_identify,
    bio_fw_download,
    bio_max,
};

enum iosize_opcode {
    iosize_total = 0,
    iosize_read,
    iosize_write,
    iosize_deallocate,
    iosize_write_zeroes,
    iosize_write_uncorrect,
    iosize_num_max,
};
struct iogen_job;
struct bio {
    struct list_head list;

    u16 id;
    struct iogen_job *job;

    struct block_desc *block_desc;
    void *ctx;

    u32 opcode;
    u32 flags;
    u64 slba;
    u32 nlbs;
    u64 iosize;     /* nlbs << ds */
    u64 offset;     /* slba << ds */
    u64 data_size;  /* data + metadata(extended lba) size */
    void *data_buf;
    void *data;
    u64 meta_size;  /* metadata(separate buffer) size */
    void *meta;     /* metadata(separate buffer) */

    u16 stream_id;
    int result;
    int expected_result;

    u8 pract;
    u8 prchk;
    u16 apptag;
    u32 reftag;

    struct timespec begin_ts;
    struct timespec end_ts;
    bool timeout;
    void (*end)(struct bio *);

    union {
        struct iocb iocb;   /* libaio */
        struct {            /* spdk */
            u32 sgl_offset;
        } spdk;
    };

    u32 hist_idx;
};

#endif /* __NBIO_BIO_H__ */

