/**
 * @file acct.h
 * @date 2016. 05. 31.
 * @author kmsun
 */
#ifndef __NBIO_ACCT_H__
#define __NBIO_ACCT_H__

#include "bio.h"

#define LATENCY_ACCT_NUM_BUCKETS        countof (latency_bucket_marks)

/* nano second unit */
static const u32 latency_bucket_marks[] = {
    /* <9us */
    1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
    /* <90us */
    10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
    /* <900us */
    100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
    /* <9ms */
    1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000,
    /* <90ms */
    10000000, 20000000, 30000000, 40000000, 50000000, 60000000, 70000000, 80000000, 90000000,
    /* <900ms */
    100000000, 200000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000,
    /* >=900ms */
    U32_MAX,
};

struct latency_acct_bucket {
    u64 count;
    u64 sum;
};

struct latency_acct {
    struct latency_acct_bucket buckets[LATENCY_ACCT_NUM_BUCKETS];
    u64 sum;
    u64 max;
    u64 min;
};

struct opacct {
    u64 ops;
    u64 bytes;
    struct latency_acct latency;
};

struct ioacct {
    struct opacct ops[bio_max];
    struct opacct sum;
};

#endif /* __NBIO_ACCT_H__ */

