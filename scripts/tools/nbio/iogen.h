
/**
 * @file iogen.h
 * @date 2016. 05. 13.
 * @author kmsun
 * 
 * i/o workload generator
 */
#ifndef __NBIO_IOGEN_H__
#define __NBIO_IOGEN_H__

#include <time.h>
#include "types.h"
#include "namespace.h"
#include "ioengine.h"
#include "params.h"
#include "rng.h"
#include "jesd219.h"
#include "acct.h"

enum iogen_job_state {
    iogen_job_state_init = 0,
    iogen_job_state_waiting,
    iogen_job_state_running,
    iogen_job_state_done,
    iogen_job_state_reset,
    iogen_job_state_aborting,
    iogen_job_state_aborted,
};

struct iogen_job_timer {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct timespec ts;
    struct iogen_job *job;
};

struct iogen_job {
    struct list_head list;
    char display_name[16];

    u32 id;
    enum iogen_job_state state;
    struct timespec begin_ts;
    struct timespec end_ts;

    u32 reset_dist;
    u32 reset_next;
    u32 nr_reset;


    struct rng rng;
    struct jesd219 jesd219;

    u64 slba_next[2];

    u32 slba_align;
    struct {
        u32 min_lbas;
        u32 max_lbas;
        u32 lbas_dist;
		u32 align;
    } lbas[iosize_num_max];
    u64 lba_range_begin;
    u64 lba_range_size;

    u32 replay_hist_idx;
    u32 replay_hist_cnt;

    struct namespace_info *ns_info;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    struct iogen_job_timer timer;
    bool timeout;
    struct job_params *params;
    struct iogen *iogen;
    u8 opr_lottery[100];

    struct ioengine engine;
    struct bio *bio_pool;
    struct list_head bio_free;
    struct list_head bio_inflight;
    u32 num_inflight;

    struct ioacct acct;

    void *identify;
    int identify_valid;

    void *priv;
};

enum iogen_state {
    iogen_state_init = 0,
    iogen_state_running,
    iogen_state_done,
    iogen_state_aborting,
    iogen_state_aborted,
};

struct iogen {
    int state;
    int result;
    struct timespec begin_ts;
    struct timespec end_ts;

    struct iogen_job *jobs;
    u32 njobs;
    struct namespace *namespaces;
    u32 nnamespaces;
    u64 running_job_mask;
    u64 finished_job_mask;
    struct ioengine_ops *engine_ops;
    struct global_params *params;
};

int iogen_init(struct iogen* iogen, struct global_params *params);
void iogen_exit(struct iogen* iogen);
int iogen_start(struct iogen* iogen);
int iogen_stop(struct iogen* iogen);
int iogen_abort(struct iogen* iogen);
int iogen_running(struct iogen* iogen);
u32 iogen_eta(struct iogen* iogen);

#endif /* __NBIO_IOGEN_H__ */

