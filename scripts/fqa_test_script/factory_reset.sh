#!/bin/bash

NVME_DEV=/dev/nvme0
FQA_DIR=~/fqa/trunk
FQA_DEVICE=0000:02:00.0
SPDK_ENABLE=0
NVME_CLI=/usr/sbin/nvme

if [ "$1" != "$NULL" ]; then
	NVME_DEV=$1
fi

# if [ ! -e $NVME_DEV ]; then
#     echo "$NVME_DEV is not found. maybe loaded SPDK"

#     echo "revert SPDK to $NVME_DEV"
#     source $FQA_DIR/source.bash
#     $FQA_DIR/setup.sh reset
#     sleep 3

#     SPDK_ENABLE=1
# fi

echo "!!! Factory Reset !!!"
$NVME_CLI admin-passthru $NVME_DEV -o 0xc4 --cdw2=0x0
# sleep 3

# echo 1 > /sys/bus/pci/devices/$FQA_DEVICE/remove
# sleep 1
# while [ -e $NVME_DEV ]; do
#     echo "wait for 1 sec to remove $NVME_DEV"
#     sleep 1
# done

# echo 1 > /sys/bus/pci/rescan
# sleep 1
# while [ ! -e $NVME_DEV ]; do
#     echo "wait for 1 sec to attach $NVME_DEV"
#     sleep 1
# done

# sleep 1
# $NVME_CLI list

# if [ $SPDK_ENABLE -eq 1 ]; then
#     echo "Revert $NVME_DEV to SPDK"
#     sleep 3

#     source $FQA_DIR/source.bash
#     $FQA_DIR/setup.sh
# fi
