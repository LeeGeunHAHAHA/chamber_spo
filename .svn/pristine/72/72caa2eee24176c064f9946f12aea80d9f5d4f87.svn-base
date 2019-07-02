#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "mpa.h"
#include "vu_common.h"

#define VU_DATA_BUF_SIZE                (512)

#define NUM_THRESHOLD_STEPS             (5)
#define NUM_DUMMY_STEPS                 (2)

#define MAX_ERASE_COUNT                 (20000)
//#define READ_COUNT_TH_MARGIN            (30000)
#define READ_COUNT_TH_MARGIN            (300000)

#define PRECOND_LBA_RANGE               (0xC00000) //6GiB
#define PRECOND_MUN_RANGE               (PRECOND_LBA_RANGE / 8)

//#define LBA_COUNT_TO_READ               (0x100)
#define LBA_COUNT_TO_READ               (0x4)

unsigned int read_disturb_threshold_table[NUM_THRESHOLD_STEPS + NUM_DUMMY_STEPS][2] =
{   //Erase count range     Read count threshold
    {0,                     0xFFFFFFFF},    // dummy step for calculation
    {1000,                  70000},    
    {3000,                  60000},
    {5000,                  50000},
    {7000,                  40000},
    {10000,                 30000},
    {MAX_ERASE_COUNT,       0xFFFFFFFF},    // dummy step for calculation
};

