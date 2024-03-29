/**
 * @file jesd219.h
 * @date 2018. 10. 04.
 * @author Hunk
 */
#ifndef __NBIO_JESD219_H__
#define __NBIO_JESD219_H__

#include "types.h"

#define RANDOM_STATE       (16)
#define SIZE_OF_SECTOR     (512)
#define SIZE_OF_SECTOR_4K  (4096)

/* - ---------------------- JESD219_ENTERPRISE Workload Configuration  ---------------------- */
#define PROB_PAYLOAD_SIZE_BELOW_512B		  (4)
#define PROB_PAYLOAD_SIZE_BELOW_1024B		  (PROB_PAYLOAD_SIZE_BELOW_512B + 1)
#define PROB_PAYLOAD_SIZE_BELOW_1536B		  (PROB_PAYLOAD_SIZE_BELOW_1024B + 1)
#define PROB_PAYLOAD_SIZE_BELOW_2048B		  (PROB_PAYLOAD_SIZE_BELOW_1536B + 1)
#define PROB_PAYLOAD_SIZE_BELOW_2560B		  (PROB_PAYLOAD_SIZE_BELOW_2048B + 1)
#define PROB_PAYLOAD_SIZE_BELOW_3072B		  (PROB_PAYLOAD_SIZE_BELOW_2560B + 1)
#define PROB_PAYLOAD_SIZE_BELOW_3584B		  (PROB_PAYLOAD_SIZE_BELOW_3072B + 1)
#define PROB_PAYLOAD_SIZE_BELOW_4K            (PROB_PAYLOAD_SIZE_BELOW_3584B + 67)
#define PROB_PAYLOAD_SIZE_BELOW_8K            (PROB_PAYLOAD_SIZE_BELOW_4K  + 10)
#define PROB_PAYLOAD_SIZE_BELOW_16K           (PROB_PAYLOAD_SIZE_BELOW_8K  + 7)
#define PROB_PAYLOAD_SIZE_BELOW_32K           (PROB_PAYLOAD_SIZE_BELOW_16K + 3)
#define PROB_PAYLOAD_SIZE_BELOW_64K           (PROB_PAYLOAD_SIZE_BELOW_32K + 3)

#define LBA_GROUP_IDX_A                       (0)
#define LBA_GROUP_IDX_B                       (1)
#define LBA_GROUP_IDX_C                       (2)

#define PROB_ACCESS_LBA_SPACE_GROUP_A         (50)
#define PROB_ACCESS_LBA_SPACE_GROUP_B         (30)
#define PROB_ACCESS_LBA_SPACE_GROUP_C         (20)

#define RATIO_LBA_SPACE_GROUP_A                (0.05)
#define RATIO_LBA_SPACE_GROUP_B                (0.15)
#define RATIO_LBA_SPACE_GROUP_C                (0.80)

#define RATIO_LBA_SPACE_10_PERCENT             (0.1)

struct jesd219 {
    bool enterprise;
    u64 slba;
    u32 nbls;
};

struct iogen_job;
void jesd219_gen(struct iogen_job *job);
u64 jesd219_slba(struct jesd219 *jesd219);
u32 jesd219_nlbs(struct jesd219 *jesd219);

#endif /* __NBIO_JESD219_H__ */