#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "mpa.h"
#include "vu_common.h"


#define VU_DATA_BUF_SIZE                    (512)

#define NUM_TEST_STRIPE_WLS                 (8)
#define NUM_TEST_STRIPE_PAGES               (NUM_TEST_STRIPE_WLS * NUM_PAGES_PER_WL)

//#define PRECOND_LBA_RANGE                   (0xC000) // 4 stripe size (for 8 x 4)
#define PRECOND_LBA_RANGE                   ((NUM_BLKS_PER_SPBLK * NUM_TEST_STRIPE_PAGES * 16 * 1024) / 512)
#define PRECOND_MUN_RANGE                   (PRECOND_LBA_RANGE / 8)

#define NUM_TEST_MUN                        (1)

#define TEST_MODE_RANDR_RUNTIME_CLOSED_RAID (20)
#define TEST_MODE_RANDR_RUNTIME_OPEN_RAID   (21)
#define TEST_MODE_RANDRW_RUNTIME_RAID       (22)

char* fqa_device;
char cmd_str[500];

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

    u64 precondition_size = (u64)PRECOND_LBA_RANGE * 512;
    u32* vu_data_buf;

    u32 test_mode;

    printf("%s", KNRM);
    srand(time(NULL));

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

    // Pre-condition Seq write
    LOG_PRINTF(" [Pre-condition] write slba:0x%08X, length:0x%08x\n", 0, PRECOND_LBA_RANGE);

    sprintf(cmd_str, "fio SEQW.ini --size=%lld > log", precondition_size);
    system(cmd_str);
    //______________________________________________________________________________________________

    // Get MPA of Last MUN
    u32 last_mun;
    u32 last_mpa;
    last_mun = PRECOND_MUN_RANGE - 1;

    vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                    last_mun, 0, 0, 0, vu_data_buf);
    last_mpa = vu_data_buf[0];
    //______________________________________________________________________________________________

    // Check last MPA is available for error injection test
    u32 last_page_no;
    last_page_no = (last_mpa >> OFFSET_PAGE) & MASK_PAGE;

    if (last_page_no >= (NUM_PAGES_PER_BLOCK - NUM_TEST_STRIPE_PAGES)) {
        free(vu_data_buf);
        fclose(fp_log);

        LOG_PRINTF("%sTest canceled by not available condition - page no:%d\n", KNRM, last_page_no);

        return;
    }
    //______________________________________________________________________________________________
    #if 0
    // Error inject(by SLC program) in WL which will be programmed soon
    u32 error_mpa;
    u32 error_mpa_slc;
    u32 error_ppa;
    u32 error_plane_bitmap;

    if (test_mode == TEST_MODE_RANDR_RUNTIME_CLOSED_RAID) {
        //error_mpa = last_mpa + ((rand() % 9 + 3) << OFFSET_PAGE);
        error_mpa = last_mpa + ((rand() % ((NUM_TEST_STRIPE_PAGES / 2) - NUM_PAGES_PER_WL) + NUM_PAGES_PER_WL) << OFFSET_PAGE);
    }
    else if (test_mode == TEST_MODE_RANDR_RUNTIME_OPEN_RAID) {
        //error_mpa = last_mpa + ((rand() % 21 + 3) << OFFSET_PAGE);
        error_mpa = last_mpa + ((rand() % (NUM_TEST_STRIPE_PAGES - NUM_PAGES_PER_WL) + NUM_PAGES_PER_WL) << OFFSET_PAGE);
    }
    else if (test_mode == TEST_MODE_RANDRW_RUNTIME_RAID) {
        //error_mpa = last_mpa + ((rand() % 21 + 3) << OFFSET_PAGE);
        error_mpa = last_mpa + ((rand() % (NUM_TEST_STRIPE_PAGES - NUM_PAGES_PER_WL) + NUM_PAGES_PER_WL) << OFFSET_PAGE);
    }
    error_mpa_slc = TLC_MPA_TO_SLC_MPA(error_mpa);

    error_ppa = MPA_TO_PPA(error_mpa_slc);

    // Error injection by over-program WL by "single-plane SLC mode"
    error_plane_bitmap = (1 << NUM_PLANES_PER_LUN) - 1;

    vu_no_data(fqa_device, SUB_OPCODE_NAND_PROGRAM, 1/*SLC mode*/, error_ppa, error_plane_bitmap, 0);
    sleep(1);
    #else
    // Error inject(by SLC program) in WL which will be programmed soon
    u32 error_mpa;
    u32 error_mpa_slc;
    u32 error_page_no;
    u32 error_ppa;

    error_mpa = last_mpa;
    error_page_no = last_page_no;

    u32 count = 0;

    while (1) {
        u32 error_interval = (rand() % 3) + 12;

        error_mpa += ((NUM_PAGES_PER_WL * error_interval) << OFFSET_PAGE);
        error_page_no += NUM_PAGES_PER_WL * error_interval;

        if (error_page_no >= NUM_PAGES_PER_BLOCK) {
            break;
        }

        if (test_mode == TEST_MODE_RANDR_RUNTIME_CLOSED_RAID) {
            if ((error_page_no - last_page_no) > (NUM_TEST_STRIPE_PAGES / 2)) {
                break;
            }
        }

        if (test_mode == TEST_MODE_RANDR_RUNTIME_OPEN_RAID) {
            if ((error_page_no - last_page_no) > NUM_TEST_STRIPE_PAGES) {
                break;
            }
        }

        #if 1
        if (test_mode == TEST_MODE_RANDRW_RUNTIME_RAID) {
            // randomly select channel & LUN
            error_mpa = error_mpa & ~(MASK_CH << OFFSET_CH);
            error_mpa |= ((rand() % NUM_CHANNEL) << OFFSET_CH);
            
            error_mpa = error_mpa & ~(MASK_LUN << OFFSET_LUN);
            error_mpa |= ((rand() % NUM_LUNS_PER_CHANNEL) << OFFSET_LUN);
        }
        #endif

        error_mpa_slc = TLC_MPA_TO_SLC_MPA(error_mpa);
        error_ppa = MPA_TO_PPA(error_mpa_slc);

        // Error injection by over-program WL by "single-plane SLC mode"
        u32 error_plane_bitmap = (1 << NUM_PLANES_PER_LUN) - 1;

        LOG_PRINTF("%s crash ppa ch:%d, lun:%d, mpblk:%d, page:%d(TLC page:%d)\n", KNRM,
                    MPA_TO_CH(error_mpa_slc), MPA_TO_LUN(error_mpa_slc), MPA_TO_MPBLK(error_mpa_slc), MPA_TO_PAGE(error_mpa_slc), MPA_TO_PAGE(error_mpa_slc) * 3);

        vu_no_data(fqa_device, SUB_OPCODE_NAND_PROGRAM, 1/*SLC mode*/, error_ppa, error_plane_bitmap, 0);
        //crush_wl_vt(error_mpa_slc);

        count++;

        #if 0
        if (count == 10) {
            break;
        }
        #endif
    }
    sleep(1);
    #endif
    //______________________________________________________________________________________________

    // mixed random io
    if ((test_mode == TEST_MODE_RANDR_RUNTIME_CLOSED_RAID)
        || (test_mode == TEST_MODE_RANDR_RUNTIME_OPEN_RAID)) {
        LOG_PRINTF("%s Random read for 5 seconds\n", KNRM);
        sprintf(cmd_str, "fio RANDR.ini --size=%lld > log", precondition_size);
        system(cmd_str);
    }
    else if (test_mode == TEST_MODE_RANDRW_RUNTIME_RAID) {
        u32 test_mp_blk = (last_mpa >> OFFSET_MPBLK) & MASK_MPBLK;

        while (1) {
            LOG_PRINTF("%s random read background &", KNRM);
            sprintf(cmd_str, "fio RANDR.ini --size=%lld > log &", precondition_size);
            system(cmd_str);

            LOG_PRINTF("%s random write\n", KNRM);
            sprintf(cmd_str, "fio RANDW.ini --size=%lld > log", precondition_size);
            system(cmd_str);

            // Get MPA of Last MUN
            vu_data_to_host(fqa_device, SUB_OPCODE_GET_MPA_FROM_MUN, VU_DATA_BUF_SIZE,
                            last_mun, 0, 0, 0, vu_data_buf);
            
            u32 updated_mpa = vu_data_buf[0];
            u32 updated_mp_blk = (updated_mpa >> OFFSET_MPBLK) & MASK_MPBLK;
            u32 updated_page = (updated_mpa >> OFFSET_PAGE) & MASK_PAGE;

            LOG_PRINTF("%s last mpa blk:%d page:%d\n", KNRM, updated_mp_blk, updated_page);

            if (test_mp_blk != updated_mp_blk) {
                break;
            }
            //______________________________________________________________________________________
        }
    }
    //______________________________________________________________________________________________

    printf("%s", KNRM);
    free(vu_data_buf);
    fclose(fp_log);
}
