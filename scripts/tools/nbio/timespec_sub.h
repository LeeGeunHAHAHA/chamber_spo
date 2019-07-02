/**
 * @file timespec_sub.h
 * @date 2016. 08. 03.
 * @author kmsun
 */
#ifndef __NBIO_TIMESPEC_SUB_H__
#define __NBIO_TIMESPEC_SUB_H__

#include <time.h>
#include <sys/time.h>

#define NSEC_PER_SEC     (1000000000ULL)

static inline u64 timespec_sub(struct timespec *ts1, struct timespec *ts2)
{
    u64 res;
    res = (ts1->tv_sec - ts2->tv_sec) * NSEC_PER_SEC;
    res += ts1->tv_nsec - ts2->tv_nsec;

    return res;
}

#endif /* __NBIO_TIMESPEC_SUB_H__ */

