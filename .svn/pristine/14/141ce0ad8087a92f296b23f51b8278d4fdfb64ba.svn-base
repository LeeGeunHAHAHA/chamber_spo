import argparse
import sys
import os
import mmap
import math
import resource
import subprocess
import time
from wrapper_mmap import wrapper_mmap

class pcie_proc() :
    wm:wrapper_mmap

    dev_bdf:str
    pci_resource:str
    ddr:str
    fuser:str
    fw:str

    ddr_raw_buf_addr:hex   = 0x8070008000 #DDR_INIT_UCODE_BASE
    fuser_raw_buf_addr:hex = 0x0070000000 #PMU_OCM_BASE;
    fw_raw_buf_addr:hex    = 0x8400000000 #FW_BASE;
    jump_addr:hex          = 0x0070000000

    def __init__(self, wm:wrapper_mmap, dev_bdf:str, ddr:str, fuser:str, fw:str):
        self.wm = wm
        self.dev_bdf = dev_bdf
        self.ddr = self.read_bin_file(ddr)
        self.fuser = self.read_bin_file(fuser)
        self.fw = self.read_bin_file(fw)

    def __enter__(self):
        print('setpci -s {} COMMAND=0x02'.format(self.dev_bdf))
        result = subprocess.check_output('setpci -s {} COMMAND=0x02'.format(self.dev_bdf),shell=True, stderr=subprocess.STDOUT)
        if result.decode('utf-8') != '':
            print(result.decode('utf-8'))
            exit(-1)
        
        self.pci_resource = self.find_pci_resource(self.dev_bdf)
        if self.pci_resource is None :
            print("Not find {} pci resource!".format(self.dev_bdf))
            exit(-1)

        if self.wm.attach_pci(self.pci_resource) is False : 
            print("Device {} Attach pci error!".format(self.dev_bdf))
            exit(-1)
    
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        if self.wm.detach_pci() is False:
            print("Device {} Detach pci error!".format(self.pci_resource))
            exit(-1)

    def run(self):
        from datetime import datetime
        print("{} PCIe_boot fusing start!".format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))
        
        ''' ddr image loading '''
        print("ddr image loading")
        self.set_buf_addr('LOADING', self.ddr_raw_buf_addr >> 32 , self.ddr_raw_buf_addr & 0xFFFFFFFF)
        self.set_buf_len('LOADING', len(self.ddr))
        self.send_data('LOADING', self.ddr, len(self.ddr))

        ''' fuser image loading '''
        print("fuser image loading")
        self.set_buf_addr('LOADING', self.fuser_raw_buf_addr >> 32 , self.fuser_raw_buf_addr & 0xFFFFFFFF)
        self.set_buf_len('LOADING', len(self.fuser))
        self.send_data('LOADING', self.fuser, len(self.fuser))

        ''' jump pmu pc register to fuser image '''
        print("jump image loading")
        self.set_buf_addr('JUMPING', self.fuser_raw_buf_addr >> 32 , self.fuser_raw_buf_addr & 0xFFFFFFFF)
        self.wait_to_ack('JUMPING')

        ''' fw image loading '''
        print("fw image loading")
        self.set_buf_addr('LOADING', self.fw_raw_buf_addr >> 32, self.fw_raw_buf_addr & 0xFFFFFFFF)
        self.set_buf_len('LOADING', len(self.fw))
        self.send_data('LOADING', self.fw, len(self.fw))

        ''' wait fw image fusing '''
        self.wait_to_ack('COMPLETE')
        print("{} PCIe_boot fusing end!".format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))

    def wait_to_jumping(self, cond_type:str):
        PCIE_BOOT_STATUS_OK:int = 0
        DEFAULT_CPL_VAL:int = 1

        self.set_cpl_reg(DEFAULT_CPL_VAL)
        self.write_cmd_reg_addr(self.switch('{}_ACK'.format(cond_type)))
        self.wait_cpl(PCIE_BOOT_STATUS_OK)
        print("Sent jump to fuser succesfully");	

    def wait_to_complete(self, cond_type:str):
        COMPLETE_ACK_MESSAGE:int = 0xdeadbeef
        DEFAULT_CPL_VAL:int = 1

        self.set_cpl_reg(DEFAULT_CPL_VAL)
        self.write_cmd_reg_addr(self.switch('{}_ACK'.format(cond_type)))
        self.wait_cpl(COMPLETE_ACK_MESSAGE)
        print("Complete firmware download")

    def wait_to_ack(self, cond_type:str):    
        return {
            'JUMPING'  : self.wait_to_jumping,
            'COMPLETE' : self.wait_to_complete,
        }.get(cond_type)(cond_type)

    def read_bin_file(self, bin_file:str):
        try:
            with open(bin_file, 'rb') as file:
                contents = file.read(os.stat(bin_file).st_size)
        except FileNotFoundError as err:
            print(err)
            exit(-1)

        return contents

    def find_pci_resource(self, dev_bdf:str):
        pci_resource_list = [
            # '/sys/class/nvme/{}/device/resource0'.format(dev_bdf),
            # '/sys/class/misc/{}/device/resource0'.format(dev_bdf),
            '/sys/bus/pci/devices/0000:{}/resource0'.format(dev_bdf),
            # '/sys/bus/pci/devices/{}/resource0'.format(dev_bdf)
        ]

        for pci_resource in pci_resource_list:
            if os.path.exists(pci_resource) is True:
                return pci_resource
        
        return None

    def write_cmd_reg_addr(self, msg:int):
        FW_DOWNLOAD_REG_REQ = 0x50
        return self.wm.write_register(FW_DOWNLOAD_REG_REQ, msg)

    def write_data_reg_addr(self, msg:int):
        FW_DOWNLOAD_REG_DATA = 0x54
        return self.wm.write_register(FW_DOWNLOAD_REG_DATA, msg)

    def write_cpl_reg_addr(self, msg:int):
        FW_DOWNLOAD_REG_CPL = 0x58
        return self.wm.write_register(FW_DOWNLOAD_REG_CPL, msg)

    def wait_cpl(self, msg:int):
        FW_DOWNLOAD_REG_CPL = 0x58
        while True:
            recv_msg = self.wm.read_register(FW_DOWNLOAD_REG_CPL)
            if recv_msg == msg :
                return
            

    def set_cpl_reg(self, cpl_val:int):
        self.write_cpl_reg_addr(cpl_val)
        self.wait_cpl(cpl_val)

    def switch(self, cond_type:str):
        PCIE_FW_DOWNLOAD_SET_FW_BUF_ADDR:hex = 0x1
        PCIE_FW_DOWNLOAD_SET_FW_LEN:hex = 0x2
        PCIE_FW_DOWNLOAD_SEND_FW:hex = 0x3
        PCIE_FW_DOWNLOAD_SET_JUMP_ADDR:hex = 0xc
        PCIE_FW_DOWNLOAD_JUMP_TO_FW:hex = 0xd
        ADDR_HIGH:hex = 0x1
        FW_DOWNLOAD_REQ_SHIFT_CMD:int = 28

        return {
            'LOADING_LOW'       : PCIE_FW_DOWNLOAD_SET_FW_BUF_ADDR << FW_DOWNLOAD_REQ_SHIFT_CMD,
            'LOADING_HIGH'      : (PCIE_FW_DOWNLOAD_SET_FW_BUF_ADDR << FW_DOWNLOAD_REQ_SHIFT_CMD) | ADDR_HIGH,
            'LOADING_LEN'       : PCIE_FW_DOWNLOAD_SET_FW_LEN << FW_DOWNLOAD_REQ_SHIFT_CMD,
            'LOADING_SEND'      : PCIE_FW_DOWNLOAD_SEND_FW << FW_DOWNLOAD_REQ_SHIFT_CMD,
            'JUMPING_LOW'      : PCIE_FW_DOWNLOAD_SET_JUMP_ADDR << FW_DOWNLOAD_REQ_SHIFT_CMD,
            'JUMPING_HIGH'     : (PCIE_FW_DOWNLOAD_SET_JUMP_ADDR << FW_DOWNLOAD_REQ_SHIFT_CMD) | ADDR_HIGH,
            'JUMPING_ACK'      : PCIE_FW_DOWNLOAD_JUMP_TO_FW << FW_DOWNLOAD_REQ_SHIFT_CMD,
            'COMPLETE_ACK'     : PCIE_FW_DOWNLOAD_JUMP_TO_FW << FW_DOWNLOAD_REQ_SHIFT_CMD,
        }.get(cond_type)

    def set_register(self, data:int, cmd:str):
        PCIE_BOOT_STATUS_OK:int = 0
        DEFAULT_CPL_VAL:int = 1

        self.set_cpl_reg(DEFAULT_CPL_VAL)
        self.write_data_reg_addr(data)
        self.write_cmd_reg_addr(self.switch(cmd))
        self.wait_cpl(PCIE_BOOT_STATUS_OK)

    def set_buf_addr(self, cond_type:str, addr_high:int, addr_low:int):
        self.set_register(addr_low, '{}_LOW'.format(cond_type))
        self.set_register(addr_high, '{}_HIGH'.format(cond_type))
        print("Set buffer addr for type {} succesfully, {}_{}\n".format(cond_type, str(hex(addr_high)).zfill(8), str(hex(addr_low)).zfill(8)))
    
    def set_buf_len(self, cond_type:str, len:int):
        self.set_register(len, '{}_LEN'.format(cond_type))
        print("Set length for type {} succesfully, length: {}\n".format(cond_type, len))

    def send_data(self, cond_type:str, raw_buf:str, raw_buf_len:int):
        BYTES_PER_DW = 4
        FW_DOWNLOAD_REQ_SHIFT_OFFSET = 0
        DEFAULT_CPL_VAL:int = 1
        PCIE_BOOT_STATUS_OK:int = 0

        raw_buf_len_dw = math.ceil(raw_buf_len / BYTES_PER_DW)
        for dw_cnt in range(0, raw_buf_len_dw):
            self.set_cpl_reg(DEFAULT_CPL_VAL)
            self.write_data_reg_addr(int.from_bytes(raw_buf[dw_cnt * BYTES_PER_DW : dw_cnt * BYTES_PER_DW + BYTES_PER_DW], byteorder='little', signed=False))
            self.write_cmd_reg_addr(self.switch('{}_SEND'.format(cond_type)) | ((dw_cnt * BYTES_PER_DW) << FW_DOWNLOAD_REQ_SHIFT_OFFSET  ))
            self.wait_cpl(PCIE_BOOT_STATUS_OK)
            print("Sent type {} image {} of {} successfully".format(cond_type, dw_cnt,raw_buf_len_dw-1))
        
        self.set_cpl_reg(DEFAULT_CPL_VAL)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--dev_bdf', default='01:00.0', help="[pci bus number], default:01:00.0")
    parser.add_argument('--ddr', default='nvme_ddr.bin', help="ddr.bin image filepath, default:nvme_ddr.bin")
    parser.add_argument('--fuser',default='fuser.bin', help="fuser.bin image filepath, default:fuser.bin")
    parser.add_argument('--fw', default='fw.bin', help="fw.bin image filepath, default:fw.bin")
    args = parser.parse_args()

    with wrapper_mmap() as wm :    
        with pcie_proc(wm, args.dev_bdf, args.ddr, args.fuser, args.fw) as pproc:
            pproc.run()