void main (int argc, char* argv[])
{
    char cmd_str[500];
    char pattern_file[20] = "read_result";
    char* fqa_device;
    char* log_file_name;
    FILE* fp_log;

    unsigned int test_step_no;
    unsigned int test_erase_count;
    unsigned int test_read_threshold;

    unsigned int device_size = 0x200000;
    unsigned int block_size = 4096;
    unsigned int i;
    unsigned int *vu_data_buf;

    printf("%s", KNRM);

    vu_data_buf = malloc(VU_DATA_BUF_SIZE);

    if (argc < 4) {
        printf("argc:%d\n", argc);
        return;
    }

    // generate dummy file for VU write
    srand(time(NULL));
    sprintf(cmd_str, "touch %s", pattern_file);
    system(cmd_str);

    fqa_device = argv[1];
    test_step_no = atoi(argv[2]);
    log_file_name = argv[3];

    fp_log = fopen(log_file_name, "a");

    if (fp_log == NULL) {
        printf("log file open failure: %s\n", log_file_name);
    }

    // randomly select erase count
    unsigned int erase_count_range;
    unsigned int erase_count_offset;
    
    erase_count_range = read_disturb_threshold_table[test_step_no][0]
                        - read_disturb_threshold_table[test_step_no - 1][0];
    erase_count_offset = rand() % erase_count_range;

    test_erase_count = read_disturb_threshold_table[test_step_no - 1][0] + erase_count_offset;
    test_read_threshold = read_disturb_threshold_table[test_step_no][1];

    LOG_PRINTF("fqa_device: %s\n", fqa_device);
    LOG_PRINTF("test_step_no: %d\n", test_step_no);
    LOG_PRINTF("test_erase_count: %d\n", test_erase_count);
    LOG_PRINTF("test_read_threshold: %d\n", test_read_threshold);

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

    // Set threshold table
    unsigned int step_no;
    for (step_no = 1; step_no <= NUM_THRESHOLD_STEPS; step_no++) {
        unsigned int erase_count_range;
        unsigned int read_count_th;

        erase_count_range = read_disturb_threshold_table[step_no][0];
        read_count_th = read_disturb_threshold_table[step_no][1];

        LOG_PRINTF("[VU command] Set threshold step:%d, erase cnt:%d, read count:%d\n",
                 (step_no - 1), erase_count_range, read_count_th);

        vu_data_to_host(fqa_device, SUB_OPCODE_SET_READ_DISTURB_TH, VU_DATA_BUF_SIZE,
                            (step_no - 1), erase_count_range, read_count_th, 0, vu_data_buf);
    }
    //______________________________________________________________________________________________

    /* write data
       Pre-condition to make closed stripes
       8 stirpes data write
       8 stirpes size: 8CH * 4LUN * 2Plane * 768page * 16KB * 8 stripe
               	           = 6GB = 0x180000(4KB block count) */

    unsigned int start_lba = 0;
    system("fio SEQW_6GiB.ini > log");
    //______________________________________________________________________________________________

    /* randomly seletect programmed lba*/
    unsigned int test_mun;
    test_mun = rand() % PRECOND_MUN_RANGE;
    LOG_PRINTF("test mun: 0x%08x\n", test_mun);
    unsigned int test_lba;

    test_lba = test_mun * 8;
    //______________________________________________________________________________________________

    unsigned int mpa_origin;
    unsigned int mpa_checking;
    unsigned int mpa_ch;
    unsigned int mpa_lun;
    unsigned int mpa_mpblk;	
    unsigned int read_count_updated = 0xFFFFFFFF;
    unsigned int read_count_prev;

    // Get MPA of MUN by VU command
    vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                        test_mun, 0, 0, 0, vu_data_buf);
    mpa_origin = vu_data_buf[0];
    LOG_PRINTF("[VU command] Get MPA of LBA mpa_origin : 0x%08X\n", mpa_origin);
    LOG_PRINT_MPA(mpa_origin, fp_log);
    //______________________________________________________________________________________________

    // Set Erase count of testing super block which include MPA by VU command
    LOG_PRINTF("[VU command] Set erase count of testing super block:%d\n", test_erase_count);

    vu_no_data(fqa_device, SUB_OPCODE_SET_SPBLK_EC, mpa_origin, test_erase_count, 0, 0);
    //______________________________________________________________________________________________

    while (1) {
        #if 1
        for (i = 0; i < 1000; i++) {
            // Seq read
            sprintf(cmd_str, "nvme read %s -s %d -c %d -z %d -d %s 2> log ",
                    fqa_device, test_lba, LBA_COUNT_TO_READ - 1, block_size, pattern_file);

            system(cmd_str);
        }
        #else
        system("fio SEQR_6GiB.ini > log");
        #endif
        //__________________________________________________________________________________________

        // Get Read count of testing physical block which include MPA by VU command
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_READ_COUNT, VU_DATA_BUF_SIZE,
                            mpa_origin, 0, 0, 0, vu_data_buf);
        read_count_updated = vu_data_buf[0];
        //__________________________________________________________________________________________

        // Get MPA of MUN by VU command
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                            test_mun, 0, 0, 0, vu_data_buf);
        mpa_checking = vu_data_buf[0];
        mpa_ch = (mpa_checking >> OFFSET_CH) & MASK_CH;
        mpa_lun = (mpa_checking >> OFFSET_LUN) & MASK_LUN;
        mpa_mpblk = (mpa_checking >> OFFSET_MPBLK) & MASK_MPBLK;
        //__________________________________________________________________________________________

        LOG_PRINTF("%sread th:%d, read cnt:%d, mpa:0x%08X ch:%d lun:%d mp_blk:%d\n",
                 KNRM, test_read_threshold, read_count_updated,
                 mpa_checking, mpa_ch, mpa_lun, mpa_mpblk);

        if (read_count_updated > (test_read_threshold + READ_COUNT_TH_MARGIN)) {
            if (mpa_origin != mpa_checking) {
                LOG_PRINTF("%s[TEST FAILED] - threshold over\n", KRED);
                //while(1);
            }
            else {
                LOG_PRINTF("%s[TEST FAILED] - monitoring block reclaim time\n", KRED);
            }
        }

        if (read_count_prev == read_count_updated) {
            if (mpa_origin != mpa_checking) {
                if (read_count_updated < test_read_threshold) {
                    LOG_PRINTF("%s[TEST FAILED] - reclaimed too early\n", KRED);
                    //while(1);
                }
                else {
                    LOG_PRINTF("%s[TEST PASSED]\n", KBLU);
                }
                return;
            }
        }

        read_count_prev = read_count_updated;
    }

    free(vu_data_buf);
    fclose(fp_log);
}
