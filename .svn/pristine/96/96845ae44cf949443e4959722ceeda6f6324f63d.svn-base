#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "vu_common.h"
#include "debug_struct.h"

#define VU_DATA_BUF_SIZE            (24 * 1024)

FILE *fp_log;

void print_cell_status(cell_status_t *p_cell_status, char *cell_name, u32 cluster, u32 cell)
{
    u32 i;

    if (cell_name != NULL) {
        LOG("%s[%s]\r\nCELL_CORE(%d,%d): ", KNRM, cell_name, cluster, cell);
    }

    // PC
    for (i = 1; i < 5; i++) {
        if (p_cell_status->pc[0] != p_cell_status->pc[i]) {
            break;
        }
    }

    if (i == 5) {
        LOG("%sPC(%s0x%08x%s) ", KNRM, KRED, p_cell_status->pc[0], KNRM);
    } else {
        LOG("%sPC(0x%08x) ", KNRM, p_cell_status->pc[0]);
    }

    // Port Status
    if (p_cell_status->fifo_status == p_cell_status->idle_fifo_status) {
        LOG("%sFIFO(0x%08x) ", KNRM, p_cell_status->fifo_status);
    } else {
        LOG("%sFIFO(%s0x%08x", KNRM, KRED, p_cell_status->fifo_status);
        u32 i;
        for (i = 0; i < 16; i++) {
            if ((p_cell_status->fifo_status ^ p_cell_status->idle_fifo_status) & (1 << i)) {
                LOG("%s ip%d", KRED, i);
            }
        }
        for (i = 16; i < 32; i++) {
            if ((p_cell_status->fifo_status ^ p_cell_status->idle_fifo_status) & (1 << i)) {
                LOG("%s op%d", KRED, i - 16);
            }
        }
        LOG("%s) ", KNRM);
    }

    // SR.S flag
    LOG("%sSR.S(0x%08x) ", KNRM, p_cell_status->status_reg);

    // INT_STAT
    LOG("%sINT(0x%08x)\r\n", KNRM, p_cell_status->dbg_int_stat);

    // Registers
    u32 i_reg;
    for (i_reg = 0; i_reg < 32; i_reg++) {
        if (i_reg % 4 == 0) {
            LOG("%sREG %02d: ", KNRM, i_reg);
        }
        LOG("%s0x%08x ", KNRM, p_cell_status->reg[i_reg]);
        if (i_reg % 4 == 3) {
            LOG("%s\r\n", KNRM);
        }
    }
    LOG("%s\r\n", KNRM);
}

