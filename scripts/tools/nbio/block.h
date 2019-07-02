/**
 * @file block.h
 * @date 2016. 06. 04.
 * @author kmsun
 *
 * logical block descriptor and data header definition
 */
#ifndef __NBIO_BLOCK_H__
#define __NBIO_BLOCK_H__

#include "types.h"

#define BLOCK_HEADER_START_MARKER   (0x12)
#define BLOCK_HEADER_FINISH_MARKER  (0x34)

/* protection information */
struct PACKED pr_info {
    u16 guard;
    u16 app;
    u32 ref;
};

/*
 * block descriptor to retain
 */
struct PACKED block_desc {
    u16 timestamp;
    u8  state:3;
    u8  pattern:5;
    u8  flags;
};

struct PACKED block_header {
    u8  pattern;
    u8  start_marker;
    u64 timestamp;
    u64 lba;
    u8  rsvd;
    u8  finish_marker;
};

#define BD_CURR             0
#define BD_NEXT             1
#define NUM_BD_HISTORY      2

struct PACKED hist_desc {
    u64 lba;
    struct block_desc bd[NUM_BD_HISTORY];
};

#endif /* __NBIO_BLOCK_H__ */

