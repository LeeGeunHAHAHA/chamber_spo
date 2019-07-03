#!/bin/bash
QUAL_TRUNK_DIR=~/qual/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script
PATH_GET_LOG=~/qual/trunk/simple/smart_log

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'


INPUT=$1


$PATH_FQA_TEST_SCRIPT/get_fw_info.sh


if [ $INPUT == $NULL ]; then
	echo -e "${GREEN}[ Input Test Script No. ]${NC}"
	echo "      1. : sprint_pfqa.sh"
	echo "      2. : sprint_pfqa_trim.sh"
	echo "      100. : do_test_standard.sh"
	echo "      101. : do_test_standard_trim_4k_lba_format.sh"
	echo -e "${GREEN}Input> ${NC}"
	read INPUT
fi


# $PATH_FQA_TEST_SCRIPT/factory_reset.sh

rm -rf $PATH_GET_LOG
mkdir $PATH_GET_LOG

cd $QUAL_TRUNK_DIR
case $INPUT in
	1)
		echo -e "${RED}[sprint_pfqa.sh Run!]${NC}"
		./sprint_pfqa.sh
		;;
	2) 
		echo -e "${RED}[sprint_pfqa_trim.sh Run!]${NC}"
		./sprint_pfqa_trim.sh
		;;
	100)
		echo -e "${RED}[do_test_standard.sh Run!]${NC}"
		./do_test_standard.sh
		;;
	101)
		echo -e "${RED}[do_test_standard_trim_4k_lba_format.sh Run!]${NC}"
		./do_test_standard_trim_4K_lba_format.sh
		;;
esac