void print_nvme_status(nvme_status_t *p_nvme_status)
{
    u32 data;

    LOG("%s[NVMe Status]\r\n", KNRM);

    /*** NVMe Register Display ***/
    LOG("%s%s: 0x%08x\r\n", KNRM, "NVME_CTRL_REG_INTSC", p_nvme_status->intsc);

    /*** Cell Tile PC ***/
    print_cell_status(&p_nvme_status->cell_status[0], "NVME_CELL_UNIQUE_ID_SQ_SCHED", 0, 0);
    print_cell_status(&p_nvme_status->cell_status[1], "NVME_CELL_UNIQUE_ID_SQ_DMA_MGR", 0, 1);
    print_cell_status(&p_nvme_status->cell_status[2], "NVME_CELL_UNIQUE_ID_CQ_DMA_MGR", 0, 2);
    print_cell_status(&p_nvme_status->cell_status[3], "NVME_CELL_UNIQUE_ID_INT_GEN", 0, 3);


    /* Debug Information */
    LOG("%s%s: 0x%08x\r\n", KNRM, "NVME_CTRL_DBG_REG_FIFO_STAT_NVME_CELL_0_VALID", p_nvme_status->dbg_fifo_stat_cell_0_valid);
    data = p_nvme_status->dbg_fifo_stat_cell_0_valid;
    LOG("%s\tVALID_SQ_DMA_FEEDBACK_OUT     :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tVALID_SQ_FETCH_REQ_OUT        :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tVALID_SQ_DMA_INFO_OUT         :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tVALID_CQ_WAIT_QUEUE_OUT       :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tVALID_CQ_MGR_INTR_CLR_OUT     :%d\r\n", KNRM, (data >> 4) & 1);
    LOG("%s\tVALID_TIMER_EVENT             :%d\r\n", KNRM, (data >> 5) & 1);
    LOG("%s\tVALID_T0_CELL_IOCTL_INT_1_OUT :%d\r\n", KNRM, (data >> 6) & 1);
    LOG("%s\tVALID_T0_CELL_IOCTL_INT_3_OUT :%d\r\n", KNRM, (data >> 7) & 1);

    LOG("%s%s: 0x%08x\r\n", KNRM, "NVME_CTRL_DBG_REG_FIFO_STAT_NVME_CELL_0_READY", p_nvme_status->dbg_fifo_stat_cell_0_ready);
    data = p_nvme_status->dbg_fifo_stat_cell_0_ready;
    LOG("%s\tREADY_SQ_DMA_FEEDBACK_OUT     :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tREADY_SQ_FETCH_REQ_OUT        :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tREADY_SQ_DMA_INFO_OUT         :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tREADY_CQ_WAIT_QUEUE_OUT       :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tREADY_CQ_MGR_INTR_CLR_OUT     :%d\r\n", KNRM, (data >> 4) & 1);
    LOG("%s\tREADY_T0_CELL_IOCTL_INT_1_OUT :%d\r\n", KNRM, (data >> 6) & 1);
    LOG("%s\tREADY_T0_CELL_IOCTL_INT_3_OUT :%d\r\n", KNRM, (data >> 7) & 1);

    LOG("%s%s: 0x%08x\r\n", KNRM, "NVME_CTRL_DBG_REG_FIFO_STAT_NVME_CTRL_VALID", p_nvme_status->dbg_fifo_stat_ctrl_valid);
    data = p_nvme_status->dbg_fifo_stat_ctrl_valid;
    LOG("%s\tVALID_SQT_DBL_OUT             :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tVALID_SQ_DMA_REQ_OUT          :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tVALID_SQ_DMA_DATA_OUT         :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tVALID_SQ_DMA_REQ_INFO_OUT     :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tVALID_CQ_DMA_REQ_SAFE         :%d\r\n", KNRM, (data >> 4) & 1);
    LOG("%s\tVALID_CQ_DMA_CPL_OUT          :%d\r\n", KNRM, (data >> 5) & 1);
    LOG("%s\tVALID_PCIE_CQ_INTR_CLR        :%d\r\n", KNRM, (data >> 8) & 1);
    LOG("%s\tVALID_CQH_DBL_OUT             :%d\r\n", KNRM, (data >> 9) & 1);
    LOG("%s\tVALID_CMB_SQ_DMA_DATA         :%d\r\n", KNRM, (data >> 10) & 1);
    LOG("%s\tVALID_PCIE_SQ_DMA_DATA        :%d\r\n", KNRM, (data >> 11) & 1);
    LOG("%s\tVALID_CMB_CQ_DMA_CPL          :%d\r\n", KNRM, (data >> 12) & 1);
    LOG("%s\tVALID_PCIE_CQ_DMA_CPL         :%d\r\n", KNRM, (data >> 13) & 1);
    LOG("%s\tVALID_CQ_DMA_REQ_INFO_OUT     :%d\r\n", KNRM, (data >> 15) & 1);

    LOG("%s%s: 0x%08x\r\n", KNRM, "NVME_CTRL_DBG_REG_FIFO_STAT_NVME_CTRL_READY", p_nvme_status->dbg_fifo_stat_ctrl_ready);
    data = p_nvme_status->dbg_fifo_stat_ctrl_ready;
    LOG("%s\tREADY_SQT_DBL                 :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tREADY_SQ_DMA_REQ              :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tREADY_SQ_DMA_DATA             :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tREADY_SQ_DMA_REQ_INFO         :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tREADY_CQ_DMA_REQ              :%d\r\n", KNRM, (data >> 4) & 1);
    LOG("%s\tREADY_CQ_DMA_CPL              :%d\r\n", KNRM, (data >> 5) & 1);
    LOG("%s\tREADY_PCIE_CQ_INTR            :%d\r\n", KNRM, (data >> 6) & 1);
    LOG("%s\tREADY_CQH_DBL                 :%d\r\n", KNRM, (data >> 7) & 1);
    LOG("%s\tREADY_CQH_DBL_INTA_CLR        :%d\r\n", KNRM, (data >> 8) & 1);
    LOG("%s\tREADY_CQH_DBL_INTR_AGGR       :%d\r\n", KNRM, (data >> 9) & 1);
    LOG("%s\tREADY_CMB_SQ_DMA_REQ          :%d\r\n", KNRM, (data >> 10) & 1);
    LOG("%s\tREADY_PCIE_SQ_DMA_REQ         :%d\r\n", KNRM, (data >> 11) & 1);
    LOG("%s\tREADY_CMB_CQ_DMA_REQ          :%d\r\n", KNRM, (data >> 12) & 1);
    LOG("%s\tREADY_PCIE_CQ_DMA_REQ         :%d\r\n", KNRM, (data >> 13) & 1);
    LOG("%s\tREADY_PCIE_CQ_DMA_DATA        :%d\r\n", KNRM, (data >> 14) & 1);
    LOG("%s\tREADY_CQ_DMA_REQ_INFO         :%d\r\n", KNRM, (data >> 15) & 1);
    LOG("%s\r\n", KNRM);
}

