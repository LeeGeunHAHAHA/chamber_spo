#!/bin/bash
USER_ID=jspark
SERVER_IP=192.168.110.2
IMAGE_DIR=/user/share/fw_release/daily_fqa
IMAGE_NAME=pcie_fw.bin

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'


FQA_TRUNK_DIR=~/fqa/trunk

#NVME list
#nvme list
#./get_fw_info.sh

# FW download
rm -f $FQA_TRUNK_DIR/$IMAGE_NAME
scp $USER_ID@$SERVER_IP:$IMAGE_DIR/$1.bin $FQA_TRUNK_DIR/$IMAGE_NAME


if [ -e ~/fqa/trunk/$IMAGE_NAME ]; then
	echo -ne "${GREEN}PCIE FW Binary Found${NC}\n"
else
	echo -ne "${RED}PCIE FW Binary Not Found${NC}\n"
fi

# PCIE FW Copy
cp ~/fqa/trunk/$IMAGE_NAME ~/qual/trunk/tools/pcie_boot/fw.bin

# PCIE_DEVICE SET



#FW update
cd ~/qual/trunk/tools/pcie_boot/
make setup
sleep 2
make test
