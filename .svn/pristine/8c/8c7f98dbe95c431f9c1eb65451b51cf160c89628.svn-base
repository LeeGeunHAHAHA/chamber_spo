#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "mpa.h"
#include "vu_common.h"
#include "common.h"

#define VU_DATA_BUF_SIZE                (512)

#define PRECOND_LBA_RANGE               (0xC00000) //6GiB
#define PRECOND_MUN_RANGE               (PRECOND_LBA_RANGE / 8)

#define NUM_TEST_WL_GROUPS              (16)

#define READ_BIAS_MAKES_UECC            (0x44444444)

#define PATROL_READ_ROUND_TIME_DEFAULT  (60 * 60) // 1HR

typedef struct _test_wl_group {
    u32 mun;
    u32 mpa;
    u32 wl_group_no;
    u32 is_open_block;    
} test_wl_group_t;

unsigned int mpa_to_wl_group(unsigned int mpa)
{
    unsigned int page_no;
    unsigned int wl_group_offset;
    unsigned int wl_group_no;

    page_no = (mpa >> OFFSET_PAGE) & MASK_PAGE;
    wl_group_offset = page_no / (NUM_PAGES_PER_BLOCK / NUM_WL_GROUP_PER_BLOCK);

    wl_group_no = (mpa >> OFFSET_MPBLK) * NUM_WL_GROUP_PER_BLOCK + wl_group_offset;

    return wl_group_no;
}