void print_hxl_status(hxl_status_t *p_hxl_status)
{
    u32 data;

    LOG("%s[HXL Status]\r\n", KNRM);

    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_INTSC", p_hxl_status->intsc);

    /*** SMART INFO ***/
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_DATA_UNIT_READ_512B", p_hxl_status->stat[0]);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_DATA_UNIT_WRITE_512B", p_hxl_status->stat[1]);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_HOST_READ_COMPLETION", p_hxl_status->stat[2]);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_HOST_WRITE_COMPLETION", p_hxl_status->stat[3]);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_CONTROLLER_BUSY_USEC", p_hxl_status->stat[4]);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_POWER_ON_USEC", p_hxl_status->stat[5]);

    LOG("%s           - %s: %d\r\n", KNRM, "HXL_HOST_READ_COMMAND", p_hxl_status->stat[6]);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_HOST_WRITE_COMMAND", p_hxl_status->stat[7]);

    LOG("%s           - %s: %d\r\n", KNRM, "HXL_OUTSTANDING_READ_ID", p_hxl_status->stat[8] & 0xFFFF);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_OUTSTANDING_WRITE_ID", p_hxl_status->stat[8] >> 16);

    LOG("%s           - %s: %d\r\n", KNRM, "HXL_OUTSTANDING_RD_DESC_ID", p_hxl_status->stat[9] & 0xFFFF);
    LOG("%s           - %s: %d\r\n", KNRM, "HXL_OUTSTANDING_WR_DESC_ID", p_hxl_status->stat[9] >> 16);


    /*** HW Monitoring */
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_WRITE_CMD", p_hxl_status->num_write_cmd);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_READ_CMD", p_hxl_status->num_read_cmd);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_FLUSH_CMD", p_hxl_status->num_flush_cmd);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_DEALLOC_CMD", p_hxl_status->num_dealloc_cmd);

    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_WR_REQ_ID", p_hxl_status->num_wr_req_id);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_WR_DESC_ID", p_hxl_status->num_wr_desc_id);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_RD_REQ_ID", p_hxl_status->num_rd_req_id);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_RD_DESC_ID", p_hxl_status->num_rd_desc_id);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_NVME_CMD_ID", p_hxl_status->num_nvme_cmd_id);

    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_REG_MONITOR_NUM_ERROR_CPL", p_hxl_status->num_error_cpl);


    /*** Cell Tile PC ***/
    print_cell_status(&p_hxl_status->cell_status[0], "HXL_CELL_UNIQUE_ID_DECODER", 0, 0);
    print_cell_status(&p_hxl_status->cell_status[1], "HXL_CELL_UNIQUE_ID_DESC_PARSER", 0, 1);
    print_cell_status(&p_hxl_status->cell_status[2], "HXL_CELL_UNIQUE_ID_DESC_SETUP", 0, 2);
    print_cell_status(&p_hxl_status->cell_status[3], "HXL_CELL_UNIQUE_ID_CHOPPER", 0, 3);
    print_cell_status(&p_hxl_status->cell_status[4], "HXL_CELL_UNIQUE_ID_STATUS_MGR", 1, 0);
    print_cell_status(&p_hxl_status->cell_status[5], "HXL_CELL_UNIQUE_ID_CPL_THROTTLER", 1, 1);
    print_cell_status(&p_hxl_status->cell_status[6], "HXL_CELL_UNIQUE_ID_VF_SCHED", 1, 2);

    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_PS_REG_DBG_CTRL_NET", p_hxl_status->dbg_ctrl_net);
    data = p_hxl_status->dbg_ctrl_net;
    LOG("%s\tWR_REQ_ID_VALID     :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tRD_REQ_ID_VALID     :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tWR_DESC_ID_VALID    :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tRD_DESC_ID_VALID    :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tNVME_CMD_ID_VALID   :%d\r\n", KNRM, (data >> 4) & 1);

    LOG("%s\tWR_REQ_ID_CONFLICT  :%d\r\n", KNRM, (data >> 8) & 1);
    LOG("%s\tRD_REQ_ID_CONFLICT  :%d\r\n", KNRM, (data >> 9) & 1);
    LOG("%s\tWR_DESC_ID_CONFLICT :%d\r\n", KNRM, (data >> 10) & 1);
    LOG("%s\tRD_DESC_ID_CONFLICT :%d\r\n", KNRM, (data >> 11) & 1);
    LOG("%s\tNVME_CMD_ID_CONFLICT:%d\r\n", KNRM, (data >> 12) & 1);

    LOG("%s\tDESC_REQ_PKT_IDX    :%d\r\n", KNRM, (data >> 16) & 7);

    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_PS_REG_DBG_FIFO_STAT_CELL_0_VALID", p_hxl_status->dbg_fifo_stat_cell_0_valid);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_PS_REG_DBG_FIFO_STAT_CELL_0_READY", p_hxl_status->dbg_fifo_stat_cell_0_ready);

    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_PS_REG_DBG_FIFO_STAT_CTRL_VALID", p_hxl_status->dbg_fifo_stat_ctrl_valid);
    LOG("%s%s: 0x%08x\r\n", KNRM, "HXL_PS_REG_DBG_FIFO_STAT_CTRL_READY", p_hxl_status->dbg_fifo_stat_ctrl_ready);
    LOG("%s\r\n", KNRM);
}

