#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script
PATH_GET_LOG=~/fqa/trunk/io/get_log

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'


# $PATH_FQA_TEST_SCRIPT/factory_reset.sh

INPUT=$1

$PATH_FQA_TEST_SCRIPT/get_fw_info.sh

if [ $INPUT == $NULL ]; then
	echo -e "${GREEN}[ Input Test Script No. ]${NC}"
	echo "      1. : 11.jesd219"
	echo "      2. : 12.jesd219_4k"
	echo -e "${GREEN}Input> ${NC}"
	read INPUT
fi

cd $FQA_TRUNK_DIR
source source.bash
./setup.sh

rm -rf $PATH_GET_LOG
mkdir $PATH_GET_LOG

cd ./io

case $INPUT in
	1)
		echo -e "${RED}[11.jesd219 Run!]${NC}"
		cd ./11.jesd219
		;;
	2)
		echo -e "${RED}[12.jesd219_4k Run!]${NC}"
		cd ./12.jesd219_4k
		;;
esac

./run

$PATH_FQA_TEST_SCRIPT/send_current_status.sh "Endurance Test Pass"