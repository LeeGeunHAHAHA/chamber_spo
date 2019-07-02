/**
 * @file iover.h
 * @date 2016. 05. 13.
 * @author kmsun
 *
 * I/O verifier
 */
#ifndef __NBIO_IOVER_H__
#define __NBIO_IOVER_H__

#include <stdio.h>
#include <pthread.h>
#include "types.h"
#include "namespace.h"
#include "list.h"
#include "bio.h"
#include "ioengine.h"
#include "datagen.h"

struct iover_thread {
    pthread_t thread;
    struct list_head bios;
    struct iover *iover;
};

struct PACKED journal_header {
    u32 magic;
    u8  sn[20];     /* serial number for the NVM subsystem          */
    u8  eui64[8];   /* globally unique identifier for the namespace */
    u16 data_size_shift;
    u16 meta_size;
    u64 nlbs;

    u32 nhist;
    u32 hist_idx;
};

struct PACKED journal_footer {
    u8  host_writes[16];
    u64 timestamp;
    u32 rsvd;
    u32 magic;
};

struct iover_journal {
    void *base;
    struct journal_header *header;
    struct journal_footer *footer;
    int fd;
    u64 size;
};

struct iover {
    struct namespace_info *ns_info;
    struct iover_thread *threads;
    u32 nthreads;

    struct block_desc *block_desc;
    struct hist_desc *hist_desc;
    bool journal_enabled;
    struct iover_journal journal;

    ulong *lba_lock;

    u64 timestamp;
    int pagesize;
    struct datagen datagen;
};

int iover_init(struct iover *iover, struct namespace_info *ns_info);
void iover_exit(struct iover *iover);
int iover_generate_data(struct iover *iover, struct bio *bio);
int iover_submit(struct iover *iover, struct bio *bio);
/* get/set the number of host write commands to check validity of block state file */
void iover_set_host_writes(struct iover *iover, const u8 *host_writes);
void iover_get_host_writes(struct iover *iover, u8 *host_writes);

void backup_hist_state(struct iover *iover, struct bio *bio);
void clear_hist(struct iover *iover);

#endif /* __NBIO_IOVER_H__ */