void print_fal_status(fal_status_t *p_fal_status)
{
    u32 data;
    u32 delta;
    u32 i;

    LOG("%s\r\n\r\n[FAL status]\r\n", KNRM);

    /* Parse Register Values */
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_INTSC_0", p_fal_status->intsc_0);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_INTSC_1", p_fal_status->intsc_1);

    /*** PFL ***/
    u32 i_pfl;
    for (i_pfl = 0; i_pfl < p_fal_status->num_pfl; i_pfl++) {
        u32 pfl_status0;
        u32 pfl_status1;
        pfl_status0 = p_fal_status->pfl_status0[i_pfl];
        pfl_status1 = p_fal_status->pfl_status1[i_pfl];
        LOG("%sPFL #%2d: Slot0(%d, BlockCnt:%3d) Slot1(%d, BlockCnt:%3d) IDX(%3d) WLN(%4d)\r\n", KNRM, i_pfl,
                (pfl_status0 & 0x8000) >> 15,
                pfl_status0 & 0x7FFF,
                ((pfl_status0 >> 16) & 0x8000) >> 15,
                (pfl_status0 >> 16) & 0x7FFF,
                (pfl_status1 >> 16) & 0xFFFF,
                pfl_status1 & 0xFFFF);
    }

    // HW Monitoring
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_HOST_WRITE_COUNT", p_fal_status->host_write_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_HOST_READ_COUNT", p_fal_status->host_read_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_GC_WRITE_COUNT", p_fal_status->gc_write_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_GC_READ_COUNT", p_fal_status->gc_read_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_HOST_DEALLOC_COUNT", p_fal_status->host_dealloc_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_NIL_ERASE_COUNT", p_fal_status->nil_erase_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_NIL_PROGRAM_COUNT", p_fal_status->nil_program_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_NIL_READ_COUNT", p_fal_status->nil_read_count);

    /*** SCHEDULER_MONITOR ***/
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_HXL_WR_REQ_QUEUED", p_fal_status->hxl_wr_req_queued);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_GC_WR_REQ_QUEUED", p_fal_status->gc_wr_req_queued);

    /*** FAL_PS_REG_MONITOR ***/
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_GC_ABORT_COUNT", p_fal_status->gc_abort_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_FG_BUFFERING_COUNT", p_fal_status->fg_buffering_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_MONITOR_BUF_READ_COUNT", p_fal_status->buf_read_count);
    LOG("%s%s: 0x%08x\r\n", KNRM, "GC_LAST_RATIO_BY_VALIDITY", p_fal_status->gc_last_ratio_by_validity);
    LOG("%s\r\n", KNRM);

    /*** Cell Tile PC ***/
    print_cell_status(&p_fal_status->cell_status[0], "FAL_CELL_UNIQUE_ID_DEP_CHECK_FRONT", 0, 0);
    print_cell_status(&p_fal_status->cell_status[1], "FAL_CELL_UNIQUE_ID_DEP_CHECK_BACK", 0, 1);
    print_cell_status(&p_fal_status->cell_status[2], "FAL_CELL_UNIQUE_ID_MAP_LOOKUP_FRONT", 0, 2);
    print_cell_status(&p_fal_status->cell_status[3], "FAL_CELL_UNIQUE_ID_MAP_LOOKUP_BACK", 0, 3);
    print_cell_status(&p_fal_status->cell_status[4], "FAL_CELL_UNIQUE_ID_WRITE_MERGER", 1, 0);
    print_cell_status(&p_fal_status->cell_status[5], "FAL_CELL_UNIQUE_ID_READ_MERGER", 1, 1);
    print_cell_status(&p_fal_status->cell_status[6], "FAL_CELL_UNIQUE_ID_REQUEST_GENERATOR", 1, 2);
    print_cell_status(&p_fal_status->cell_status[7], "FAL_CELL_UNIQUE_ID_COMPLETION_HANDLER", 2, 0);
    print_cell_status(&p_fal_status->cell_status[8], "FAL_CELL_UNIQUE_ID_MW_APPEND", 2, 1);
    print_cell_status(&p_fal_status->cell_status[9], "FAL_CELL_UNIQUE_ID_QOS_MANAGER", 2, 2);
    print_cell_status(&p_fal_status->cell_status[10], "FAL_CELL_UNIQUE_ID_MW_REPLAY", 3, 0);
    print_cell_status(&p_fal_status->cell_status[11], "FAL_CELL_UNIQUE_ID_MAP_UPDATE_FRONT", 3, 1);
    print_cell_status(&p_fal_status->cell_status[12], "FAL_CELL_UNIQUE_ID_MAP_UPDATE_MID", 3, 2);
    print_cell_status(&p_fal_status->cell_status[13], "FAL_CELL_UNIQUE_ID_MAP_UPDATE_BACK", 3, 3);

    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_CTRL_NET", p_fal_status->dbg_ctrl_net);
    data = p_fal_status->dbg_ctrl_net;
    LOG("%s\tpfl_stop_write_request :%03x\r\n", KNRM, (data >> 0) & 0x1FF);
    LOG("%s\tnil_req_id_valid       :%d\r\n", KNRM, (data >> 16) & 1);
    LOG("%s\twr_idt_idx_valid       :%d\r\n", KNRM, (data >> 17) & 1);
    LOG("%s\trd_idt_idx_valid       :%d\r\n", KNRM, (data >> 18) & 1);
    LOG("%s\tnil_req_id_conflict    :%d\r\n", KNRM, (data >> 20) & 1);
    LOG("%s\twr_idt_idx_conflict    :%d\r\n", KNRM, (data >> 21) & 1);
    LOG("%s\trd_idt_idx_conflict    :%d\r\n", KNRM, (data >> 22) & 1);
    LOG("%s\treorder_buf_id_conflict:%d\r\n", KNRM, (data >> 23) & 1);

    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_FIFO_STAT_CELL_0_VALID", p_fal_status->dbg_fifo_stat_cell_0_valid);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_FIFO_STAT_CELL_0_READY", p_fal_status->dbg_fifo_stat_cell_0_ready);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_FIFO_STAT_CELL_1_VALID", p_fal_status->dbg_fifo_stat_cell_1_valid);
    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_FIFO_STAT_CELL_1_READY", p_fal_status->dbg_fifo_stat_cell_1_ready);

    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_FIFO_STAT_CTRL_VALID", p_fal_status->dbg_fifo_stat_ctrl_valid);
    data = p_fal_status->dbg_fifo_stat_ctrl_valid;
    LOG("%s\tvalid_fal_req_chained_out;    :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tvalid_fal_req_dep_out;        :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tvalid_fal_hp_req_out;         :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tvalid_fal_cpl_out;            :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tvalid_cbf_req_out;            :%d\r\n", KNRM, (data >> 4) & 1);
    LOG("%s\tvalid_cbf_ack_out;            :%d\r\n", KNRM, (data >> 5) & 1);
    LOG("%s\tvalid_cbf_delete_out;         :%d\r\n", KNRM, (data >> 6) & 1);
    LOG("%s\tvalid_need_pfl_req_out;       :%d\r\n", KNRM, (data >> 7) & 1);
    LOG("%s\tvalid_need_pfl_cpl_out;       :%d\r\n", KNRM, (data >> 8) & 1);
    LOG("%s\tvalid_map_int_cpl_out;        :%d\r\n", KNRM, (data >> 9) & 1);
    LOG("%s\tvalid_fal_buf_read_req_out;   :%d\r\n", KNRM, (data >> 10) & 1);
    LOG("%s\tvalid_read_count_req_out;     :%d\r\n", KNRM, (data >> 11) & 1);
    LOG("%s\tvalid_mw_notify_out;          :%d\r\n", KNRM, (data >> 12) & 1);
    LOG("%s\tvalid_ack_notify_out;         :%d\r\n", KNRM, (data >> 13) & 1);
    LOG("%s\tvalid_fg_buf_req_id_out;      :%d\r\n", KNRM, (data >> 14) & 1);
    LOG("%s\tVALID_PL_DDR_MAP_LK_RD_REQ;   :%d\r\n", KNRM, (data >> 15) & 1);
    LOG("%s\tvalid_pl_ddr_map_lk_rd_data;  :%d\r\n", KNRM, (data >> 16) & 1);
    LOG("%s\tVALID_PL_DDR_META_WR_REQ;     :%d\r\n", KNRM, (data >> 17) & 1);
    LOG("%s\tvalid_pl_ddr_meta_wr_cpl;     :%d\r\n", KNRM, (data >> 18) & 1);
    LOG("%s\tVALID_PL_DDR_MW_WR_REQ;       :%d\r\n", KNRM, (data >> 19) & 1);
    LOG("%s\tvalid_pl_ddr_mw_wr_cpl;       :%d\r\n", KNRM, (data >> 20) & 1);
    LOG("%s\tVALID_PL_DDR_MW_RD_REQ;       :%d\r\n", KNRM, (data >> 21) & 1);
    LOG("%s\tvalid_pl_ddr_mw_rd_data;      :%d\r\n", KNRM, (data >> 22) & 1);
    LOG("%s\tVALID_PL_DDR_MAP_UP_WR_REQ;   :%d\r\n", KNRM, (data >> 23) & 1);
    LOG("%s\tvalid_pl_ddr_map_up_wr_cpl;   :%d\r\n", KNRM, (data >> 24) & 1);
    LOG("%s\tVALID_PL_DDR_MAP_UP_RD_REQ;   :%d\r\n", KNRM, (data >> 25) & 1);
    LOG("%s\tvalid_pl_ddr_map_up_rd_data   :%d\r\n", KNRM, (data >> 26) & 1);
    LOG("%s\tVALID_PL_DDR_METAINFO_WR_REQ  :%d\r\n", KNRM, (data >> 27) & 1);
    LOG("%s\tvalid_pl_ddr_metainfo_wr_cpl  :%d\r\n", KNRM, (data >> 28) & 1);
    LOG("%s\tVALID_PL_DDR_METAINFO_RD_REQ  :%d\r\n", KNRM, (data >> 29) & 1);
    LOG("%s\tvalid_pl_ddr_metainfo_rd_data :%d\r\n", KNRM, (data >> 30) & 1);

    LOG("%s%s: 0x%08x\r\n", KNRM, "FAL_PS_REG_DBG_FIFO_STAT_CTRL_READY", p_fal_status->dbg_fifo_stat_ctrl_ready);
    data = p_fal_status->dbg_fifo_stat_ctrl_ready;
    LOG("%s\tready_fal_req_chained;        :%d\r\n", KNRM, (data >> 0) & 1);
    LOG("%s\tready_fal_req_dep;            :%d\r\n", KNRM, (data >> 1) & 1);
    LOG("%s\tready_fal_hp_req;             :%d\r\n", KNRM, (data >> 2) & 1);
    LOG("%s\tready_fal_cpl;                :%d\r\n", KNRM, (data >> 3) & 1);
    LOG("%s\tready_cbf_req;                :%d\r\n", KNRM, (data >> 4) & 1);
    LOG("%s\tready_cbf_ack;                :%d\r\n", KNRM, (data >> 5) & 1);
    LOG("%s\tready_cbf_delete;             :%d\r\n", KNRM, (data >> 6) & 1);
    LOG("%s\tready_need_pfl_req;           :%d\r\n", KNRM, (data >> 7) & 1);
    LOG("%s\tready_need_pfl_cpl;           :%d\r\n", KNRM, (data >> 8) & 1);
    LOG("%s\tready_map_int_cpl;            :%d\r\n", KNRM, (data >> 9) & 1);
    LOG("%s\tready_fal_buf_read_req;       :%d\r\n", KNRM, (data >> 10) & 1);
    LOG("%s\tready_read_count_req;         :%d\r\n", KNRM, (data >> 11) & 1);
    LOG("%s\tready_mw_notify;              :%d\r\n", KNRM, (data >> 12) & 1);
    LOG("%s\tready_ack_notify;             :%d\r\n", KNRM, (data >> 13) & 1);
    LOG("%s\tready_fg_buf_req_id;          :%d\r\n", KNRM, (data >> 14) & 1);
    LOG("%s\tready_pl_ddr_map_lk_rd_req;   :%d\r\n", KNRM, (data >> 15) & 1);
    LOG("%s\tREADY_PL_DDR_MAP_LK_RD_DATA;  :%d\r\n", KNRM, (data >> 16) & 1);
    LOG("%s\tready_pl_ddr_meta_wr_req;     :%d\r\n", KNRM, (data >> 17) & 1);
    LOG("%s\tREADY_PL_DDR_META_WR_CPL;     :%d\r\n", KNRM, (data >> 18) & 1);
    LOG("%s\tready_pl_ddr_mw_wr_req;       :%d\r\n", KNRM, (data >> 19) & 1);
    LOG("%s\tREADY_PL_DDR_MW_WR_CPL;       :%d\r\n", KNRM, (data >> 20) & 1);
    LOG("%s\tready_pl_ddr_mw_rd_req;       :%d\r\n", KNRM, (data >> 21) & 1);
    LOG("%s\tREADY_PL_DDR_MW_RD_DATA;      :%d\r\n", KNRM, (data >> 22) & 1);
    LOG("%s\tready_pl_ddr_map_up_wr_req;   :%d\r\n", KNRM, (data >> 23) & 1);
    LOG("%s\tREADY_PL_DDR_MAP_UP_WR_CPL;   :%d\r\n", KNRM, (data >> 24) & 1);
    LOG("%s\tready_pl_ddr_map_up_rd_req;   :%d\r\n", KNRM, (data >> 25) & 1);
    LOG("%s\tREADY_PL_DDR_MAP_UP_RD_DATA;  :%d\r\n", KNRM, (data >> 26) & 1);
    LOG("%s\tready_pl_ddr_metainfo_wr_req; :%d\r\n", KNRM, (data >> 27) & 1);
    LOG("%s\tREADY_PL_DDR_METAINFO_WR_CPL; :%d\r\n", KNRM, (data >> 28) & 1);
    LOG("%s\tready_pl_ddr_metainfo_rd_req; :%d\r\n", KNRM, (data >> 29) & 1);
    LOG("%s\tREADY_PL_DDR_METAINFO_RD_DATA;:%d\r\n", KNRM, (data >> 30) & 1);

    LOG("%s\r\n", KNRM);
    for (i = 0; i < p_fal_status->num_channel; i++) {
        if (p_fal_status->nil_produce_packet_count[i] >= p_fal_status->nil_consume_packet_count[i]) {
            delta = p_fal_status->nil_produce_packet_count[i] - p_fal_status->nil_consume_packet_count[i];
        } else {
            delta = (0xFFFFFFFF - p_fal_status->nil_consume_packet_count[i]);
            delta += p_fal_status->nil_produce_packet_count[i] + 1;
        }

        LOG("%s\t[Channel %d] Outstanding NIL cmd      :%d (produce %d) (consume %d)\r\n", KNRM,
            i, delta, p_fal_status->nil_produce_packet_count[i], p_fal_status->nil_consume_packet_count[i]);
    }

    LOG("%s\r\n", KNRM);
    for (i = 0; i < p_fal_status->num_channel; i++) {
        if (p_fal_status->nil_produce_group_count[i] >= p_fal_status->nil_consume_group_count[i]) {
            delta = p_fal_status->nil_produce_group_count[i] - p_fal_status->nil_consume_group_count[i];
        } else {
            delta = (0xFFFFFFFF - p_fal_status->nil_consume_group_count[i]);
            delta += p_fal_status->nil_produce_group_count[i] + 1;
        }

        LOG("%s\t[Channel %d] Outstanding NIL cmd group:%d (produce %d) (consume %d)\r\n", KNRM,
            i, delta, p_fal_status->nil_produce_group_count[i], p_fal_status->nil_consume_group_count[i]);
    }
    LOG("%s\r\n", KNRM);
}

