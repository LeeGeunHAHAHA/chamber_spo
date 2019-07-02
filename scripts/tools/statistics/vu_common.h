#ifndef _VU_COMMON_H_
#define _VU_COMMON_H_

typedef unsigned char                       u8;
typedef unsigned short                      u16;
typedef unsigned int                        u32;
typedef unsigned long long                  u64;

#define VU_COMMON_OPCODE 0xC2

// module FTL
#define SUB_OPCODE_GET_READ_COUNT           (0x80000001)
#define SUB_OPCODE_GET_MPA_FROM_MUN         (0x80000401)
#define SUB_OPCODE_SET_SPBLK_EC             (0x80000500)
#define SUB_OPCODE_SET_RETENTION_TH         (0x80000600)
#define SUB_OPCODE_SET_READ_DISTURB_TH      (0x80000700)
#define SUB_OPCODE_GET_READ_BIAS_TABLE      (0x80000801)
#define SUB_OPCODE_SET_READ_BIAS_TABLE      (0x80000900)
#define SUB_OPCODE_SET_PATROL_ROUND_TIME    (0x80000A00)
#define SUB_OPCODE_GET_SPBLK_STATE          (0x80000B01)
#define SUB_OPCODE_SET_SET_FTL_PRINT_MODE   (0x80000C00)

// module NIL
#define SUB_OPCODE_NAND_PROGRAM             (0x80010000)

// module STAT
#define SUB_OPCODE_GET_STAT_DEFENSE_CODE    (0x80020001)
#define SUB_OPCODE_RESET_STAT_DEFENSE_CODE  (0x80020100)

// module STATUS
#define VU_SUB_OPCODE_GET_NVME_STATUS       (0x80060001)
#define VU_SUB_OPCODE_GET_HXL_STATUS        (0x80060101)
#define VU_SUB_OPCODE_GET_FAL_STATUS        (0x80060201)
#define VU_SUB_OPCODE_GET_NIL_STATUS        (0x80060301)

// print
#define KNRM    "\x1B[0m"
#define KRED    "\x1B[31m"
#define KGRN    "\x1B[32m"
#define KYEL    "\x1B[33m"
#define KBLU    "\x1B[34m"
#define KMAG    "\x1B[35m"
#define KCYN    "\x1B[36m"
#define KWHT    "\x1B[37m"

#define LOG(fmt, ...)                   printf(fmt, __VA_ARGS__); \
                                        fprintf(fp_log, fmt, __VA_ARGS__);


static inline void vu_no_data(char *fqa_device, unsigned int sub_opcode, unsigned int param0, unsigned int param1, unsigned int param2, unsigned int param3)
{
    char cmd_str[500];

    sprintf(cmd_str, "nvme io-passthru %s --opcode=0x%x --namespace-id=1 --cdw2=0x%x --cdw12=%d --cdw13=%d --cdw14=%d --cdw15=%d 2> temp_out",
                fqa_device,
                VU_COMMON_OPCODE,
                sub_opcode,
                param0, param1, param2, param3);

    system(cmd_str);
    system("rm ./temp_out");
}

/*  data_buf should be allocated and released by caller
    and allocated buffer size should be same or bigger than data_len */
static inline void vu_data_to_host(char *fqa_device, unsigned int sub_opcode, unsigned int data_len, unsigned int param0, unsigned int param1, unsigned int param2, unsigned int param3, unsigned int *data_buf)
{
    char cmd_str[500];

    sprintf(cmd_str, "nvme io-passthru %s --opcode=0x%x --namespace-id=1 --cdw2=0x%x --read --raw-binary --data-len=%d --cdw10=%d --cdw12=%d --cdw13=%d --cdw14=%d --cdw15=%d > temp_out",
                fqa_device,
                VU_COMMON_OPCODE,
                sub_opcode,
                data_len,
                data_len / 4, // ndt
                param0, param1, param2, param3);

    system(cmd_str);

    FILE *fp;
    fp = fopen("temp_out", "rb");

    if (fp == NULL) {
        printf("VU data out buffer file open failed\n");
    }
    else {
        fread(data_buf, data_len, 1, fp);
        fclose(fp);
    }

    system("rm ./temp_out");
}

static inline void vu_data_from_host(char *fqa_device, unsigned int sub_opcode, unsigned int data_len, unsigned int param0, unsigned int param1, unsigned int param2, unsigned int param3, unsigned int *data_buf)
{
    char cmd_str[500];
    FILE *fp;
    fp = fopen("temp_in", "wb");

    if (fp == NULL) {
        printf("VU data in buffer file open failed\n");
        return;
    }
    else {
        fwrite(data_buf, data_len, 1, fp);

        sprintf(cmd_str, "nvme io-passthru %s --opcode=0x%x --namespace-id=1 --cdw2=0x%x --write --input-file=./temp_in --data-len=%d --cdw10=%d --cdw12=%d --cdw13=%d --cdw14=%d --cdw15=%d",
                    fqa_device,
                    VU_COMMON_OPCODE,
                    sub_opcode,
                    data_len,
                    data_len / 4, // ndt
                    param0, param1, param2, param3);

        system(cmd_str);

        system("rm ./temp_in");
        fclose(fp);
    }
}

#endif // _VU_COMMON_H_
