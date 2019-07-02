#ifndef _SELF_CHK_H_
#define _SELF_CHK_H_

#ifndef __GNUC__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/limits.h>
#else
#include <Windows.h>
#endif

typedef signed char         s8;

typedef unsigned int        u32;
typedef signed int          s32;

typedef signed short        s16;
typedef signed long long    s64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned long long  u64;

typedef u32                 vc_t;
typedef u32                 mpa_t;

#define CHECK_VC 	0

#define INVALID_MPA             (0xFFFFFFFF)
#define UNMAPPED_MPA            (0xFFFFFFF0)
#define UNCORRECTABLE_MPA       (0xFFFFFFF1)

#define FAL_VALIDITY_TABLE_SIZE_BYTE (((((1UL << ((3) + (3) + (12) + (10) + (1) + (2))) >> 3UL)+(0x7fffUL))&(~0x7fffUL)))
#define FAL_VC_TABLE_SIZE_BYTE (((((1UL << ((3) + (3) + (12))) * 4UL)+(0x7fffUL))&(~0x7fffUL)))
#define FTL_NOTIFY_GROUP_OFFSET ((((2)) + ((1)) + ((10))))

typedef struct _self_chk_ctx {
    u32 *vb_table;
    u32 vb_tbl_size;

    u32 *vc_table;
    u32 vc_tbl_size;

    u32* org_map_tbl;
        
#ifdef __GNUC__
    off_t org_map_tbl_len;
    int org_map_tbl_fd;
#else
    DWORD   org_map_tbl_len;
    HANDLE  org_map_tbl_file_handle;
    HANDLE  org_map_tbl_mapping_handle;
#endif
    
    u32 storage_max_lba;
    u32 ftl_notify_group_offset;

#ifdef __GNUC__
    char map_file[PATH_MAX];
    char bitmap_file[PATH_MAX];
    char valid_cnt_file [PATH_MAX];
#else
    char map_file[MAX_PATH];
    char bitmap_file[MAX_PATH];
    char valid_cnt_file[MAX_PATH];
#endif
}self_chk_ctx_t;

#endif //_SELF_CHK_H_
