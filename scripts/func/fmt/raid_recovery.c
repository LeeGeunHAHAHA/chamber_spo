#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "mpa.h"
#include "vu_common.h"


#define VU_DATA_BUF_SIZE                    (512)

#define PRECOND_LBA_RANGE                   (0x180000) //0.75GiB
#define PRECOND_MUN_RANGE                   (PRECOND_LBA_RANGE / 8)

#define NUM_TEST_MUN                        (1)

#define PATROL_READ_ROUND_TIME_DEFAULT      (60 * 60) // 1HR
#define PATROL_READ_ROUND_TIME_SLOW         (60 * 60 * 24) // 24HR

#define TEST_MODE_BASIC_CLOSED              (1)
#define TEST_MODE_SEQ_READ_CLOSED           (2)
#define TEST_MODE_MULTI_PLANE_ERROR         (3)
#define TEST_MODE_BASIC_OPEN                (4)
#define TEST_MODE_APPEND_AFTER_OPEN_RAID    (5)
#define TEST_MODE_UNRECOVERABLE_CLOSED      (6)
#define TEST_MODE_UNRECOVERABLE_CLOSED_EXT  (7)
#define TEST_MODE_UNRECOVERABLE_OPEN        (8)
#define TEST_MODE_RAND_READ_CLOSED          (9)

typedef struct _test_mun {
    u32 mun;
    u32 mpa;
    u32 is_open_block;
} test_mun_t;

char cmd_str[500];
char* fqa_device;

void crush_wl_vt(u32 slc_mpa)
{
    u32 crush_mu_vector;

    crush_mu_vector = rand() % ((1 << (1 << WIDTH_MU)) - 1);
    crush_mu_vector |= (1 << MPA_TO_MU(slc_mpa));
   
    sprintf(cmd_str, "~/func_fqa/trunk/tools/nvme-cli-1.6/nvme fadu shell %s io wl_program_raw --ch %d --lun %d --mp_blk %d --plane_idx %d --wl %d --blk_mode s --mu_vector 0x%X",
                    fqa_device, MPA_TO_CH(slc_mpa), MPA_TO_LUN(slc_mpa), MPA_TO_MPBLK(slc_mpa), MPA_TO_PLANE(slc_mpa), MPA_TO_PAGE(slc_mpa), crush_mu_vector);
    system(cmd_str);
}

