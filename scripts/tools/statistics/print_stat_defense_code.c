#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "vu_common.h"
#include "statistics_struct.h"

#define VU_DATA_BUF_SIZE                (24 * 1024)

void main (int argc, char* argv[])
{
    char cmd_str[500];
    char* fqa_device;
    char* log_file_name;
    FILE* fp_log;

    u32* p_vu_data_buf;

    printf("%s", KNRM);

    p_vu_data_buf = malloc(VU_DATA_BUF_SIZE);

    if (argc < 2) {
        printf("argc:%d\n", argc);
        return;
    }

    fqa_device = argv[1];

    log_file_name = argv[2];

    fp_log = fopen(log_file_name, "a");

    if (fp_log == NULL) {
        printf("log file open failure: %s\n", log_file_name);
        return;
    }

    // Get stat defense code
    vu_data_to_host(fqa_device, SUB_OPCODE_GET_STAT_DEFENSE_CODE, VU_DATA_BUF_SIZE,
                    0, 0, 0, 0, p_vu_data_buf);
    //______________________________________________________________________________________________

    // Print Statistics
    u32 channel;
    u32 lun;
    u32 chk;
    u32 idx;
    dc_stat_t* p_dc_stat;
    p_dc_stat = (dc_stat_t*)p_vu_data_buf;

    LOG("%s## Defense code statistic size:%d Byte\n", KGRN, (u32)sizeof(dc_stat_t));

    // RRT Statistics
    read_ctx_stat_t* p_stat;
    read_ctx_lun_stat_t* p_rrt_lun_stat;

    p_stat = &p_dc_stat->read_ctx_stat;

    LOG("%s================== Read Recovery RRT ==================\n", KCYN);
    LOG("%s------------------ TLC LUN STAT Closed Block  ----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
        for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
            p_rrt_lun_stat = &p_stat->tlc_lun_stat[read_ctx_stat_idx_closed_blk][channel][lun];
            if (p_rrt_lun_stat->max_rrt_ctx > 0) {
              LOG("%s[%2d][%2d] max OIO ctx:%lld recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                      channel, lun,
                      p_rrt_lun_stat->max_rrt_ctx,
                      p_rrt_lun_stat->num_failure,
                      p_rrt_lun_stat->num_ctx_alloc,
                      p_rrt_lun_stat->num_ctx_free);
            }
        }
    }
    LOG("%s------------------ SLC LUN STAT Closed Block ----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
      for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
          p_rrt_lun_stat = &p_stat->slc_lun_stat[read_ctx_stat_idx_closed_blk][channel][lun];
          if (p_rrt_lun_stat->max_rrt_ctx > 0) {
              LOG("%s[%2d][%2d] max OIO ctx:%lld recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                      channel, lun,
                      p_rrt_lun_stat->max_rrt_ctx,
                      p_rrt_lun_stat->num_failure,
                      p_rrt_lun_stat->num_ctx_alloc,
                      p_rrt_lun_stat->num_ctx_free);
          }
      }
    }
    LOG("%s-------------------------------------------------------\n", KBLU);
    LOG("%sCTX allocated(Closed Block) :%lld\n", KWHT, p_stat->num_ctx_alloc[read_ctx_stat_idx_closed_blk]);
    LOG("%sCTX free(Closed Block)      :%lld\n", KWHT, p_stat->num_ctx_free[read_ctx_stat_idx_closed_blk]);

    LOG("%s------------------ TLC LUN STAT Open Block  ----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
      for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
          p_rrt_lun_stat = &p_stat->tlc_lun_stat[read_ctx_stat_idx_open_blk][channel][lun];
          if (p_rrt_lun_stat->max_rrt_ctx > 0) {
              LOG("%s[%2d][%2d] max OIO ctx:%lld recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                      channel, lun,
                      p_rrt_lun_stat->max_rrt_ctx,
                      p_rrt_lun_stat->num_failure,
                      p_rrt_lun_stat->num_ctx_alloc,
                      p_rrt_lun_stat->num_ctx_free);
          }
      }
    }

    LOG("%s------------------ SLC LUN STAT Open Block ----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
      for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
          p_rrt_lun_stat = &p_stat->slc_lun_stat[read_ctx_stat_idx_open_blk][channel][lun];
          if (p_rrt_lun_stat->max_rrt_ctx > 0) {
              LOG("%s[%2d][%2d] max OIO ctx:%lld recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                      channel, lun,
                      p_rrt_lun_stat->max_rrt_ctx,
                      p_rrt_lun_stat->num_failure,
                      p_rrt_lun_stat->num_ctx_alloc,
                      p_rrt_lun_stat->num_ctx_free);
          }
      }
    }
    LOG("%s-------------------------------------------------------\n", KBLU);
    LOG("%sCTX allocated(Open Block) :%lld\n", KWHT, p_stat->num_ctx_alloc[read_ctx_stat_idx_open_blk]);
    LOG("%sCTX free(Open Block)      :%lld\n", KWHT,p_stat->num_ctx_free[read_ctx_stat_idx_open_blk]);

    // ORBS Statistics
    orbs_stat_t* p_orbs_stat;
    orbs_lun_stat_t* p_orbs_lun_stat;
    u64 recovery_orbs_failure;
    u64 ftl_orbs_failure;

    p_orbs_stat = &p_dc_stat->orbs_stat;
    recovery_orbs_failure = p_orbs_stat->num_ctx_alloc_recovery_orbs;
    ftl_orbs_failure = p_orbs_stat->num_ctx_alloc_ftl_orbs;

    LOG("%s====================== ORBS STAT ======================\n", KCYN);
    LOG("%s----------------- Read Recovery ORBS ------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
        for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
            p_orbs_lun_stat = &p_orbs_stat->lun_stat[channel][lun];
            if (p_orbs_lun_stat->num_ctx_alloc_recovery_orbs > 0) {
                LOG("%s[%2d][%2d] recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                        channel, lun,
                        p_orbs_lun_stat->num_read_failure,
                        p_orbs_lun_stat->num_ctx_alloc_recovery_orbs,
                        p_orbs_lun_stat->num_ctx_free_recovery_orbs);
            }
        }
    }
    LOG("%sCTX allocated   :%lld\n", KWHT, p_orbs_stat->num_ctx_alloc_recovery_orbs);
    LOG("%sCTX free        :%lld\n", KWHT, p_orbs_stat->num_ctx_free_recovery_orbs);
    LOG("%sCTX open string :%lld\n", KWHT, p_orbs_stat->num_ctx_open_string_recovery_orbs);
    for (idx = 0; idx < 3/*GM_MAX_RUN*/; idx++) {
        LOG("%stry count(%d)    :%lld\n", KWHT, idx+1, p_orbs_stat->recovery_orbs_try_cnt[idx]);
        recovery_orbs_failure -= p_orbs_stat->recovery_orbs_try_cnt[idx];
    }
    LOG("%sSearch failure  :  %lld/%lld\n", KWHT, recovery_orbs_failure, p_orbs_stat->orbs_search_failure);
    LOG("%s---------------------- FTL ORBS -----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
        for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
            p_orbs_lun_stat = &p_orbs_stat->lun_stat[channel][lun];
            if (p_orbs_lun_stat->num_ctx_alloc_ftl_orbs > 0) {
                LOG("%s[%2d][%2d] test read failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                        channel, lun,
                        p_orbs_lun_stat->num_test_read_failure,
                        p_orbs_lun_stat->num_ctx_alloc_ftl_orbs,
                        p_orbs_lun_stat->num_ctx_free_ftl_orbs);
            }
        }
    }
    LOG("%sCTX allocated  :%lld\n", KWHT, p_orbs_stat->num_ctx_alloc_ftl_orbs);
    LOG("%sCTX free       :%lld\n", KWHT, p_orbs_stat->num_ctx_free_ftl_orbs);
    LOG("%sCTX open block :%lld\n", KWHT, p_orbs_stat->num_ctx_open_blk_ftl_orbs);
    for (idx = 0; idx < 3/*GM_MAX_RUN*/; idx++) {
        LOG("%stry count(%d) :  %lld\n",  KWHT, idx+1, p_orbs_stat->ftl_orbs_try_cnt[idx]);
        ftl_orbs_failure -= p_orbs_stat->ftl_orbs_try_cnt[idx];
    }
    LOG("%sSearch failure  :  %lld/%lld\n", KWHT, ftl_orbs_failure, p_orbs_stat->orbs_search_failure);

    LOG("%s-------------------- ORBS BF STAT ---------------------\n", KBLU);
    LOG("%sCTX BF decoding   : %lld\n", KWHT, p_orbs_stat->num_decoder_type[0]);
    LOG("%sMaximum iteration : %d\n", KWHT, p_orbs_stat->max_iter[0]);
    LOG("%sAverage iteration : %d.%02d\n", KWHT, p_orbs_stat->avg_iterx100[0] / 100,
                                       p_orbs_stat->avg_iterx100[0] % 100);
    LOG("%s--------------- ORBS Min-sum hard STAT ----------------\n", KBLU);
    LOG("%sCTX MSH decoding  : %lld\n", KWHT, p_orbs_stat->num_decoder_type[1]);
    LOG("%sMaximum iteration : %d\n", KWHT, p_orbs_stat->max_iter[1]);
    LOG("%sAverage iteration : %d.%02d\n", KWHT, p_orbs_stat->avg_iterx100[1] / 100,
                                       p_orbs_stat->avg_iterx100[1] % 100);

    // Softdecode Statistics
    sdecode_stat_t* p_sdecode_stat;
    sdecode_lun_stat_t* p_sdecode_lun_stat;

    p_sdecode_stat = &p_dc_stat->sdecode_stat;
    LOG("%s============= Read Recovery Soft-decoding =============\n", KCYN);
    LOG("%s------------------ TLC LUN STAT  ----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
       for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
           p_sdecode_lun_stat = &p_sdecode_stat->tlc_lun_stat[channel][lun];
           if (p_sdecode_lun_stat->num_ctx_alloc > 0) {
               LOG("%s[%2d][%2d] recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                                                       channel, lun,
                                                       p_sdecode_lun_stat->num_failure,
                                                       p_sdecode_lun_stat->num_ctx_alloc,
                                                       p_sdecode_lun_stat->num_ctx_free);
           }
       }
    }

    LOG("%s------------------ SLC LUN STAT  ----------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
       for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
           p_sdecode_lun_stat = &p_sdecode_stat->slc_lun_stat[channel][lun];
           if (p_sdecode_lun_stat->num_ctx_alloc > 0) {
               LOG("%s[%2d][%2d] recovery failure cnt:%lld (allocated:%lld free:%lld)\n", KWHT,
                                                       channel, lun,
                                                       p_sdecode_lun_stat->num_failure,
                                                       p_sdecode_lun_stat->num_ctx_alloc,
                                                       p_sdecode_lun_stat->num_ctx_free);
           }
       }
    }
    LOG("%s-------------------------------------------------------\n", KBLU);
    LOG("%sCTX allocated  :%lld\n", KWHT, p_sdecode_stat->num_ctx_alloc);
    LOG("%sCTX free       :%lld\n", KWHT, p_sdecode_stat->num_ctx_free);
    LOG("%sCTX open string:%lld\n", KWHT, p_sdecode_stat->num_ctx_open_string);
    LOG("%srecovery chunk :%lld\n", KWHT, p_sdecode_stat->num_recv_success_cpl);

    for (chk = 0; chk < __MAX_NUM_4K_CHUNK_PER_PAGE; chk++) {
        LOG("%salloc chunk %d  :%lld\n", KWHT, chk, p_sdecode_stat->num_ctx_alloc_per_chk[chk]);
    }
    for (idx = 0; idx < 26/*MAX_LLR_CHANGE_STEP*/; idx++) {
        if (p_sdecode_stat->llrt_change_step[idx] > 0) {
            LOG("%sLLRT step(%2d)  :%d\n", KWHT, idx+1, p_sdecode_stat->llrt_change_step[idx]);
        }
    }

    // Parity Statistics
    parity_stat_t* p_parity_stat;
    parity_lun_stat_t* p_parity_lun_stat;

    p_parity_stat = &p_dc_stat->parity_stat;

    LOG("%s================= Read Recovery PARITY ================\n", KCYN);
    LOG("%s------------------- TLC LUN STAT  ---------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
        for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
            p_parity_lun_stat = &p_parity_stat->tlc_lun_stat[channel][lun];
            if (p_parity_lun_stat->num_ctx_alloc > 0) {
                LOG("%s[%2d][%2d] num_success :%d num_failure:%d num_alloc:%d num_free:%d num_open_stripe_success:%d num_open_stripe_failure:%d\n", KWHT,
                    channel, lun,
                    p_parity_lun_stat->num_success,
                    p_parity_lun_stat->num_failure,
                    p_parity_lun_stat->num_ctx_alloc,
                    p_parity_lun_stat->num_ctx_free,
                    p_parity_lun_stat->num_open_success,
                    p_parity_lun_stat->num_open_failure);
            }
        }
    }

    LOG("%s------------------- SLC LUN STAT  ---------------------\n", KBLU);
    for (channel = 0; channel < __MAX_NUM_FLASH_CHANNEL; channel++) {
        for (lun = 0; lun < __MAX_NUM_LUN_PER_CHANNEL; lun++) {
            p_parity_lun_stat = &p_parity_stat->slc_lun_stat[channel][lun];
            if (p_parity_lun_stat->num_ctx_alloc > 0) {
                LOG("%s[%2d][%2d] num_success :%d num_failure:%d num_alloc:%d num_free:%d num_open_stripe_success:%d num_open_stripe_failure:%d\n", KWHT,
                    channel, lun,
                    p_parity_lun_stat->num_success,
                    p_parity_lun_stat->num_failure,
                    p_parity_lun_stat->num_ctx_alloc,
                    p_parity_lun_stat->num_ctx_free,
                    p_parity_lun_stat->num_open_success,
                    p_parity_lun_stat->num_open_failure);
            }
        }
    }
    LOG("%s-------------------------------------------------------\n", KBLU);
    LOG("%sCTX allocated :%lld\n", KWHT, p_parity_stat->num_ctx_alloc);
    LOG("%sCTX free      :%lld\n", KWHT, p_parity_stat->num_ctx_free);

    LOG("%s=======================================================\n", KCYN);
    LOG("%sRecovery give up count Closed Block :%lld\n", KWHT, p_stat->final_give_up_cnt[read_ctx_stat_idx_closed_blk]);
    LOG("%sRecovery give up count Open Block   :%lld\n", KWHT, p_stat->final_give_up_cnt[read_ctx_stat_idx_open_blk]);

    
    LOG("%s\n", KNRM);    
    //______________________________________________________________________________________________

    free(p_vu_data_buf);
    fclose(fp_log);
}