void print_nil_status(nil_status_t *p_nil_status)
{
    u32 data;
    u32 value;
    u32 idx;
    u32 unique_id;

    LOG("%s\r\n\r\n[NIL status]\r\n", KNRM);

    LOG("%s                       CH0  CH1  CH2  CH3  CH4  CH5  CH6  CH7\r\n", KNRM);

    data = p_nil_status->cmd_list_full;
    LOG("%scmd list full(LP)   :  ", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s  %d  ", KNRM, (data >> idx) & 0x1);
    }
    LOG("%s\r\n", KNRM);

    LOG("%scmd list full(HP)   :  ", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s  %d  ", KNRM, (data >> (idx + 8)) & 0x1);
    }
    LOG("%s\r\n", KNRM);

    LOG("%scmd list full(UP)   :  ", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s  %d  ", KNRM, (data >> (idx + 16)) & 0x1);
    }
    LOG("%s\r\n", KNRM);

    data = p_nil_status->cmd_list_empty;
    LOG("%scmd list empty(LP)  :  ", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s  %d  ", KNRM, (data >> idx) & 0x1);
    }
    LOG("%s\r\n", KNRM);

    LOG("%scmd list empty(HP)  :  ", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s  %d  ", KNRM, (data >> (idx + 8)) & 0x1);
    }
    LOG("%s\r\n", KNRM);

    LOG("%scmd list empty(UP)  :  ", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s  %d  ", KNRM, (data >> (idx + 16)) & 0x1);
    }
    LOG("%s\r\n", KNRM);


    data = p_nil_status->queued_packet_count_ch0to3;
    LOG("%squeued packet cnt   : ", KNRM);
    for (idx = 0; idx < 4; idx++) {
        LOG("%s %3d ", KNRM, (data >> (idx << 3)) & 0xFF);
    }
    data = p_nil_status->queued_packet_count_ch4to7;
    for (idx = 0; idx < p_nil_status->num_channel - 4; idx++) {
        LOG("%s %3d ", KNRM, (data >> (idx << 3)) & 0xFF);
    }
    /* status registers will be update */


    LOG("%s\r\n\r\n\r\n [NIL configuration]\r\n", KNRM);

    LOG("%sADDR_CELL_CORE_START:0x%08x\r\n", KNRM, p_nil_status->cell_core_start);
    LOG("%sDMEM_CMD_SLOT_SIZE         : %d (0: 64DWs, 1: 48DWs, others: 32DWs)\r\n", KNRM, p_nil_status->dmem_cmd_slot_size);
    LOG("%sDO_NOT_USE_CMD_SLOT3TO0    : %d\r\n", KNRM, p_nil_status->do_not_use_cmd_slot3to0);
    LOG("%sPOWER_OFF_DETECTED         : %d\r\n", KNRM, p_nil_status->power_off_detected);
    LOG("%sDLL_UPDATE_TIMER_PRECISION : %d\r\n\r\n", KNRM, p_nil_status->dll_update_timer_precision);

    LOG("%sADDR_PAGE_MASK_12TO8:0x%08x\r\n", KNRM, p_nil_status->page_mask_12to8);
    LOG("%sPAGE_12TO8_VALID_BITS      : %d\r\n", KNRM, p_nil_status->page_12to8_valid_bits);
    LOG("%sPAGE_OFFSET_WITHIN_PPA     : %d\r\n", KNRM, p_nil_status->page_offset_within_ppa);

    LOG("%sCHANNEL_OFFSET             : %d (DW1[%d:%d]\r\n", KNRM, p_nil_status->channel_offset, p_nil_status->channel_offset + 18, p_nil_status->channel_offset + 16);
    LOG("%sCHANNEL_VALID_BITS         : %d\r\n", KNRM, p_nil_status->channel_valid_bits);

    LOG("%sLUN_OFFSET                 : %d (DW1[%d:%d]\r\n", KNRM, p_nil_status->lun_offset, p_nil_status->lun_offset + 18, p_nil_status->lun_offset + 15);
    LOG("%sLUN_VALID_BITS             : %d\r\n", KNRM, p_nil_status->lun_valid_bits);
    LOG("%sPAGE_SIZE                  : %d (0: 8KB+a, 1: 16KB+a)\r\n", KNRM, p_nil_status->page_size);
    LOG("%sBF_CLK_GATE_ALWAYS_OPEN    : %d\r\n", KNRM, p_nil_status->bf_clk_gate_always_open);
    LOG("%sMS0_CLK_GATE_ALWAYS_OPEN   : %d\r\n", KNRM, p_nil_status->ms0_clk_gate_always_open);
    LOG("%sMS1_CLK_GATE_ALWAYS_OPEN   : %d\r\n", KNRM, p_nil_status->ms1_clk_gate_always_open);
    LOG("%sDMA_CMD_INTERVAL           : %d (1 tick = 32 DATA_CLK cycles)\r\n", KNRM, p_nil_status->dma_cmd_interval);
    LOG("%sENC_CLK_GATE_ALWAYS_OPEN   : %d\r\n", KNRM, p_nil_status->enc_clk_gate_always_open);

    LOG("%sRNB_RETRY_LATENCY          : %d\r\n", KNRM, p_nil_status->rnb_retry_latency);
    LOG("%sHP_FIRST_THRESHOLD         : %d\r\n", KNRM, p_nil_status->hp_first_threshold);
    LOG("%sBIT_ERROR_INJECTION_ENABLE : %d\r\n", KNRM, p_nil_status->bit_error_injection_enable);
    LOG("%sBIT_ERROR_INJECTION_PERIOD : %d (%d.%d bit injection)\r\n", KNRM, p_nil_status->bit_error_injection_period, 2304/(p_nil_status->bit_error_injection_period+1), (230400/(p_nil_status->bit_error_injection_period+1))%100);

    LOG("%sLDPC_BF_MAX_ITERATION      : %d\r\n", KNRM, p_nil_status->ldpc_bf_max_iteration);
    LOG("%sLDPC_MSH_MAX_ITERATION     : %d\r\n", KNRM, p_nil_status->ldpc_msh_max_iteration);
    LOG("%sLDPC_MSS_MAX_ITERATION     : %d\r\n", KNRM, p_nil_status->ldpc_mss_max_iteration);
    LOG("%sLLR_MAG_BIT0               : %d\r\n", KNRM, p_nil_status->llr_mag_bit0);
    LOG("%sLLR_MAG_BIT1               : %d\r\n", KNRM, p_nil_status->llr_mag_bit1);

    LOG("%sLDPC_BF_THRESHOLD_1TO16    : %d\r\n", KNRM, p_nil_status->ldpc_bf_threshold_1to16);

    LOG("%sLDPC_BF_THRESHOLD_17TO24   : %d\r\n", KNRM, p_nil_status->ldpc_bf_threshold_17to24);
    LOG("%sBOOT_MODE                  : %d\r\n", KNRM, p_nil_status->boot_mode);
    LOG("%sIN_ORDER_LDPC_DEC          : %d\r\n", KNRM, p_nil_status->in_order_ldpc_dec);
    LOG("%sLDPC_MS_DISABLE            : %d\r\n", KNRM, p_nil_status->ldpc_ms_disable);
    LOG("%sLDPC_MS_CORE1_ENABLE       : %d\r\n", KNRM, p_nil_status->ldpc_ms_core1_enable);
    LOG("%sLDPC_BF_DISABLE            : %d\r\n", KNRM, p_nil_status->ldpc_bf_disable);
    LOG("%sSPARE_488B                 : %d\r\n", KNRM, p_nil_status->spare_488b);
    LOG("%sCRC16TO9_VALID             : %d\r\n", KNRM, p_nil_status->crc16to9_valid);

    LOG("%sLDPC_MSH_MODE              : %d\r\n", KNRM, p_nil_status->ldpc_msh_mode);
    LOG("%sLDPC_MSH_P_ERROR_THRES0    : %d\r\n", KNRM, p_nil_status->ldpc_msh_p_error_thres0);
    LOG("%sLDPC_MSH_P_ERROR_THRES1    : %d\r\n", KNRM, p_nil_status->ldpc_msh_p_error_thres1);
    LOG("%sLDPC_MSH_P_ERROR_THRES2    : %d\r\n", KNRM, p_nil_status->ldpc_msh_p_error_thres2);

    LOG("%sLDPC_MSH_MAX_ITER1         : %d\r\n", KNRM, p_nil_status->ldpc_msh_max_iter1);
    LOG("%sLDPC_MSH_MAX_ITER2         : %d\r\n", KNRM, p_nil_status->ldpc_msh_max_iter2);
    LOG("%sLDPC_MSH_MAX_ITER3         : %d\r\n", KNRM, p_nil_status->ldpc_msh_max_iter3);

    LOG("%sNIL_QUEUE_RD_VALID_DW0     : 0x%08x\r\n", KNRM, p_nil_status->queue_rd_valid_dw[0]);
    LOG("%sNIL_QUEUE_RD_VALID_DW1     : 0x%08x\r\n", KNRM, p_nil_status->queue_rd_valid_dw[1]);
    LOG("%sNIL_QUEUE_RD_VALID_DW2     : 0x%08x\r\n", KNRM, p_nil_status->queue_rd_valid_dw[2]);

    LOG("%sNIL_QUEUE_WR_READY_DW0     : 0x%08x\r\n", KNRM, p_nil_status->queue_wr_ready_dw[0]);
    LOG("%sNIL_QUEUE_WR_READY_DW1     : 0x%08x\r\n", KNRM, p_nil_status->queue_wr_ready_dw[1]);
    LOG("%sNIL_QUEUE_WR_READY_DW2     : 0x%08x\r\n", KNRM, p_nil_status->queue_wr_ready_dw[2]);

    if ((p_nil_status->dfi_init_complete & 0x0000FF00) != 0) {
        LOG("%sADDR_DFI_INIT_COMPLETE      : 0x%08x\r\n", KRED, p_nil_status->dfi_init_complete);
    }
    else {
        LOG("%sADDR_DFI_INIT_COMPLETE     : 0x%08x\r\n", KNRM, p_nil_status->dfi_init_complete);
    }
    LOG("%s", KNRM);

    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%s[CH %d] phy_dll_master_ctrl_reg : %08x (param_dll_bypass_mode[23] %d)\r\n", KNRM,
                    idx, p_nil_status->cdns_phy_dll_master_ctrl_reg[idx], (p_nil_status->cdns_phy_dll_master_ctrl_reg[idx]>>23)&0x1 );

        LOG("%s[CH %d] phy_dll_obs_reg_1 : 0x%08x (wr_dly_taps[22:16] %d, rd_dly_taps[6:0] %d\r\n", KNRM,
                    idx, p_nil_status->cdns_phy_dll_obs_reg[idx], (p_nil_status->cdns_phy_dll_obs_reg[idx]>>16)&0x7F, (p_nil_status->cdns_phy_dll_obs_reg[idx]>>0)&0x7F );
    }

    /*** Cell Tile PC ***/
    LOG("%s\r\n", KNRM);
    for (idx = 0; idx < p_nil_status->num_channel; idx++) {
        LOG("%sCH(%d): ", KNRM, idx);
        print_cell_status(&p_nil_status->cell_status[idx], NULL, 0, 0);
    }
    LOG("%s\r\n", KNRM);
}

