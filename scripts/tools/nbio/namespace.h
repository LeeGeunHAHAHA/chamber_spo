/**
 * @file namespace.h
 * @date 2016. 05. 21.
 * @author kmsun
 *
 * per-namespace data structure definition
 */
#ifndef __NBIO_NAMESPACE_H__
#define __NBIO_NAMESPACE_H__

#include "types.h"
#include "bio.h"
#include "ioengine.h"

struct namespace_info {
    u8  eui64[8];
    u8  nguid[16];
    u8  uuid[16];
    struct iover *iover;
    struct ioengine engine;
    const char *journal_file;
    bool journal_file_mmap;
    int journal_shm;
    bool randomize_header;

    u32 data_size;              /* lba data size in bytes           */
    u32 meta_size;              /* meta-data size in bytes          */
    u32 block_size;             /* lba data + meta-data(extended lba) size in bytes */
    int extended_lba;           /* meta-data is transferred as a separate buffer */
    u64 nlbs;                   /* number of logical blocks         */
    u32 lba_shift;
    u32 command_supported;      /* present supported bio operations */

    u8  pi_type;                /* 0: protection information is not enabled, 1-3: type 1/2/3 */
    u8  pi_loc;                 /* 0: the last 8 bytes of the metadata, 1: the first 8 bytes of the metadata */

    u32 nhist;
};

#endif /* __NBIO_NAMESPACE_H__ */

