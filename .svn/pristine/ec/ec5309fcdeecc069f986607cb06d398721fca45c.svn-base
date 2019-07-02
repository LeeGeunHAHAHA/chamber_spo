/**
 * @file bit.h
 * @date 2016. 05. 23.
 * @author 
 */
#ifndef __NBIO_BIT_H__
#define __NBIO_BIT_H__

#define BITS_PER_BYTE       8
#define BITS_PER_LONG       (sizeof(ulong) * BITS_PER_BYTE)
#define BIT_MASK(nr)        (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)
#define BITS_TO_LONGS(nr)   DIV_ROUND_UP(nr, BITS_PER_LONG)

#define popcount(x)         __builtin_popcountl(x)
#define popcount64(x)       __builtin_popcountll(x)
#define ctz(x)              __builtin_ctzl(x)

#define GET_MASK32(offset, width)   \
    ((U32_MAX << offset) & (U32_MAX >> ((32 - offset) - width)))
#define GET_MASK64(offset, width)   \
    ((U64_MAX << offset) & (U64_MAX >> ((64 - offset) - width)))

#endif /* __NBIO_BIT_H__ */

