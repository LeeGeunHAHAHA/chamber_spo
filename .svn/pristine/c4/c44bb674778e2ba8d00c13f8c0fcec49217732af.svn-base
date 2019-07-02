/**
 * @file nvme.h
 * @date 2016. 06. 11.
 * @author kmsun
 *
 * nvme data structure definition
 */
#ifndef __NBIO_NVME_H__
#define __NBIO_NVME_H__

#include "types.h"

enum {
    NVME_LOG_ERROR          = 0x01,
    NVME_LOG_SMART          = 0x02,
    NVME_LOG_FW_SLOT        = 0x03,
    NVME_LOG_RESERVATION    = 0x80,
};

struct PACKED nvme_id_power_state {
    u16 max_power;
    u8  rsvd2;
    u8  flags;
    u32 entry_lat;
    u32 exit_lat;
    u8  read_tpu;
    u8  read_lat;
    u8  write_put;
    u8  write_lat;
    u16 idle_power;
    u8  idle_scale;
    u8  rsvd19;
    u16 active_power;
    u8  active_work_scale;
    u8  rsvd23[9];
};

struct PACKED nvme_id_ctrl {
    u16 vid;
    u16 ssvid;
    char sn[20];
    char mn[40];
    char fr[8];
    u8  rab;
    u8  ieee[3];
    u8  mic;
    u8  mdts;
    u16 cntlid;
    u32 ver;
    u8  rsvd84[172];
    u16 oacs;
    u8  acl;
    u8  aerl;
    u8  frmw;
    u8  lpa;
    u8  elpe;
    u8  npss;
    u8  avscc;
    u8  apsta;
    u16 wctemp;
    u16 cctemp;
    u8  rsvd270[242];
    u8  seqes;
    u8  cqes;
    u8  rsvd514[2];
    u32 nn;
    u16 oncs;
    u16 fuses;
    u8  fna;
    u8  vwc;
    u16 awun;
    u16 awupf;
    u8  nvscc;
    u8  rsvd531;
    u16 acwu;
    u8  rsvd534[2];
    u32 sgls;
    u8  rsvd540[1508];
    struct nvme_id_power_state  psd[32];
    u8  vs[1024];
};

struct PACKED nvme_lbaf {
    u16 ms;
    u8  lbads;
    u8  rp;
};

typedef struct PACKED nvme_id_ns {
    u64 nsze;
    u64 ncap;
    u64 nuse;
    u8  nsfeat;
    u8  nlbaf;
    u8  flbas;
    u8  mc;
    u8  dpc;
    u8  dps;
    u8  nmic;
    u8  rescap;
    u8  fpi;
    u8  rsvd33;
    u16 nawun;
    u16 nawupf;
    u16 nacwu;
    u16 nabsn;
    u16 nabo;
    u16 nabspf;
    u16 rsvd46;
    u64 nvmcap[2];
    u8  rsvd64[40];
    u8  nguid[16];
    u8  eui64[8];
    struct nvme_lbaf lbaf[16];
    u8  rsvd192[192];
    u8  vs[3712];
}nvme_id_ns_t;

typedef struct PACKED nvme_smart_log {
    u8  critical_warning;
    u16 composite_temperature;
    u8  avail_spare;
    u8  spare_thresh;
    u8  percent_used;
    u8  rsvd6[26];
    u8  data_units_read[16];
    u8  data_units_written[16];
    u8  host_reads[16];
    u8  host_writes[16];
    u8  ctrl_busy_time[16];
    u8  power_cycles[16];
    u8  power_on_hours[16];
    u8  unsafe_shutdowns[16];
    u8  media_errors[16];
    u8  num_err_log_entries[16];
    u32 wc_temp_time;
    u32 cc_temp_time;
    u16 temperatures[8];
    u8  rsvd216[296];
} nvme_smart_log_t;

#endif /* __NBIO_NVME_H__ */

