/**
 * @file nvme_ds.h
 * @date 2018. 10. 03.
 * @author kmsun
 */
#ifndef __ADMIN_VIRTUAL_MGMT_NVME_DS_H__
#define __ADMIN_VIRTUAL_MGMT_NVME_DS_H__

#define PACKED __attribute__ ((__packed__))

typedef unsigned int    u32;
typedef unsigned short  u16;
typedef unsigned char   u8;

typedef struct PACKED __nvme_id_prim_ctrl_cap {
    u16 cntlid;
    u16 portid;
    u8  crt;
    u8  rsvd5[27];
    u32 vqfrt;
    u32 vqrfa;
    u16 vqrfap;
    u16 vqprt;
    u16 vqfrsm;
    u16 vqgran;
    u8  rsvd48[16];
    u32 vifrt;
    u32 virfa;
    u16 virfap;
    u16 viprt;
    u16 vifrsm;
    u16 vigran;
    u8  rsvd80[4016];
} nvme_id_prim_ctrl_cap_t;

typedef struct PACKED __nvme_sec_ctrl_entry {
    u16 scid;
    u16 pcid;
    u8  scs;
    u8  rsvd5[3];
    u16 vfn;
    u16 nvq;
    u16 nvi;
    u8  rsvd4[18];
} nvme_sec_ctrl_entry_t;

typedef struct PACKED __nvme_id_sec_ctrl_list {
    u8  nr;
    u8  rsvd1[31];
    nvme_sec_ctrl_entry_t   entries[127];
} nvme_id_sec_ctrl_list_t;

extern char nvme_id_prim_ctrl_cap_t_size[(sizeof (nvme_id_prim_ctrl_cap_t) == 4096 ? 0 : -1)];
extern char nvme_sec_ctrl_entry_t_size[(sizeof (nvme_sec_ctrl_entry_t) == 32 ? 0 : -1)];
extern char nvme_id_sec_ctrl_list_t_size[(sizeof (nvme_id_sec_ctrl_list_t) == 4096 ? 0 : -1)];

#endif /* __ADMIN_VIRTUAL_MGMT_NVME_DS_H__ */

