/**
 * @file rng.h
 * @date 2016. 05. 20.
 * @author kmsun
 */
#ifndef __NBIO_RNG_H__
#define __NBIO_RNG_H__

/* do not use sse instruction */
#ifdef __SSE__
#undef __SSE__
#endif

#include "types.h"
#include <math.h>
#ifdef __SSE__
#include <xmmintrin.h>
#include <memory.h>
#else
#include <stdlib.h>
#endif

struct rng {
#ifdef __SSE__
    __attribute__((aligned(16))) __m128i seed;
#else
    unsigned short seed[3];
#endif
    struct {
        double theta;
        double zeta2;
        double zetan;
        u64 n;
    } zipf;
    struct {
        double shape;
        double scale;
    } pareto;
};

#ifdef __SSE__
/*
 * random number generator using sse instruction
 * @see https://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor
 */
static inline u64 rand64(struct rng *rng)
{
    u64 res;
    __attribute__((aligned(16))) __m128i cur_seed_split;
    __attribute__((aligned(16))) __m128i multiplier;
    __attribute__((aligned(16))) __m128i adder;
    __attribute__((aligned(16))) __m128i mod_mask;
    __attribute__((aligned(16))) static const unsigned int mult[4] = { 214013, 17405, 214013, 69069 };
    __attribute__((aligned(16))) static const unsigned int gadd[4] = { 2531011, 10395331, 13737667, 1 };
    __attribute__((aligned(16))) static const unsigned int mask[4] = { 0xFFFFFFFF, 0, 0xFFFFFFFF, 0 };

    adder = _mm_load_si128((__m128i *)gadd);
    multiplier = _mm_load_si128((__m128i *)mult);
    mod_mask = _mm_load_si128((__m128i *)mask);
    cur_seed_split = _mm_shuffle_epi32(rng->seed, _MM_SHUFFLE(2, 3, 0, 1));
    rng->seed = _mm_mul_epu32(rng->seed, multiplier);
    multiplier = _mm_shuffle_epi32(multiplier, _MM_SHUFFLE(2, 3, 0, 1));
    cur_seed_split = _mm_mul_epu32(cur_seed_split, multiplier);
    rng->seed = _mm_and_si128(rng->seed, mod_mask);
    cur_seed_split = _mm_and_si128(cur_seed_split, mod_mask);
    cur_seed_split = _mm_shuffle_epi32(cur_seed_split, _MM_SHUFFLE(2, 3, 0, 1));
    rng->seed = _mm_or_si128(rng->seed, cur_seed_split);
    rng->seed = _mm_add_epi32(rng->seed, adder);

    _mm_storeu_si128((__m128i *)&res, rng->seed);

    return res;
}

static inline void rng_init(struct rng *rng, u64 seed)
{
    memset(&rng->seed, 0, sizeof (seed));
}
#else
static inline u64 rand64(struct rng *rng)
{
    unsigned int lo;
    unsigned int hi;

    lo = nrand48(rng->seed);
    hi = nrand48(rng->seed);

    return ((u64)hi << 32) | lo;
}

static inline u64 rand48(struct rng *rng)
{
    return nrand48(rng->seed);
}

static inline void rng_init(struct rng *rng, u64 seed)
{
    rng->seed[0] = seed & 0xFFFF;
    rng->seed[1] = (seed >> 16) & 0xFFFF;
    rng->seed[2] = (seed >> 32) & 0xFFFF;
}
#endif

void rng_zipf_set(struct rng *rng, u64 n, double theta);
u64 rng_zipf_int(struct rng *rng);
void rng_pareto_set(struct rng *rng, double mean, double shape);
void rng_fill(struct rng *rng, void *data, u64 nbytes);

static inline double rng_uniform_real(struct rng *rng)
{
    return (double)rand64(rng) / U64_MAX;
}

static inline u64 rng_uniform_u64(struct rng *rng, u64 ubound)
{
    return rand64(rng) % ubound;
}

static inline u64 rng_uniform_u48(struct rng *rng, u64 ubound)
{
    return rand48(rng) % ubound;
}

static inline double rng_pareto_real(struct rng *rng)
{
    return (rng->pareto.scale / pow(rng_uniform_real(rng), 1.0 / rng->pareto.shape));
}

static inline u64 rng_pareto_u64(struct rng *rng, u64 ubound)
{
    return (u64)ubound * rng_pareto_real(rng);
}

#endif /* __NBIO_RNG_H__ */

