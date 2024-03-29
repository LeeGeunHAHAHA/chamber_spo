/**
 * @file params.h
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * parameter definitions
 */
#ifndef __NBIO_PARAMS_H__
#define __NBIO_PARAMS_H__

#include "types.h"
#include "bio.h"

extern u32 iosize_max;
extern u32 iosize_min;

enum slba_type {
    slba_type_seq = 0,
    slba_type_uniform,
    slba_type_zipf,
    slba_type_pareto,
    slba_type_fixed,
    slba_type_jesd219,
    slba_type_replay,
};

enum amount_type {
    amount_type_infinite = 0,
    amount_type_time,
    amount_type_io,
    amount_type_byte,
};

enum unwr_read_result_type {
    unwr_read_result_type_dontcare      = 0,
    unwr_read_result_type_auto          = 1,
    unwr_read_result_type_error         = 2,
    unwr_read_result_type_value         = 3,
};

enum unwr_read_value_type {
    unwr_read_value_type_zeroes         = 0,
    unwr_read_value_type_ones           = 1,
};

enum dealloc_read_result_type {
    dealloc_read_result_type_dontcare   = 0,
    dealloc_read_result_type_auto       = 1,
    dealloc_read_result_type_error      = 2,
    dealloc_read_result_type_value      = 3,
};

enum dealloc_read_value_type {
    dealloc_read_value_type_zeroes      = 0,
    dealloc_read_value_type_ones        = 1,
    dealloc_read_value_type_last        = 2,
};

struct job_params {
    char name[64];
    char ioengine[64];
    char ioengine_param[256];
    char controller[256];
    u32 nsid;
    char journal_file[256];
    bool journal_file_mmap;
    int journal_shm;
    char dependency[64];
    int dependency_job_idx;
    u64 delay;
    bool lba_overlap;
    bool randomize_header;

    bool compare;
    bool compare_header_only;
    bool stop_on_miscompare;
    double inject_miscompare;
    bool stop_on_error;

    u32 iodepth;
    u32 batch_submit;
    u32 batch_complete;
    u32 seed;
    enum slba_type slba_type;
    union {
        struct {
            float theta;
        } zipf;
        struct {
            float scale;
            float shape;
        } pareto;
        struct {
            u64 lba;
        } fixed;
        struct {
            bool enterprise;
            u32  sector_size;
        } jesd219;
    } slba;
    u32 slba_align;
    bool separate_stream;
    bool flush_passthru;
    struct {
        u64 iosize_min;
        u64 iosize_max;
        u32 iosize_align;
    } iosizes[iosize_num_max];
    u32 operations[bio_max];
    u8  lba_range_begin_percent_enable:1;
    u8  lba_range_begin_percent:7;
    u8  lba_range_size_percent_enable:1;
    u8  lba_range_size_percent:7;
    u64 lba_range_begin;
    u64 lba_range_size;
    enum amount_type amount_type;
    u8  amount_bytes_percent_enabled:1;
    u8  amount_bytes_percent:7;
    union {
        u64 seconds;
        u64 ios;
        u64 bytes;
    } amount;
    u32 amount_scale;
    enum unwr_read_result_type unwr_read_result_type;
    enum unwr_read_value_type unwr_read_value_type;
    enum dealloc_read_result_type dealloc_read_result_type;
    enum dealloc_read_value_type dealloc_read_value_type;
    u32 reset_min;
    u32 reset_max;
    u32 num_streams;        // must be zero when stream write is not tested
    double inject_tag_error;
    u32 timeout;
    bool stop_on_timeout;
    double pract;
    u8 prchk;

    u32 history;
    struct namespace_info *ns_info;
};

struct global_params {
    char iolog[256];
    char smartlog[256];
    char errorlog[256];
    char iotrace[256];
    char powerctl_dev[256];
    u32 spor_level;
    u32 skip_ioengine_exit;

    struct job_params *job_params;
    u32 njobs;
};

#endif /* __NBIO_PARAMS_H__ */

