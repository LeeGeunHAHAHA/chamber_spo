#!/bin/bash

#### Color code
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[1;31m'
NC='\033[0m'

FQA_DEVICE=/dev/nvme0
if [ "$1" != "$NULL" ]; then
	FQA_DEVICE=$1
fi

OUTPUT_FILE=vu_fw_info.txt

function send_vu_get_fw_info()
{
	VU_COMMON_OPCODE=0xC4
	VU_SUB_OPCODE=0x401
	DATA_LEN_BYTE=70
	NDT=$[$DATA_LEN_BYTE/4]

	/usr/sbin/nvme admin-passthru $FQA_DEVICE --opcode $VU_COMMON_OPCODE --cdw2=$VU_SUB_OPCODE --read --raw-binary --data-len=$DATA_LEN_BYTE --cdw10=$NDT > output.temp

	tr -cd '\11\12\15\40-\176' < output.temp > $OUTPUT_FILE
	rm -rf output.temp
}

send_vu_get_fw_info


if [[ "$1" != "--mute" ]]; then
	echo ""
        echo -e "${GREEN}Date     Time   FW Info"
        echo "-------- ------ --------------------------------------------------"
        cat $OUTPUT_FILE
        echo -e "${NC}"
	echo ""
fi
