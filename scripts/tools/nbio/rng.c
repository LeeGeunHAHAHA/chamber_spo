/**
 * @file rng.c
 * @date 2016. 05. 20.
 * @author kmsun
 */

#include "rng.h"
#include <float.h>
#include <string.h>

#define ZIPF_MAX        (10000000ULL)

u64 rng_zipf_int(struct rng *rng)
{
    double u;
    double alpha;
    double eta;
    double z;

    alpha = 1.0 / (1.0 - rng->zipf.theta);
    eta = (1.0 - pow(2.0 / rng->zipf.n, 1.0 - rng->zipf.theta)) /
        (1.0 - rng->zipf.zeta2 / rng->zipf.zetan);
    u = rng_uniform_real(rng);
    z = u * rng->zipf.zetan;

    if (z < 1.0) {
        return 0;
    } else if (z < (1.0 + pow(0.5, rng->zipf.theta))) {
        return 1;
    } else {
        return (u64)(rng->zipf.n * pow(eta * u - eta + 1.0, alpha));
    }

    return 0;
}

void rng_zipf_set(struct rng *rng, u64 n, double theta)
{
    u64 i;
    double c = 0;

    rng->zipf.n = MIN(n, ZIPF_MAX);
    rng->zipf.zeta2 = pow(1.0, theta) + pow(0.5, theta);
    rng->zipf.theta = theta;

    for (i = 1; i <= rng->zipf.n; i++) {
        c += (1.0 / pow((double)i, theta));
    }

    rng->zipf.zetan = c;
}

void rng_pareto_set(struct rng *rng, double mean, double shape)
{
    double scale;

    scale = mean * (shape - 1.0) / shape;
    rng->pareto.scale = scale;
    rng->pareto.shape = shape;
}

void rng_fill(struct rng *rng, void *data, u64 nbytes)
{
    u64 remained;
    u64 tmp;
    u64 *qword_data;

    qword_data = (u64 *)data;
    remained = nbytes;
    while (remained > sizeof (u64)) {
        *qword_data = rand64(rng);
        qword_data++;
        remained -= sizeof (u64);
    }
    if (remained > 0) {
        tmp = rand64(rng);
        memcpy(qword_data, &tmp, remained);
    }
}

