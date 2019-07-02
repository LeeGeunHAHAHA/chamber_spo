/**
 * @file hash.h
 * @date 2016. 05. 31.
 * @author kmsun
 */
#ifndef __NBIO_HASH_H__
#define __NBIO_HASH_H__

#define GOLDEN_RATIO_32     (0x61C88647)
#define GOLDEN_RATIO_64     (0x61C8864680B583EBULL)

static inline u64 hash64(u64 val)
{
    return val * GOLDEN_RATIO_64;
}

#endif /* __NBIO_HASH_H__ */

