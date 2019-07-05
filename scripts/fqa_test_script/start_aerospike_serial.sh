#!/bin/bash
# e.g. start_aerospike_serial.sh VENDER_NAME FW_NAME DRIVE_NAME 

AEROSPIKE_DIR=~/act
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script

GREEN='\033[1;32m'
RED='\033[1;31m'
COLOR=$GREEN
NC='\033[0m'

TOTAL_TEST_TIME=24  # hour

SN_INFO=$(nvme list | head -n 3 | tail -n 1 | awk '{print $2}')
MODEL_INFO=$(nvme list | head -n 3 | tail -n 1 | awk '{print $3}')
CAPA_INFO=$(nvme list | head -n 3 | tail -n 1 | awk '{print $8}')
FW_INFO=$(nvme list | head -n 3 | tail -n 1 | awk '{print $15}')

# $PATH_FQA_TEST_SCRIPT/factory_reset.sh

$PATH_FQA_TEST_SCRIPT/get_fw_info.sh

echo "Aerospike serial Test"

cd $AEROSPIKE_DIR
for times in 3 6 12 24 48
do
	echo -e "${COLOR}[Aerospike ${times}x Test]${NC}"
	$PATH_FQA_TEST_SCRIPT/send_current_status.sh "Aerospike ${times}x Test Start"

	DATE=$(date "+%y%m%d_%H%M%S")
	./runact /dev/nvme0n1 actconfig_${times}x_1d.txt ${DATE}_${times}x_${MODEL_INFO}_${FW_INFO}_${CAPA_INFO}.txt
	python2.7 ./latency_calc/act_latency.py -l ${DATE}_${times}x_${MODEL_INFO}_${FW_INFO}_${CAPA_INFO}.txt > ${DATE}_${times}x_${MODEL_INFO}_${FW_INFO}_${CAPA_INFO}_log_convert.txt

	test_time=$(tail -n 4 ./${DATE}_${times}x_${MODEL_INFO}_${FW_INFO}_${CAPA_INFO}_log_convert.txt | head -n 1  | awk -F' ' '{print $(1)}' | sed 's/ //g')
	test_time=&((test_time))
	if [ $test_time -lt $TOTAL_TEST_TIME ];then
		echo -e "${RED}~Test Failed : ${times}x ${test_time}hr~${NC}"
		$PATH_FQA_TEST_SCRIPT/send_current_status.sh "Aerospike $times Test Fail [~$test_timehr]"
		break
	else
		echo -e "${COLOR}[All done.]${NC}"
		$PATH_FQA_TEST_SCRIPT/send_current_status.sh "Aerospike All Test Pass"
	fi
	
	echo "sleep 60"
	sleep 60
done