void main (int argc, char* argv[])
{
    char cmd_str[500];
    char pattern_file[20] = "read_result";
    char* fqa_device;
    char* log_file_name;
    FILE* fp_log;

    u32 patrol_read_round_sec;

    unsigned int device_size = 0x200000;
    unsigned int block_size = 4096;
    unsigned int i;
    unsigned int *vu_data_buf;

    test_wl_group_t test_wl_group[NUM_TEST_WL_GROUPS];
    u32 test_mun_index;

    printf("%s", KNRM);
    srand(time(NULL));

    vu_data_buf = malloc(VU_DATA_BUF_SIZE);

    if (argc < 3) {
        printf("argc:%d\n", argc);
        return;
    }

    // generate dummy file for VU write    
    sprintf(cmd_str, "touch %s", pattern_file);
    system(cmd_str);

    fqa_device = argv[1];
    LOG_PRINTF("fqa_device: %s\n", fqa_device);

    patrol_read_round_sec = atoi(argv[2]);
    log_file_name = argv[3];

    fp_log = fopen(log_file_name, "a");

    if (fp_log == NULL) {
        printf("log file open failure: %s\n", log_file_name);
    }

    // initialize device
    #if 0
    sprintf(cmd_str, "nvme delete-ns %s -n 0xffffffff", fqa_device);
    system(cmd_str);

    sprintf(cmd_str, "nvme create-ns %s -s %d -c %d -f 2", fqa_device, device_size, device_size);
    system(cmd_str);

    sprintf(cmd_str, "nvme attach-ns %s -n 1 -c 0", fqa_device);
    system(cmd_str);

    sprintf(cmd_str, "rescan-ns");
    system(cmd_str);
    #endif

    // Pre-condition
    /* Pre-condition to make closed stripes(8 stirpes data write)
       8 stirpes size: 8CH * 4LUN * 2Plane * 768page * 16KB * 8 stripe = 6GB = 0x180000(4KB block count) */
    time_t write_start_time;
    u32 precondition_sec;

    LOG_PRINTF("[Pre-condition] write slba:0x%08X, length:0x%08x\n", 0, PRECOND_LBA_RANGE);

    write_start_time = time(NULL);

    system("fio SEQW_6GiB.ini > log");
    
    precondition_sec = (unsigned int)difftime(time(NULL), write_start_time);
    LOG_PRINTF("[Pre-condtion] elasped sec:%d\n", precondition_sec);
    //______________________________________________________________________________________________

    test_mun_index = 0;

    // select 10 MUNs from written range
    while (test_mun_index < NUM_TEST_WL_GROUPS) {
        u32 test_mun;
        u32 test_mpa;
        u32 test_wl_group_no;
        u32 super_block_state;

        test_mun = rand() % PRECOND_MUN_RANGE;

        // Get MPA of MUN by VU command
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                            test_mun, 0, 0, 0, vu_data_buf);
        test_mpa = vu_data_buf[0];
        test_wl_group_no = mpa_to_wl_group(test_mpa);

        // avoid selecting same WL group in test MUNs
        unsigned int is_duplicated = 0;

        for (i = 0; i < test_mun_index; i++) {
            if (test_wl_group[i].wl_group_no == test_wl_group_no) {
                is_duplicated = 1;
                break;
            }            
        }

        if (is_duplicated) {
            continue;
        }

        // Get super block state of MPA VU command for check open block
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_SPBLK_STATE, VU_DATA_BUF_SIZE,
                            test_mpa, 0, 0, 0, vu_data_buf);

        super_block_state = vu_data_buf[0];

        // register selected wl group information
        test_wl_group[test_mun_index].mun = test_mun;
        test_wl_group[test_mun_index].mpa = test_mpa;
        test_wl_group[test_mun_index].wl_group_no = test_wl_group_no;
        test_wl_group[test_mun_index].is_open_block = 0;

        u32 test_mp_blk;
        test_mp_blk = (test_mpa >> OFFSET_MPBLK) & MASK_MPBLK;

        if (super_block_state == BLOCK_LIST_IN_PFL) {
            test_wl_group[test_mun_index].is_open_block = 1;
            LOG_PRINTF("%sOpen block(mp blk:%d) is selected, block state: %d\n", KCYN, test_mp_blk, super_block_state);
        }
        else if (super_block_state == BLOCK_LIST_DATA) {
            LOG_PRINTF("%sClosed block(mp blk:%d) is selected, block state:%d\n", KGRN, test_mp_blk, super_block_state);
        }
        else {
            LOG_PRINTF("%sUnexpected block state(mp blk:%d), block state:%d\n", KMAG, test_mp_blk, super_block_state);
        }

        // Set read bias to cause UECC and pro-active ORBS
        vu_no_data(fqa_device, SUB_OPCODE_SET_READ_BIAS_TABLE, test_mpa,
                    test_wl_group_no % NUM_WL_GROUP_PER_BLOCK, READ_BIAS_MAKES_UECC, READ_BIAS_MAKES_UECC);
        
        
        test_mun_index++;
    }

    // Reset patrol read stat
    // TBD
    //______________________________________________________________________________________________

    // Update patrol read interval for validation
    vu_no_data(fqa_device, SUB_OPCODE_SET_PATROL_ROUND_TIME, patrol_read_round_sec, 0, 0, 0);
    //______________________________________________________________________________________________


    // Wait during patrol read round time
    #if 1
    LOG_PRINTF("%sNow sleeping during %d sec\n", KNRM, patrol_read_round_sec);
    fflush(stdout);
    sleep(patrol_read_round_sec);
    #else
    time_t sleep_start_time;
    sleep_start_time = time(NULL);
    u32 sleeping_sec = 0;

    printf("Now sleeping. Wake up after(sec): %03d", patrol_read_round_sec);
    fflush(stdout);

    while (sleeping_sec < patrol_read_round_sec) {
        sleep(1);
        sleeping_sec = (unsigned int)difftime(time(NULL), sleep_start_time);

        printf("\b\b\b%03d", patrol_read_round_sec - sleeping_sec);
        fflush(stdout);
    }
    printf("\n");
    #endif
    //______________________________________________________________________________________________
    
    // Get patrol read stat and print
    // TBD
    //______________________________________________________________________________________________


    // Get ORBS stat and print
    // TBD
    //______________________________________________________________________________________________

    // Check read bias of selected WL group and compare to find updated read bias
    u32 is_unexpected_bias = 0;
    test_mun_index = 0;

    while (test_mun_index < NUM_TEST_WL_GROUPS) {
        // Get super block state of MPA VU command for check open block
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_READ_BIAS_TABLE, VU_DATA_BUF_SIZE,
                            test_wl_group[test_mun_index].mpa,
                            test_wl_group[test_mun_index].wl_group_no % NUM_WL_GROUP_PER_BLOCK,
                            0, 0, vu_data_buf);

        printf("%s[Optimal read bias] SLC/CSB:0x%08X\n", KNRM, vu_data_buf[0]);
        printf("%s[Optimal read bias] LSB/MSB:0x%08X\n", KNRM, vu_data_buf[1]);

        if (test_wl_group[test_mun_index].is_open_block) {
            if ((vu_data_buf[0] != READ_BIAS_MAKES_UECC) && (vu_data_buf[1] != READ_BIAS_MAKES_UECC)) {
                LOG_PRINTF("%s[TEST FAILED] - open block read bias is updated\n", KRED);
                is_unexpected_bias = 1;
            }
        }
        else {
            if ((vu_data_buf[0] == READ_BIAS_MAKES_UECC) || (vu_data_buf[1] == READ_BIAS_MAKES_UECC)) {
                LOG_PRINTF("%s[TEST FAILED] - closed block read bias is not updated\n", KRED);
                is_unexpected_bias = 1;
            }
        }
        test_mun_index++;
    }

    if (is_unexpected_bias == 0) {
        LOG_PRINTF("%s[TEST PASSED]\n", KBLU);
    }

    // Revert temporarily changed  patrol read round time to finish valication
    vu_no_data(fqa_device, SUB_OPCODE_SET_PATROL_ROUND_TIME, PATROL_READ_ROUND_TIME_DEFAULT, 0, 0, 0);
    //______________________________________________________________________________________________

    // Reset Defense code stat 
    // TBD
    //______________________________________________________________________________________________

    // Seq.Read from precondition range
    LOG_PRINTF("%sPost Seq.R Read\n", KNRM);
    system("fio SEQR_6GiB.ini > log");
    //______________________________________________________________________________________________

    // Get Defense code stat 
    // TBD - expect nothing is udpated
    //______________________________________________________________________________________________

    free(vu_data_buf);
    fclose(fp_log);
}
