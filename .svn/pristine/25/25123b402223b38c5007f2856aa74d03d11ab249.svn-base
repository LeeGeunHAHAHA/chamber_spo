/**
 * @file stat_struct.h
 * @date 2018. 10. 31.
 * @author
 */
#ifndef __STATISTICS_STRUCT_H__
#define __STATISTICS_STRUCT_H__

#define STAT_STRUCT_VER             (0)
#define __MAX_NUM_FLASH_CHANNEL     (8)
#define __MAX_NUM_LUN_PER_CHANNEL   (8)
#define __MAX_NUM_4K_CHUNK_PER_PAGE (4)

// TODO. removed example stat structure

// module stat: admin command
typedef struct __admin_cmd_stat {
    u32 example_count;
    u32 reserved[127];
} admin_cmd_stat_t;

// module stat: block manager
typedef struct __block_manager_stat {
    u32 example_stat;
    u32 reserved[127];
} block_manager_stat_t;

typedef struct _parity_lun_stat {
    u32 num_ctx_alloc;
    u32 num_ctx_free;
    u32 num_success;
    u32 num_failure;
    u32 num_open_success;
    u32 num_open_failure;
} parity_lun_stat_t;

typedef struct _parity_stat {
    u64 num_ctx_alloc;
    u64 num_ctx_free;
    parity_lun_stat_t tlc_lun_stat[__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
    parity_lun_stat_t slc_lun_stat[__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
} parity_stat_t;


typedef struct _orbs_lun_stat {
    u64 num_ctx_alloc_ftl_orbs;
    u64 num_ctx_free_ftl_orbs;
    u64 num_ctx_alloc_recovery_orbs;
    u64 num_ctx_free_recovery_orbs;
    /*
     * ORBS recovery failure count
     */
    u64 num_read_failure;

    /*
     * ftl orbs test read failure count
     */
    u64 num_test_read_failure;
} orbs_lun_stat_t;


typedef struct _orbs_stat {
    u64 num_ctx_alloc_ftl_orbs;
    u64 num_ctx_free_ftl_orbs;
    u64 num_ctx_open_blk_ftl_orbs;

    u64 num_ctx_alloc_recovery_orbs;
    u64 num_ctx_free_recovery_orbs;
    u64 num_ctx_open_string_recovery_orbs;

    /*
     * decoder type of recovery success contexts
     * 0 : bit-flip decoder
     * 1 : min-sum hard decoder
     */
    u64 num_decoder_type[2];
    u8 max_iter[2];
    u8 avg_iterx100[2];
    u32 rsvd;

    /* GM_MAX_RUN = 3 */
    u64 ftl_orbs_try_cnt[3];
    u64 recovery_orbs_try_cnt[3];

    u64 orbs_search_failure;
    orbs_lun_stat_t lun_stat[__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
} orbs_stat_t;


typedef struct _sdecode_lun_stat {
    u64 num_ctx_alloc;
    u64 num_ctx_free;
    /*
     * Soft decoding recovery failure count
     */
    u64 num_failure;
} sdecode_lun_stat_t;

typedef struct _sdecode_stat {
    u64 num_ctx_alloc;
    u64 num_ctx_free;
    u64 num_recv_success_cpl;
    u64 num_ctx_alloc_per_chk[__MAX_NUM_4K_CHUNK_PER_PAGE];
    u64 num_ctx_open_string;

    /*
     * min-sum soft decoding only
     */
    u8 max_iter;
    u8 avg_iterx100;
    u16 rsvd0;
    u32 rsvd1;

    u32 llrt_mag[5];
    u32 llrt_change_step[26];

    sdecode_lun_stat_t tlc_lun_stat[__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
    sdecode_lun_stat_t slc_lun_stat[__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
} sdecode_stat_t;


typedef struct _read_ctx_lun_stat {
    u64 num_ctx_alloc;
    u64 num_ctx_free;
    u64 num_failure;
    u64 max_rrt_ctx;
} read_ctx_lun_stat_t;

enum{
    read_ctx_stat_idx_closed_blk = 0,
    read_ctx_stat_idx_open_blk   = 1,
    read_cxt_stat_idx_count      = 2,
};

typedef struct _read_ctx_stat {
    u64 num_ctx_alloc[read_cxt_stat_idx_count];
    u64 num_ctx_free[read_cxt_stat_idx_count];
    u64 final_give_up_cnt[read_cxt_stat_idx_count];
    read_ctx_lun_stat_t tlc_lun_stat[read_cxt_stat_idx_count][__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
    read_ctx_lun_stat_t slc_lun_stat[read_cxt_stat_idx_count][__MAX_NUM_FLASH_CHANNEL][__MAX_NUM_LUN_PER_CHANNEL];
} read_ctx_stat_t;


// module stat: defense code
typedef struct __dc_stat {
    read_ctx_stat_t read_ctx_stat;
    orbs_stat_t     orbs_stat;
    sdecode_stat_t  sdecode_stat;
    parity_stat_t   parity_stat;
} dc_stat_t;


// core level statistics
typedef struct __core_pmu_stat {
    u32 stat_version;
    admin_cmd_stat_t admin_cmd_stat;
} core_pmu_stat_t;

typedef struct __core_pss0_stat {
    u32 stat_version;
    block_manager_stat_t bm_stat;
} core_pss0_stat_t;

typedef struct __core_pss1_stat {
    u32 stat_version;
    dc_stat_t dc_stat;
} core_pss1_stat_t;

#endif /* __STATISTICS_STRUCT_H__ */

