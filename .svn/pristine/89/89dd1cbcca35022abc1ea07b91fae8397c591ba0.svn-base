#!/bin/sh
#IMAGE_NAME=hxl_fal_test.bin
DEVICE=/dev/nvme0

nvme fw-download $DEVICE -f $1
#nvme fw-activate $DEVICE -s 0 -a 1
nvme fw-commit $DEVICE -s 0 -a 1

#NVMe reset
nvme reset $DEVICE

#nvme list
nvme list


