#ifndef _FMT_COMMON_H_
#define _FMT_COMMON_H_

typedef unsigned char                   u8;
typedef unsigned short                  u16;
typedef unsigned int                    u32;
typedef unsigned long long              u64;

#define NUM_CHANNEL                     (8)
#define NUM_LUNS_PER_CHANNEL            (8)
#define NUM_PLANES_PER_LUN              (2)
#define NUM_PAGES_PER_WL                (3)
#define NUM_MUS_PER_PAGE                (2)

#define NUM_WLS_PER_BLOCK               (256)
#define NUM_PAGES_PER_BLOCK             (NUM_WLS_PER_BLOCK * NUM_PAGES_PER_WL)
#define NUM_WL_GROUP_PER_BLOCK          (2)

#define NUM_PAGES_PER_BLOCK             (NUM_WLS_PER_BLOCK * NUM_PAGES_PER_WL)
#define NUM_BLKS_PER_SPBLK              (NUM_CHANNEL * NUM_LUNS_PER_CHANNEL * NUM_PLANES_PER_LUN)

#define NUM_WLS_IN_STRING               (4)

#define NUM_OPEN_WL_IN_OPEN_BLK         (NUM_BLKS_PER_SPBLK * NUM_WLS_IN_STRING)
#define NUM_OPEN_PAGES_IN_OPEN_BLK      (NUM_OPEN_WL_IN_OPEN_BLK * NUM_PAGES_PER_WL)
#define NUM_OPEN_MUS_IN_OPEN_BLK        (NUM_OPEN_PAGES_IN_OPEN_BLK * NUM_MUS_PER_PAGE)

#define KNRM    "\x1B[0m"
#define KRED    "\x1B[31m"
#define KGRN    "\x1B[32m"
#define KYEL    "\x1B[33m"
#define KBLU    "\x1B[34m"
#define KMAG    "\x1B[35m"
#define KCYN    "\x1B[36m"
#define KWHT    "\x1B[37m"

#define LOG_PRINTF(fmt, ...)            printf(fmt, __VA_ARGS__); \
                                        fprintf(fp_log, fmt, __VA_ARGS__); \

#define BLOCK_LIST_ERASABLE             (0)
#define BLOCK_LIST_IN_PFL               (1)
#define BLOCK_LIST_DATA                 (2)
#define BLOCK_LIST_GC                   (3)
#define BLOCK_LIST_VC_ZERO              (4)
#define BLOCK_LIST_ERASABLE_CANDIDATE   (5)
#define BLOCK_LIST_RECLAIM              (6)
#define BLOCK_LIST_ERASED               (7)
#define BLOCK_LIST_BAD                  (8)
#define BLOCK_LIST_PATROL_READ          (9)

#endif //_FMT_COMMON_H_