void main(int argc, char *argv[])
{
    char cmd_str[500];
    char *fqa_device;
    char *log_file_name;
    u32 *p_vu_data_buf;
    time_t timer;
    struct tm *p_tm;

    timer = time(NULL);
    p_tm = localtime(&timer);

    p_vu_data_buf = malloc(VU_DATA_BUF_SIZE);

    if (argc < 2) {
        printf("argc:%d\r\n", argc);
        return;
    }

    fqa_device = argv[1];
    log_file_name = argv[2];
    fp_log = fopen(log_file_name, "a");

    if (fp_log == NULL) {
        printf("log file open failure: %s\r\n", log_file_name);
        return;
    }

    LOG("%sTime: %s\r\n", KNRM, asctime(p_tm));

    // Get nvme status
    vu_data_to_host(fqa_device, VU_SUB_OPCODE_GET_NVME_STATUS, VU_DATA_BUF_SIZE,
                    0, 0, 0, 0, p_vu_data_buf);

    print_nvme_status((nvme_status_t *)p_vu_data_buf);

    // Get hxl status
    vu_data_to_host(fqa_device, VU_SUB_OPCODE_GET_HXL_STATUS, VU_DATA_BUF_SIZE,
                    0, 0, 0, 0, p_vu_data_buf);

    print_hxl_status((hxl_status_t *)p_vu_data_buf);

    // Get fal status
    vu_data_to_host(fqa_device, VU_SUB_OPCODE_GET_FAL_STATUS, VU_DATA_BUF_SIZE,
                    0, 0, 0, 0, p_vu_data_buf);

    print_fal_status((fal_status_t *)p_vu_data_buf);

    // Get nil status
    vu_data_to_host(fqa_device, VU_SUB_OPCODE_GET_NIL_STATUS, VU_DATA_BUF_SIZE,
                    0, 0, 0, 0, p_vu_data_buf);

    print_nil_status((nil_status_t *)p_vu_data_buf);

    free(p_vu_data_buf);
    fclose(fp_log);
}
