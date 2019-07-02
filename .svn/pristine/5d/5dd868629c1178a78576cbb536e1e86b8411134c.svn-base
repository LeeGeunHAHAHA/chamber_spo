/**
 * @file debug_struct.h
 * @date 2018. 11. 30.
 * @author
 */
#ifndef __DEBUG_STRUCT_H__
#define __DEBUG_STRUCT_H__

#define DEBUG_STRUCT_VER            (0)

typedef struct __cell_status {
    u32 pc[5];
    u32 fifo_status;
    u32 idle_fifo_status;
    u32 status_reg;
    u32 reg[32];
    u32 dbg_int_stat;
} cell_status_t;

typedef struct __nvme_status {
    u32 intsc;
    u32 dbg_fifo_stat_cell_0_valid;
    u32 dbg_fifo_stat_cell_0_ready;
    u32 dbg_fifo_stat_ctrl_valid;
    u32 dbg_fifo_stat_ctrl_ready;
    cell_status_t cell_status[4];
} nvme_status_t;

typedef struct __hxl_status {
    u32 intsc;
    u32 stat[10];
    u32 num_write_cmd;
    u32 num_read_cmd;
    u32 num_flush_cmd;
    u32 num_dealloc_cmd;
    u32 num_wr_req_id;
    u32 num_wr_desc_id;
    u32 num_rd_req_id;
    u32 num_rd_desc_id;
    u32 num_nvme_cmd_id;
    u32 num_error_cpl;
    u32 dbg_ctrl_net;
    u32 dbg_fifo_stat_cell_0_valid;
    u32 dbg_fifo_stat_cell_0_ready;
    u32 dbg_fifo_stat_ctrl_valid;
    u32 dbg_fifo_stat_ctrl_ready;
    cell_status_t cell_status[8];
} hxl_status_t;

typedef struct __fal_status {
    u32 intsc_0;
    u32 intsc_1;
    u32 num_pfl;
    u32 pfl_status0[16];
    u32 pfl_status1[16];
    u32 host_write_count;
    u32 host_read_count;
    u32 gc_write_count;
    u32 gc_read_count;
    u32 host_dealloc_count;
    u32 nil_erase_count;
    u32 nil_program_count;
    u32 nil_read_count;
    u32 hxl_wr_req_queued;
    u32 gc_wr_req_queued;
    u32 gc_abort_count;
    u32 fg_buffering_count;
    u32 buf_read_count;
    u32 dbg_ctrl_net;
    u32 dbg_fifo_stat_cell_0_valid;
    u32 dbg_fifo_stat_cell_0_ready;
    u32 dbg_fifo_stat_cell_1_valid;
    u32 dbg_fifo_stat_cell_1_ready;
    u32 dbg_fifo_stat_ctrl_valid;
    u32 dbg_fifo_stat_ctrl_ready;
    u32 num_channel;
    u32 nil_produce_packet_count[8];
    u32 nil_consume_packet_count[8];
    u32 nil_produce_group_count[8];
    u32 nil_consume_group_count[8];
    cell_status_t cell_status[16];
    u32 gc_last_ratio_by_validity;
} fal_status_t;

typedef struct __nil_status {
    u32 cmd_list_full;
    u32 cmd_list_empty;
    u32 queued_packet_count_ch0to3;
    u32 queued_packet_count_ch4to7;
    u32 cell_core_start;
    u32 dmem_cmd_slot_size;
    u32 do_not_use_cmd_slot3to0;
    u32 power_off_detected;
    u32 dll_update_timer_precision;
    u32 page_mask_12to8;
    u32 page_12to8_valid_bits;
    u32 page_offset_within_ppa;
    u32 channel_offset;
    u32 channel_valid_bits;
    u32 lun_offset;
    u32 lun_valid_bits;
    u32 page_size;
    u32 bf_clk_gate_always_open;
    u32 ms0_clk_gate_always_open;
    u32 ms1_clk_gate_always_open;
    u32 dma_cmd_interval;
    u32 enc_clk_gate_always_open;
    u32 rnb_retry_latency;
    u32 hp_first_threshold;
    u32 bit_error_injection_enable;
    u32 bit_error_injection_period;
    u32 ldpc_bf_max_iteration;
    u32 ldpc_msh_max_iteration;
    u32 ldpc_mss_max_iteration;
    u32 llr_mag_bit0;
    u32 llr_mag_bit1;
    u32 ldpc_bf_threshold_1to16;
    u32 ldpc_bf_threshold_17to24;
    u32 boot_mode;
    u32 in_order_ldpc_dec;
    u32 ldpc_ms_disable;
    u32 ldpc_ms_core1_enable;
    u32 ldpc_bf_disable;
    u32 spare_488b;
    u32 crc16to9_valid;
    u32 ldpc_msh_mode;
    u32 ldpc_msh_p_error_thres0;
    u32 ldpc_msh_p_error_thres1;
    u32 ldpc_msh_p_error_thres2;
    u32 ldpc_msh_max_iter1;
    u32 ldpc_msh_max_iter2;
    u32 ldpc_msh_max_iter3;
    u32 queue_rd_valid_dw[3];
    u32 queue_wr_ready_dw[3];
    u32 dfi_init_complete;
    u32 num_channel;
    u32 cdns_phy_dll_master_ctrl_reg[8];
    u32 cdns_phy_dll_obs_reg[8];
    cell_status_t cell_status[8];
} nil_status_t;

#endif /* __DEBUG_STRUCT_H__ */

