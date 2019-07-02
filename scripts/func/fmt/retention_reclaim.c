#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "mpa.h"
#include "vu_common.h"

#define VU_DATA_BUF_SIZE                (512)

#define NUM_THRESHOLD_STEPS             (5)
#define NUM_DUMMY_STEPS                 (2)

#define MAX_ERASE_COUNT                 (20000)

#define CHECK_PERIOD                    (2)
#define RECLAIM_SEC_MARGIN              (10)

#define PRECOND_LBA_RANGE               (0xC00000) //6GiB
#define PRECOND_MUN_RANGE               (PRECOND_LBA_RANGE / 8)

unsigned int retention_threshold_table[NUM_THRESHOLD_STEPS + NUM_DUMMY_STEPS][2] =
{   //Erase count range     Read count threshold
    {0,                     0xFFFFFFFF},    // dummy step for calculation
    {4000,                  120},           // 3 minutes,
    {5000,                  150},           // 3.5 minutes,
    {6000,                  180},           // 4 minutes,
    {7000,                  210},           // 4.5 minutes,
    {8000,                  240},           // 5 minutes,
    {MAX_ERASE_COUNT,       0xFFFFFFFF},    // dummy step for calculation
};

unsigned int retention_threshold_table_default[NUM_THRESHOLD_STEPS + NUM_DUMMY_STEPS][2] =
{   //Erase count range     Read count threshold
    {0,                     0xFFFFFFFF},    // dummy step for calculation
    {4000,                  12000},           // 3 minutes,
    {5000,                  15000},           // 3.5 minutes,
    {6000,                  18000},           // 4 minutes,
    {7000,                  21000},           // 4.5 minutes,
    {8000,                  24000},           // 5 minutes,
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
    unsigned int test_retention_time;

    unsigned int device_size = 0x200000;
    unsigned int block_size = 4096;
    unsigned int i;
    unsigned int *vu_data_buf;

    printf("%s", KNRM);

    vu_data_buf = malloc(VU_DATA_BUF_SIZE);

    if (argc < 3) {
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
    
    erase_count_range = retention_threshold_table[test_step_no][0]
                        - retention_threshold_table[test_step_no - 1][0];
    erase_count_offset = rand() % erase_count_range;

    test_erase_count = retention_threshold_table[test_step_no - 1][0] + erase_count_offset;
    test_retention_time = retention_threshold_table[test_step_no][1];

    LOG_PRINTF("fqa_device: %s\n", fqa_device);
    LOG_PRINTF("test_step_no: %d\n", test_step_no);
    LOG_PRINTF("test_erase_count: %d\n", test_erase_count);
    LOG_PRINTF("test_retention_time(sec): %d\n", test_retention_time);

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

    /* write data
       Pre-condition to make closed stripes
       8 stirpes data write
       8 stirpes size: 8CH * 4LUN * 2Plane * 768page * 16KB * 8 stripe
               	           = 6GB = 0x180000(4KB block count) */
    unsigned int start_lba;
    unsigned int lba_count;
    time_t write_start_time;
    unsigned int precondition_sec;

    start_lba = 0;
    lba_count = PRECOND_LBA_RANGE;

    LOG_PRINTF("[Pre-condition] write slba:0x%08X, length:0x%08x\n", start_lba, lba_count);

    write_start_time = time(NULL);

    system("fio SEQW_6GiB.ini > log");

    precondition_sec = (unsigned int)difftime(time(NULL), write_start_time);
    LOG_PRINTF("[Pre-condtion] elasped sec for margin:%d\n", precondition_sec);

    // Set threshold table
    unsigned int step_no;
    for (step_no = 1; step_no <= NUM_THRESHOLD_STEPS; step_no++) {
        unsigned int erase_count_range;
        unsigned int retention_time_th;

        erase_count_range = retention_threshold_table[step_no][0];
        retention_time_th = retention_threshold_table[step_no][1];

        LOG_PRINTF("[VU command] Set threshold step:%d, erase cnt:%d, read count:%d\n",
                 (step_no - 1), erase_count_range, retention_time_th);

        vu_data_to_host(fqa_device, SUB_OPCODE_SET_RETENTION_TH, VU_DATA_BUF_SIZE,
                            (step_no - 1), erase_count_range, retention_time_th, 0, vu_data_buf);
    }
    //______________________________________________________________________________________________

    /* randomly seletect programmed mun*/
    unsigned int test_mun;

    while (1) {
        test_mun = rand() % PRECOND_MUN_RANGE;

        if (test_mun > (PRECOND_MUN_RANGE / 8)) {
            break;
        }
    }
    LOG_PRINTF("test mun: 0x%08x\n", test_mun);

    unsigned int mpa_origin;
    unsigned int mpa_checking;
    unsigned int mpa_ch;
    unsigned int mpa_lun;
    unsigned int mpa_mpblk;	

    // Get MPA of MUN by VU command
    vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                        test_mun, 0, 0, 0, vu_data_buf);
    mpa_origin = vu_data_buf[0];
    LOG_PRINTF("[VU command] Get MPA of LBA mpa_origin : 0x%08X\n", mpa_origin);
    //______________________________________________________________________________________________

    // Set Erase count of testing super block which include MPA by VU command
    LOG_PRINTF("[VU command] Set erase count of testing super block:%d\n", test_erase_count);

    vu_no_data(fqa_device, SUB_OPCODE_SET_SPBLK_EC, mpa_origin, test_erase_count, 0, 0);
    //______________________________________________________________________________________________

    time_t test_start_time;
    unsigned int elapsed_sec = 0;

    test_start_time = time(NULL);

    while (1) {
        sleep(CHECK_PERIOD);
        elapsed_sec = (unsigned int)difftime(time(NULL), test_start_time);

        // Get MPA of MUN by VU command
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                            test_mun, 0, 0, 0, vu_data_buf);
        mpa_checking = vu_data_buf[0];
        mpa_ch = (mpa_checking >> OFFSET_CH) & MASK_CH;
        mpa_lun = (mpa_checking >> OFFSET_LUN) & MASK_LUN;
        mpa_mpblk = (mpa_checking >> OFFSET_MPBLK) & MASK_MPBLK;
        //__________________________________________________________________________________________

        LOG_PRINTF("reclaim sec:%d, elapsed sec: %d, mpa:0x%08X ch:%d lun:%d mp_blk:%d\n",
                 test_retention_time, elapsed_sec, mpa_checking, mpa_ch, mpa_lun, mpa_mpblk);

        if ((test_retention_time + RECLAIM_SEC_MARGIN) < elapsed_sec) {
            if (mpa_origin != mpa_checking) {
                LOG_PRINTF("%s[TEST FAILED] - threshold over\n", KRED);
                //while(1);
            }
            else {
                LOG_PRINTF("%s[TEST FAILED] - monitoring block reclaim time\n", KRED);
            }
        }

        if (mpa_origin != mpa_checking) {
            if (elapsed_sec < (test_retention_time - precondition_sec)) {
                LOG_PRINTF("%s[TEST FAILED] - reclaimed to early\n", KRED);
                //while(1);
            }
            else {
                LOG_PRINTF("%s[TEST PASSED]\n", KBLU);
            }
            //return;
            break;
        }
    }

    // Set threshold table
    for (step_no = 1; step_no <= NUM_THRESHOLD_STEPS; step_no++) {
        unsigned int erase_count_range;
        unsigned int retention_time_th;

        erase_count_range = retention_threshold_table_default[step_no][0];
        retention_time_th = retention_threshold_table_default[step_no][1];

        LOG_PRINTF("[VU command] Set threshold step:%d, erase cnt:%d, read count:%d\n",
                 (step_no - 1), erase_count_range, retention_time_th);

        vu_data_to_host(fqa_device, SUB_OPCODE_SET_RETENTION_TH, VU_DATA_BUF_SIZE,
                            (step_no - 1), erase_count_range, retention_time_th, 0, vu_data_buf);
    }
    //______________________________________________________________________________________________

    free(vu_data_buf);
    fclose(fp_log);
}
