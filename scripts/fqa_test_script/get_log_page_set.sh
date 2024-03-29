#!/bin/bash

#FQA_TRUNK_DIR=~/chamber_spo/scripts
#FQA_TEST_SCRIPT_DIR=${FQA_TRUNK_DIR}/fqa_test_script

DEVICE=$1

LOG_PATH=$2

LOG_FILE_SUFFIX=$3

DATE=$(date "+%y%m%d_%H%M%S")

echo "Get-log-Page logging (0x02, 0x07, 0xCA)"

# get-log 0x02
nvme smart-log $DEVICE > ${LOG_PATH}/get_log_0x02_${DATE}_${LOG_FILE_SUFFIX}.log

# get-log 0x07
nvme get-log $DEVICE --log-id=0x07 --log-len=2048 --raw-binary > ${LOG_PATH}/get_log_0x07_${DATE}_${LOG_FILE_SUFFIX}.log

# get-log 0xCA
nvme get-log $DEVICE --log-id=0xCA --log-len=712 --raw-binary > ${LOG_PATH}/get_log_0xCA_${DATE}_${LOG_FILE_SUFFIX}.log