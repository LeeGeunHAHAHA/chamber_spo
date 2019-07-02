/**
 * @file jesd219.c
 * @date 2018. 10. 04.
 * @author Hunk
 * 
 * Original: sim_workload.c of bm_sim project
 */

#include <stdio.h>
#include "jesd219.h"
#include "iogen.h"

static unsigned long set_random_seed = 0;

/* initialize state to random bits */
static unsigned long state[RANDOM_STATE];

/* init should also reset this to 0 */
static u32 __index = 0;

unsigned long WELLRNG512(void)
{
    unsigned long a, b, c, d;
    a = state[__index];
    c = state[(__index + 13) & 15];
    b = a ^ c ^ (a << 16) ^ (c << 15);
    c = state[(__index + 9) & 15];
    c ^= (c >> 11);
    a = state[__index] = b ^ c;
    d = a ^ ((a << 5) & 0xDA442D20UL);
    __index = (__index + 15) & 15;
    a = state[__index];
    state[__index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
    return state[__index];
}

u32 _get_lba_grp_idx_in_jesd219_workload(u32 rand_val)
{
    if (rand_val < PROB_ACCESS_LBA_SPACE_GROUP_A) {
        return LBA_GROUP_IDX_A;
    }
    else if (rand_val < PROB_ACCESS_LBA_SPACE_GROUP_A + PROB_ACCESS_LBA_SPACE_GROUP_B) {
        return LBA_GROUP_IDX_B;
    }
    else {
        return LBA_GROUP_IDX_C;
    }
}

u32 _get_num_of_mu_in_jesd219_workload(u32 rand_val)
{
    if (rand_val < PROB_PAYLOAD_SIZE_BELOW_512B) {
        return 512 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_1024B) {
        return 1024 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_1536B) {
        return 1536 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_2048B) {
        return 2048 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_2560B) {
        return 2560 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_3072B) {
        return 3072 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_3584B) {
        return 3584 / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_4K) {
        return (4ULL << 10) / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_8K) {
        return (8ULL << 10) / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_16K) {
        return (16ULL << 10) / SIZE_OF_SECTOR;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_32K) {
        return (32ULL << 10) / SIZE_OF_SECTOR;
    }
    else { //if(rand_val < PROB_PAYLOAD_SIZE_BELOW_64K)
        return (64ULL << 10) / SIZE_OF_SECTOR;
    }
}

u32 _get_num_of_mu_in_jesd219_workload_4k(u32 rand_val)
{
    if (rand_val < PROB_PAYLOAD_SIZE_BELOW_4K) {
        return (4ULL << 10) / SIZE_OF_SECTOR_4K;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_8K) {
        return (8ULL << 10) / SIZE_OF_SECTOR_4K;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_16K) {
        return (16ULL << 10) / SIZE_OF_SECTOR_4K;
    }
    else if (rand_val < PROB_PAYLOAD_SIZE_BELOW_32K) {
        return (32ULL << 10) / SIZE_OF_SECTOR_4K;
    }
    else { //if(rand_val < PROB_PAYLOAD_SIZE_BELOW_64K)
        return (64ULL << 10) / SIZE_OF_SECTOR_4K;
    }
}

void jesd219_gen(struct iogen_job *job)
{
    struct jesd219 *jesd219 = &job->jesd219;

    // LBA variable
    u32 lba_group_start_lba;
    u32 start_lba;
    u32 num_of_sector;

    u32 lba_group_size;
    u32 lba_group_idx;

    //printf("ns_info->lba_shift %d\n", job->ns_info->lba_shift);

    if (!set_random_seed) {
        u32 seed_idx;
        for (seed_idx = 0; seed_idx < RANDOM_STATE; seed_idx++) {
            state[seed_idx] = (job->params->seed + seed_idx);
        }
        set_random_seed = 1;
    }

    lba_group_idx = _get_lba_grp_idx_in_jesd219_workload(WELLRNG512() % 100);

    switch (lba_group_idx) {
    case LBA_GROUP_IDX_A:
        lba_group_start_lba = 0;
        lba_group_size = (u32)(job->ns_info->nlbs * RATIO_LBA_SPACE_GROUP_A);
        break;
    case LBA_GROUP_IDX_B:
        lba_group_start_lba = job->ns_info->nlbs * RATIO_LBA_SPACE_GROUP_A;
        lba_group_size = (u32)(job->ns_info->nlbs * RATIO_LBA_SPACE_GROUP_B);
        break;
    case LBA_GROUP_IDX_C:
        lba_group_start_lba = job->ns_info->nlbs * (RATIO_LBA_SPACE_GROUP_A + RATIO_LBA_SPACE_GROUP_B);
        lba_group_size = (u32)(job->ns_info->nlbs * RATIO_LBA_SPACE_GROUP_C);
        break;
    }

    while (1) {
        start_lba = (WELLRNG512() % lba_group_size) + lba_group_start_lba;

        if (job->params->slba.jesd219.sector_size == SIZE_OF_SECTOR) {
            num_of_sector = _get_num_of_mu_in_jesd219_workload(WELLRNG512() % 100);

            if (num_of_sector < (4096 / SIZE_OF_SECTOR))
            {
                jesd219->slba = start_lba;
                jesd219->nbls = num_of_sector;
            }
            else
            {
                jesd219->slba = start_lba - (start_lba % (4096 / SIZE_OF_SECTOR));
                jesd219->nbls = num_of_sector;
            }	
        }
        else if (job->params->slba.jesd219.sector_size == SIZE_OF_SECTOR_4K) {
            num_of_sector = _get_num_of_mu_in_jesd219_workload_4k(WELLRNG512() % 100);
            jesd219->slba = start_lba;
            jesd219->nbls = num_of_sector;
        }
        else {
            printf("Not supported Sector Size : %d\n", job->params->slba.jesd219.sector_size);
            exit(1);
        }

        if (jesd219->slba + jesd219->nbls <= job->ns_info->nlbs) {
            break;
        }
    }

    //printf("GRP %d %16llu:%4d\n", lba_group_idx, jesd219->slba, jesd219->nbls);

    return;
}

u64 jesd219_slba(struct jesd219 *jesd219)
{
    return jesd219->slba;
}

u32 jesd219_nlbs(struct jesd219 *jesd219)
{
    return jesd219->nbls;
}
