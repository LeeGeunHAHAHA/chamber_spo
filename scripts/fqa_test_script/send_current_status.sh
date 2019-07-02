#!/bin/bash

SERVER_IP=10.10.19.101
SERVER_ID=root
SERVER_PW=fadu
#LOG_PATH=~/monitor
MY_IP=$(hostname -I | awk '{print $1}')
TIME_OUT=1
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script

FW_INFO=$(tr -d '\0' < ${PATH_FQA_TEST_SCRIPT}/vu_fw_info.txt)
sshpass -p $SERVER_PW ssh -o StrictHostKeyChecking=no -o ConnectTimeout=$TIME_OUT $SERVER_ID@$SERVER_IP ./save_log.sh $MY_IP ${FW_INFO}/////$@
