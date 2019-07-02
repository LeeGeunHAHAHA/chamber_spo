#define WIDTH_MU                        (2)
#define WIDTH_PLANE                     (1)
#define WIDTH_PAGE                      (10)
#define WIDTH_MPBLK                     (11)
#define WIDTH_LUN                       (3)
#define WIDTH_CH                        (3)

#define OFFSET_MU                       (0)
#define OFFSET_PLANE                    (OFFSET_MU + WIDTH_MU)
#define OFFSET_PAGE                     (OFFSET_PLANE + WIDTH_PLANE)
#define OFFSET_MPBLK                    (OFFSET_PAGE + WIDTH_PAGE)
#define OFFSET_LUN                      (OFFSET_MPBLK + WIDTH_MPBLK)
#define OFFSET_CH                       (OFFSET_LUN + WIDTH_LUN)

#define MASK_MU                         ((1 << WIDTH_MU) - 1)
#define MASK_PLANE                      ((1 << WIDTH_PLANE) - 1)
#define MASK_PAGE                       ((1 << WIDTH_PAGE) - 1)
#define MASK_MPBLK                      ((1 << WIDTH_MPBLK) - 1)
#define MASK_LUN                        ((1 << WIDTH_LUN) - 1)
#define MASK_CH                         ((1 << WIDTH_CH) - 1)

#define MASK_CH_LUN_MPBLK_PLANE         (MASK_CH << OFFSET_CH) | (MASK_LUN << OFFSET_LUN) | (MASK_MPBLK << OFFSET_MPBLK) | (MASK_PLANE << OFFSET_PLANE)

#define MPA_TO_CH(mpa)                  ((mpa >> OFFSET_CH) & MASK_CH)
#define MPA_TO_LUN(mpa)                 ((mpa >> OFFSET_LUN) & MASK_LUN)
#define MPA_TO_MPBLK(mpa)               ((mpa >> OFFSET_MPBLK) & MASK_MPBLK)
#define MPA_TO_PAGE(mpa)                ((mpa >> OFFSET_PAGE) & MASK_PAGE)
#define MPA_TO_PLANE(mpa)               ((mpa >> OFFSET_PLANE) & MASK_PLANE)
#define MPA_TO_MU(mpa)                  ((mpa >> OFFSET_MU) & MASK_MU)

static inline void LOG_PRINT_MPA(u32 mpa, FILE* fp_log)
{
    u32 channel = (mpa >> OFFSET_CH) & MASK_CH;;
    u32 lun = (mpa >> OFFSET_LUN) & MASK_LUN;
    u32 mp_blk = (mpa >> OFFSET_MPBLK) & MASK_MPBLK;
    u32 plane = (mpa >> OFFSET_PLANE) & MASK_PLANE;
    u32 page = (mpa >> OFFSET_PAGE) & MASK_PAGE;
    u32 mu = (mpa >> OFFSET_MU) & MASK_MU;
        
    LOG_PRINTF("%sMPA:0x%08x CH:%d, LUN:%d, MPBLK:%d, PLN:%d, PAGE:%d, MU:%d\n", KNRM, mpa, channel, lun, mp_blk, plane, page, mu);
}

static inline u32 TLC_MPA_TO_SLC_MPA(u32 tlc_mpa)
{
    u32 tlc_mpa_page = (tlc_mpa >> OFFSET_PAGE) & MASK_PAGE;
    u32 slc_mpa_page = tlc_mpa_page / 3;

    u32 slc_mpa;
    slc_mpa = tlc_mpa & ~(MASK_PAGE << OFFSET_PAGE);
    slc_mpa |= (slc_mpa_page << OFFSET_PAGE);

    return slc_mpa;
}

static inline u32 MPA_TO_PPA(u32 mpa)
{
    return (mpa >> WIDTH_MU);
}

static inline u32 PPA_TO_MPA(u32 ppa)
{
    return (ppa << WIDTH_MU);
}