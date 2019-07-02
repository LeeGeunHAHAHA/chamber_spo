#include "ftl_checker.h"

#define SELF_CHK_MAP_FILE   "map.out"
#define SELF_CHK_BMAP_FILE  "bitmap.out"
#define SELF_CHK_VC_FILE    "vc.out"

#define MAX_LBA     0x2e940000

self_chk_ctx_t g_self_chk_ctx;

#ifdef __GNUC__
s32 __setup_memory_map_file(self_chk_ctx_t* s_chk_ctx)
{
    s_chk_ctx->org_map_tbl_fd = open(s_chk_ctx->map_file, O_RDONLY);
    if (s_chk_ctx->org_map_tbl_fd < 0) {
        printf("Failed to open memory map file\n");
        return -1;
    }

    s_chk_ctx->org_map_tbl_len = lseek(s_chk_ctx->org_map_tbl_fd, 0, SEEK_END);
    if (s_chk_ctx->org_map_tbl_len == (off_t)-1) {
        printf("Failed to lseek");
        return -1;
    }

    s_chk_ctx->org_map_tbl = (u32*)mmap(0, s_chk_ctx->org_map_tbl_len, PROT_READ, MAP_SHARED, s_chk_ctx->org_map_tbl_fd, 0);
    if (s_chk_ctx->org_map_tbl == MAP_FAILED) {
        printf("Failed to mmap\n");
        return -1;
    }

    return 0;
}
#else
s32 __setup_memory_map_file(self_chk_ctx_t* s_chk_ctx)
{
    DWORD       error_code;

    s_chk_ctx->org_map_tbl_file_handle = CreateFile(s_chk_ctx->map_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (s_chk_ctx->org_map_tbl_file_handle == INVALID_HANDLE_VALUE) {
        printf("Failed to CreateFile\n");
        return -1;
    }

    s_chk_ctx->org_map_tbl_mapping_handle = CreateFileMapping(s_chk_ctx->org_map_tbl_file_handle, NULL, PAGE_READONLY, 0, 0, 0);
    if (s_chk_ctx->org_map_tbl_mapping_handle == NULL) {
        printf("Failed to CreateFileMapping\n");
        return -1;
    }

    error_code = GetLastError();
    if (error_code != ERROR_SUCCESS) {
        printf("Failed to CreateFileMapping [%d]", error_code);
        return -1;
    }

    s_chk_ctx->org_map_tbl_len = GetFileSize(s_chk_ctx->org_map_tbl_file_handle, NULL);

    s_chk_ctx->org_map_tbl = MapViewOfFile(s_chk_ctx->org_map_tbl_mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (s_chk_ctx->org_map_tbl == NULL) {
        printf("Failed to MapViewOfFile [%d]", error_code);
        return -1;
    }

    return 0;
}
#endif

s32 _self_check_init(self_chk_ctx_t* s_chk_ctx, int argc, char* argv[])
{
	u32 vb_tbl_size = 0;

    strncpy(s_chk_ctx->map_file, argv[1], strlen(argv[1]));
    strncpy(s_chk_ctx->bitmap_file, argv[2], strlen(argv[2]));
    strncpy(s_chk_ctx->valid_cnt_file, argv[3], strlen(argv[3]));

    s_chk_ctx->storage_max_lba = atoi(argv[4]);
    s_chk_ctx->vb_tbl_size = atoi(argv[5]);
    s_chk_ctx->vc_tbl_size = atoi(argv[6]);
    s_chk_ctx->ftl_notify_group_offset = atoi(argv[7]);

	if (s_chk_ctx->vb_tbl_size == 0) {
		vb_tbl_size = (u32)(s_chk_ctx->storage_max_lba * 3 / 8);
	}
	else {
		vb_tbl_size = s_chk_ctx->vb_tbl_size;
	}

    //s_chk_ctx->vb_tbl_size = FAL_VALIDITY_TABLE_SIZE_BYTE;
    s_chk_ctx->vb_table = (u32 *)malloc(vb_tbl_size);
    if (s_chk_ctx->vb_table == NULL) {
        printf("Failed to allocate vb_table\n");
        return -1;
    }

    //s_chk_ctx->vc_tbl_size = FAL_VC_TABLE_SIZE_BYTE;
    s_chk_ctx->vc_table = (u32 *)malloc(s_chk_ctx->vc_tbl_size);
    if (s_chk_ctx->vc_table == NULL) {
        printf("Failed to allocate vc_table\n");
        return -1;
    }

    memset(s_chk_ctx->vb_table, 0x00, s_chk_ctx->vb_tbl_size);
    memset(s_chk_ctx->vc_table, 0x00, s_chk_ctx->vc_tbl_size);

    // connect memory map file
    if (__setup_memory_map_file(s_chk_ctx) < 0) {
        printf("Failed to setup memory map file\n");
        return -1;
    }

    return 0;
}

void _self_check_final(self_chk_ctx_t* s_chk_ctx)
{
    if (s_chk_ctx->vb_table) {
        free(s_chk_ctx->vb_table);
        s_chk_ctx->vb_table = NULL;
    }

    if (s_chk_ctx->vc_table) {
        free(s_chk_ctx->vc_table);
        s_chk_ctx->vc_table = NULL;
    }

#ifdef __GNUC__
    if (s_chk_ctx->org_map_tbl)
        munmap(s_chk_ctx->org_map_tbl, s_chk_ctx->org_map_tbl_len);

    close(s_chk_ctx->org_map_tbl_fd);
#else
    if (s_chk_ctx->org_map_tbl)
        UnmapViewOfFile((LPCVOID)s_chk_ctx->org_map_tbl);

    if (s_chk_ctx->org_map_tbl_mapping_handle != NULL)
        CloseHandle(s_chk_ctx->org_map_tbl_mapping_handle);

    if (s_chk_ctx->org_map_tbl_file_handle != INVALID_HANDLE_VALUE)
        CloseHandle(s_chk_ctx->org_map_tbl_file_handle);
#endif
}

int _self_check_vbtbl_write(self_chk_ctx_t* s_chk_ctx, mpa_t mpa)
{
    u32*      dw_addr;
    u32       bit_idx;
    u32       dw;
    u8*       lp_vb_tbl;

    lp_vb_tbl = (u8*)s_chk_ctx->vb_table;
    dw_addr = (u32*)(lp_vb_tbl + ((mpa >> 5) << 2)); // 32bit and * 4B
    bit_idx = mpa & 0x1f;

    dw = *dw_addr;

    if ((dw & (1 << bit_idx)) != 0) {
        return -1;
    }

    dw |= (1 << bit_idx);

    *dw_addr = dw;

    return 0;
}

int _self_check_vctbl_write(self_chk_ctx_t* s_chk_ctx, mpa_t mpa)
{
    u32 idx;
    volatile u32 val;
    vc_t *p_vc_tbl;

    p_vc_tbl = (vc_t*)s_chk_ctx->vc_table;
    idx = mpa >> s_chk_ctx->ftl_notify_group_offset;

    val = p_vc_tbl[idx];
    val++;

    p_vc_tbl[idx] = val;

    return 0;
}

s32 _build_vbtbl_vctbl_from_mtbl(self_chk_ctx_t* s_chk_ctx)
{
    u32 lba;
    mpa_t mpa;
    s32 ret_val = 0;
    u32 i_mpa = 0, u_mpa = 0, uc_mpa = 0;

    for (lba = 0; lba < s_chk_ctx->storage_max_lba; lba++) {
        mpa = s_chk_ctx->org_map_tbl[lba];

        //if (lba % 10000000 == 0)
        //    printf("lba:0x%08x, mpa:0x%08x\n", lba, s_chk_ctx->org_map_tbl[lba]);

        if (mpa == INVALID_MPA) {
            i_mpa++;
        }
        else if (mpa == UNMAPPED_MPA) {
            u_mpa++;
        }
        else if (mpa == UNCORRECTABLE_MPA) {
            uc_mpa++;
        }
        else {
           if (_self_check_vbtbl_write(s_chk_ctx, mpa) == -1) {
               u32 lba2;
               ret_val = -1;
               for (lba2 = 0; lba2 < s_chk_ctx->storage_max_lba; lba2++) {
                   if (mpa == s_chk_ctx->org_map_tbl[lba]) {
                       break;
                   }
               }
               printf("Detected duplicate MPA [lba = %08x, lba2 = %08x, mpa = %08x]\n", lba, lba2, mpa);
           }
           _self_check_vctbl_write(s_chk_ctx, mpa);
        }
    }

    printf("i_mpa = %d, u_mpa=%d, uc_mpa = %d\n", i_mpa, u_mpa, uc_mpa);
    return ret_val;
}

void _write_data_to_file(const char* file_name, void* data, u32 len)
{
    FILE* fp;
    fp = fopen(file_name, "wb");
    fwrite(data, 1 , len, fp);
    fclose(fp);
}

u32* _read_data_from_file(const char* file_name)
{
    FILE* fp = NULL;
    long size = 0;
    size_t readed = 0;
    u32*  data = NULL;

    fp = fopen(file_name, "rb");
    if (fp == NULL)
        return data;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    data = (u32*)malloc(size);
    if (data == NULL) {
        fclose(fp);
        return data;
    }

    readed = fread(data, 1, size, fp);
    if (readed != size) {
        printf("Failed to read file\n");
    }

    fclose(fp);

    return data;
}

s32 _compare_vbtbl_vctbl_from_chktbl(self_chk_ctx_t* s_chk_ctx)
{
    u32*   org_vb_tbl = NULL;
    u32*   org_vc_tbl = NULL;

    s32   ret_val = 0;
    u32   vb_idx = 0;
#if (CHECK_VC == 1)
    u32   vc_idx = 0;
#endif //CHECK_VC == 1
    org_vb_tbl = _read_data_from_file(s_chk_ctx->bitmap_file);
    if (org_vb_tbl == NULL) {
        printf("Failed to open bitmap file\n");
		ret_val = -1;
        goto compare_exit_routine;
    }

    org_vc_tbl = _read_data_from_file(s_chk_ctx->valid_cnt_file);
    if (org_vc_tbl == NULL) {
        printf("Failed to open valid count file\n");
		ret_val = -1;
        goto compare_exit_routine;
    }

#if (_DEBUG == 1)
    _write_data_to_file("build_vb.out", (void*)s_chk_ctx->vb_table, s_chk_ctx->vb_tbl_size);
    _write_data_to_file("build_vc.out", (void*)s_chk_ctx->vc_table, s_chk_ctx->vc_tbl_size);
#endif

    // Check VB Table
    // Example #1: First bit is problem case, last case is Good.
    // Build bitmap   : 1 0 1 0
    // Orginal bitmap : 0 0 1 1
    // XOR operation  : 1 0 0 1
    // -------------------------------
    // Build bitmap   : 1 0 1 0
    // XOR bitmap     : 1 0 0 1
    // AND operation  : 1 0 0 0

    // Example #2: All case is good
    // Build bitmap   : 0 0 1 0
    // Orginal bitmap : 1 0 1 1
    // XOR operation  : 1 0 0 1
    // -------------------------------
    // Build bitmap   : 0 0 1 0
    // XOR operation  : 1 0 0 1
    // AND operation  : 0 0 0 0

	if (s_chk_ctx->vb_tbl_size)
	{
		for (vb_idx=0; vb_idx < (s_chk_ctx->vb_tbl_size / 4); vb_idx++) {
			// Compare build bitmap and original bitmap
			u32 xor_result = s_chk_ctx->vb_table[vb_idx] ^ org_vb_tbl[vb_idx];
			// Compare build bitmap and xor_result bitmap
			u32 decide_result = s_chk_ctx->vb_table[vb_idx] & xor_result;
			if (decide_result) {
				printf("Valid bitmap has problem\n");
				printf("vb_idx = %d, build_value=%08x, read_value=%08x",
					vb_idx, s_chk_ctx->vb_table[vb_idx], org_vb_tbl[vb_idx]);
				ret_val = -1;
				goto compare_exit_routine;
			}
		}
	}

#if (CHECK_VC == 1)
    for (vc_idx=0; vc_idx < (s_chk_ctx->vc_tbl_size / 4); vc_idx++) {
        if (s_chk_ctx->vc_table[vc_idx] > org_vc_tbl[vc_idx]) {
            printf("Valid count has problem!!!");
            printf("vc_idx = %d, build_value=%08x, read_value=%08x",
                vc_idx, s_chk_ctx->vc_table[vc_idx], org_vc_tbl[vc_idx]);
            ret_val = -1;
            goto compare_exit_routine;
        }
    }
#endif //(CHECK_VC == 1)

compare_exit_routine:
    if (org_vb_tbl)
        free(org_vb_tbl);

    if (org_vc_tbl)
        free(org_vc_tbl);

    return ret_val;
}

void usage(void)
{
    printf("Self Checker Utiility\n");
    printf("self_checker [arguments]\n");
    printf("\t1: map file\n");
    printf("\t2: vb file\n");
    printf("\t3: vc file\n");
    printf("\t4: max lba\n");
    printf("\t5: max bitmap table size\n");
    printf("\t6: max valid count table size\n");
    printf("\t7: notify grp offset\n");
    printf("Example:\n");
    printf("./self_checker ./map.out ./bitmap.out ./vc.out 781451264 268435456 1048576 13\n");
}

int main(int argc, char* argv[])
{
    printf("==================FTL Checker ========================\n");

    if (argc < 8) {
        usage();
		exit(180);
        return 0;
    }

    // arg 1: map file
    // arg 2: vb file
    // arg 3: vc file
    // arg 4: MAX lba
    // arg 5: VB size
    // arg 6: VC size
    // arg 7: FTl Notify group offset

    if (_self_check_init(&g_self_chk_ctx, argc, argv) < 0) {
		exit(180);
        return 0;
	}

    printf("SELF_VB_TBL_ADDR = %p\n", g_self_chk_ctx.vb_table);
    printf("SELF_VB_TBL_SIZE = 0x%08x\n", g_self_chk_ctx.vb_tbl_size);
    printf("SELF_VC_TBL_ADDR = %p\n", g_self_chk_ctx.vc_table);
    printf("SELF_VC_TBL_SIZE = 0x%08x\n", g_self_chk_ctx.vc_tbl_size);
    printf("================================================\n");

    if (_build_vbtbl_vctbl_from_mtbl(&g_self_chk_ctx) < 0) {
        printf("Failed to check ftl meta data\n");
		exit(180);
        return 1;
    }

    if (_compare_vbtbl_vctbl_from_chktbl(&g_self_chk_ctx) < 0) {
        printf("Failed to check ftl meta data\n");
		exit(180);
        return 1;
    }

    _self_check_final(&g_self_chk_ctx);

    printf("Sucessfully completed!!!\n");

    return 0;
}
