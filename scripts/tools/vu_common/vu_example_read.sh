#!/bin/bash

source ./vu_tools.sh

DEV_NAME=/dev/nvme0n1
NSID=1
VU_SUB_OPCODE=0x00000001 #BLOCK_READ_COUNT
DATA_LEN_BYTE=12
PARAM0=0xAA
PARAM1=0X00
PARAM2=0X00
PARAM3=0X00

vu_read_common $DEV_NAME $NSID $VU_SUB_OPCODE $DATA_LEN_BYTE $PARAM0 $PARAM1 $PARAM2 $PARAM3

for i in "${vu_read_buf[@]}"
do
	echo ${vu_read_buf[$i]}
done
