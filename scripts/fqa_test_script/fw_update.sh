#!/bin/bash
USER_ID=jspark
SERVER_IP=192.168.110.2
IMAGE_DIR=/user/share/fw_release/daily_fqa
IMAGE_NAME=bravo_fw.bin

FQA_TRUNK_DIR=~/fqa/trunk

DEVICE=/dev/nvme0
if [ "$2" != "$NULL" ]; then
	DEVICE=$2
fi

#NVME list
nvme list
./get_fw_info.sh

# FW download
rm -f $FQA_TRUNK_DIR/$IMAGE_NAME
scp $USER_ID@$SERVER_IP:$IMAGE_DIR/$1.bin $FQA_TRUNK_DIR/$IMAGE_NAME


#FW update
nvme fw-download $DEVICE -f $FQA_TRUNK_DIR/$IMAGE_NAME
#nvme fw-activate $DEVICE -s 0 -a 1
nvme fw-commit $DEVICE -s 0 -a 1

#NVME reset
nvme reset $DEVICE

#NVME list
nvme list
./get_fw_info.sh