void main(int argc, char* argv[])
{
    char pattern_file[20] = "read_result";
   
    char* log_file_name;
    FILE* fp_log;

    u32 block_size = 4096;
    u32 i;
    u64 precondition_size = (u64)PRECOND_LBA_RANGE * 512;
    u32* vu_data_buf;

    test_mun_t* test_mun_array;
    u32 test_mun_index;
    u32 test_mode;

    printf("%s", KNRM);
    srand(time(NULL));

    test_mun_array = malloc(sizeof(test_mun_t) * NUM_TEST_MUN);
    vu_data_buf = malloc(VU_DATA_BUF_SIZE);

    if (argc < 4) {
        printf("argc:%d\n", argc);
        return;
    }

    // generate dummy file for VU write
    sprintf(cmd_str, "touch %s", pattern_file);
    system(cmd_str);

    fqa_device = argv[1];
    test_mode = atoi(argv[2]);
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

    sprintf(cmd_str, "fio SEQW.ini --size=%lld > log", precondition_size);
    system(cmd_str);
    system(cmd_str);
    
    precondition_sec = (unsigned int)difftime(time(NULL), write_start_time);
    //______________________________________________________________________________________________

    test_mun_index = 0;

    // select test MUNs from written range
    while (test_mun_index < NUM_TEST_MUN) {
        u32 test_mun;
        u32 test_mpa;
        u32 test_plane_bitmap;
        u32 super_block_state;

        if ((test_mode == TEST_MODE_BASIC_CLOSED)
            || (test_mode == TEST_MODE_SEQ_READ_CLOSED)
            || (test_mode == TEST_MODE_RAND_READ_CLOSED)
            || (test_mode == TEST_MODE_UNRECOVERABLE_CLOSED)
            || (test_mode == TEST_MODE_UNRECOVERABLE_CLOSED_EXT)
            || (test_mode == TEST_MODE_MULTI_PLANE_ERROR)) {
            // avoid to select mun in open string
            test_mun = rand() % (PRECOND_MUN_RANGE - NUM_OPEN_MUS_IN_OPEN_BLK);
        }
        else if ((test_mode == TEST_MODE_BASIC_OPEN)
                || (test_mode == TEST_MODE_UNRECOVERABLE_OPEN)
                || (test_mode == TEST_MODE_APPEND_AFTER_OPEN_RAID)) {
            // intend to select mun in open stripe
            test_mun = (PRECOND_MUN_RANGE - 1) - (rand() % 200);
        }

        // Get MPA of MUN by VU command
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                            test_mun, 0, 0, 0, vu_data_buf);
        test_mpa = vu_data_buf[0];

        if (test_mode == TEST_MODE_UNRECOVERABLE_OPEN) {
            u32 error_ch = (test_mpa >> OFFSET_CH) & MASK_CH;
            
            if (error_ch == 0) {
                LOG_PRINTF("%sTest canceled: not avaliable for testing unrecoverable open raid\n", KMAG);
                free(test_mun_array);
                free(vu_data_buf);
                fclose(fp_log);
                
                return;
            }
        }
        //__________________________________________________________________________________________

        // Check another MPA is duplicated
        u32 is_duplicated = 0;

        for (i = 0; i < test_mun_index; i++) {
            if (((TLC_MPA_TO_SLC_MPA(test_mun_array[i].mpa) >> OFFSET_PAGE) & MASK_PAGE)
                == ((TLC_MPA_TO_SLC_MPA(test_mpa) >> OFFSET_PAGE) & MASK_PAGE)) {
                is_duplicated = 1;
                break;
            }
        }

        if (is_duplicated) {
            continue;
        }
        //__________________________________________________________________________________________

        // Get super block state of MPA VU command for check open block
        vu_data_to_host(fqa_device, SUB_OPCODE_GET_SPBLK_STATE, VU_DATA_BUF_SIZE,
                            test_mpa, 0, 0, 0, vu_data_buf);
        super_block_state = vu_data_buf[0];
        //__________________________________________________________________________________________

        // register selected wl group information
        test_mun_array[test_mun_index].mun = test_mun;
        test_mun_array[test_mun_index].mpa = test_mpa;
        test_mun_array[test_mun_index].is_open_block = 0;

        LOG_PRINTF("%sMUN:0x%08X\n", KNRM, test_mun);
        LOG_PRINT_MPA(test_mpa, fp_log);

        if (super_block_state == BLOCK_LIST_IN_PFL) {
            test_mun_array[test_mun_index].is_open_block = 1;
            LOG_PRINTF("%sOpen block is selected, block state: %d\n", KCYN, super_block_state);
        }
        else if (super_block_state == BLOCK_LIST_DATA) {
            LOG_PRINTF("%sClosed block is selected, block state:%d\n", KGRN, super_block_state);
        }
        else {
            LOG_PRINTF("%sUnexpected block state, block state:%d\n", KMAG, super_block_state);
            while(1);
        }

        #if 1 // Error injection
        u32 slc_mpa = TLC_MPA_TO_SLC_MPA(test_mpa);
        u32 error_ppa = MPA_TO_PPA(slc_mpa);

        if (test_mode != TEST_MODE_MULTI_PLANE_ERROR) {
            // Error injection by over-program WL by "single-plane SLC mode"
            u32 test_plane = (test_mpa >> OFFSET_PLANE) & MASK_PLANE;
            test_plane_bitmap = (1 << test_plane);
        }
        else {
            // Error injection by over-program WL by "single-plane SLC mode"
            test_plane_bitmap = (1 << NUM_PLANES_PER_LUN) - 1;
        }

        //vu_no_data(fqa_device, SUB_OPCODE_NAND_PROGRAM, 1/*SLC mode*/, error_ppa, test_plane_bitmap, 0);
        crush_wl_vt(slc_mpa);

        if ((test_mode == TEST_MODE_UNRECOVERABLE_CLOSED)
            || (test_mode == TEST_MODE_UNRECOVERABLE_CLOSED_EXT)
            || (test_mode == TEST_MODE_UNRECOVERABLE_OPEN)) {
            u32 error_ch_2nd = ((slc_mpa >> OFFSET_CH) - 1) & MASK_CH;
            u32 error_slc_mpa_2nd;
            u32 error_ppa_2nd;
        
            error_slc_mpa_2nd = (slc_mpa & ~(MASK_CH << OFFSET_CH)) + (error_ch_2nd << OFFSET_CH);
            error_ppa_2nd = MPA_TO_PPA(error_slc_mpa_2nd);

            //vu_no_data(fqa_device, SUB_OPCODE_NAND_PROGRAM, 1/*SLC mode*/, error_ppa_2nd, test_plane_bitmap, 0);
            crush_wl_vt(error_slc_mpa_2nd);
        }

        if (test_mode == TEST_MODE_UNRECOVERABLE_CLOSED_EXT) {
            u32 error_ch_3rd = ((slc_mpa >> OFFSET_CH) - 2) & MASK_CH;
            u32 error_slc_mpa_3rd;
            u32 error_ppa_3rd;
        
            error_slc_mpa_3rd = (slc_mpa & ~(MASK_CH << OFFSET_CH)) + (error_ch_3rd << OFFSET_CH);
            error_ppa_3rd = MPA_TO_PPA(error_slc_mpa_3rd);

            //vu_no_data(fqa_device, SUB_OPCODE_NAND_PROGRAM, 1/*SLC mode*/, error_ppa_3rd, test_plane_bitmap, 0);
            crush_wl_vt(error_slc_mpa_3rd);
        }
        #endif
        
        test_mun_index++;
    }

    // Slow down patrol read round time not to influence test result
    vu_no_data(fqa_device, SUB_OPCODE_SET_PATROL_ROUND_TIME, PATROL_READ_ROUND_TIME_SLOW, 0, 0, 0);
    //______________________________________________________________________________________________

    sleep(1);

    // Seq.Read from precondition range & verify error injected page is recovered
    if ((test_mode == TEST_MODE_BASIC_CLOSED)
        || (test_mode == TEST_MODE_BASIC_OPEN)
        || (test_mode == TEST_MODE_UNRECOVERABLE_CLOSED)
        || (test_mode == TEST_MODE_UNRECOVERABLE_CLOSED_EXT)
        || (test_mode == TEST_MODE_UNRECOVERABLE_OPEN)
        || (test_mode == TEST_MODE_APPEND_AFTER_OPEN_RAID)) {
        for (i = 0; i < NUM_TEST_MUN; i++) {
            sprintf(cmd_str, "nvme read %s -s %d -c %d -z %d -d %s 2> log ",
                    fqa_device, test_mun_array[i].mun * 8, 4 - 1, block_size, pattern_file);
            LOG_PRINTF("%sVerify UECC injected MUN:%d\n", KNRM, test_mun_array[i].mun);
            system(cmd_str);
        }
    }
    else if ((test_mode == TEST_MODE_SEQ_READ_CLOSED)
            || (test_mode == TEST_MODE_MULTI_PLANE_ERROR)) {
        LOG_PRINTF("%sVerify Seq.R Read\n", KNRM);
        sprintf(cmd_str, "fio SEQR.ini --size=%lld > log", precondition_size);
        system(cmd_str);
    }
    else if (test_mode == TEST_MODE_RAND_READ_CLOSED) {
        LOG_PRINTF("%sVerify Rand.R Read in 5 sec\n", KNRM);
        sprintf(cmd_str, "fio RANDR.ini --size=%lld > log", precondition_size);
        system(cmd_str);
    }
    //______________________________________________________________________________________________

    if (test_mode == TEST_MODE_APPEND_AFTER_OPEN_RAID) {
        // Append write to open RAID page
        LOG_PRINTF("[Post-condition] Append write write slba:0x%08X, length:0x%08x\n", 0, PRECOND_LBA_RANGE / 2);

        sprintf(cmd_str, "fio SEQW.ini --size=%lld > log", precondition_size / 2);
        system(cmd_str);
        //__________________________________________________________________________________________

        // Get MPA of MUN by VU command
        for (i = 0; i < NUM_TEST_MUN; i++) {
            vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                                test_mun_array[i].mun, 0, 0, 0, vu_data_buf);
            LOG_PRINT_MPA(vu_data_buf[0], fp_log);
        }
        //__________________________________________________________________________________________        

        // Verify recovery in appended RAID page
        for (i = 0; i < NUM_TEST_MUN; i++) {
            sprintf(cmd_str, "nvme read %s -s %d -c %d -z %d -d %s 2> log ",
                    fqa_device, test_mun_array[i].mun * 8, 4 - 1, block_size, pattern_file);
            LOG_PRINTF("%sVerify UECC injected MUN:%d from appended page\n", KNRM, test_mun_array[i].mun);
            system(cmd_str);
        }
        //__________________________________________________________________________________________
    }

    // Get RAID stat and print
    // TBD
    //______________________________________________________________________________________________

    // Check error injected MPA is replaced
    // TBD
    //______________________________________________________________________________________________

    printf("%s", KNRM);

    // Revert temporarily changed  patrol read round time to finish valication
    vu_no_data(fqa_device, SUB_OPCODE_SET_PATROL_ROUND_TIME, PATROL_READ_ROUND_TIME_DEFAULT, 0, 0, 0);
    //______________________________________________________________________________________________

    // Reset RAID stat 
    // TBD
    //______________________________________________________________________________________________
    #if 0
    // Seq.Read from precondition range
    LOG_PRINTF("%sPost Seq.R Read\n", KNRM);
    //sprintf(cmd_str, "fio SEQR_6GiB.ini > log");
    sprintf(cmd_str, "fio SEQR.ini --size=%lld > log", precondition_size);
    system(cmd_str);
    #endif
    //______________________________________________________________________________________________

    // Get RAID stat 
    // TBD - expect nothing is udpated
    //______________________________________________________________________________________________

    free(test_mun_array);
    free(vu_data_buf);
    fclose(fp_log);
}
