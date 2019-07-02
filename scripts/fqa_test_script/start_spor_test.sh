#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

INPUT=$1
OPTION=$2

ip=$(echo "`hostname -I | awk -F'.' '{printf $4}'` - 101" | bc)

mkfile_time=$(date "+%Y%m%d_%H%M%S")

array_lack=("A-1-1" "A-1-2" "A-1-3" "A-1-4"
	    "A-2-1" "A-2-2" "A-2-3" "A-2-4"
	    "A-3-1" "A-3-2" "A-3-3" "A-3-4"
	    "A-4-1" "A-4-2" "A-4-3" "A-4-4"
			
	    "B-1-1" "B-1-2" "B-1-3" "B-1-4"
	    "B-2-1" "B-2-2" "B-2-3" "B-2-4"
	    "B-3-1" "B-3-2" "B-3-3" "B-3-4" 
	    "B-4-1" "B-4-2" "B-4-3" "B-4-4"

            "C-1-1" "C-1-2" "C-1-3" "C-1-4"
            "C-2-1" "C-2-2" "C-2-3" "C-2-4"
            "C-3-1" "C-3-2" "C-3-3" "C-3-4"
            "C-4-1" "C-4-2" "C-4-3" "C-4-4")

$PATH_FQA_TEST_SCRIPT/get_fw_info.sh

if [ $INPUT == $NULL ]; then
	echo -e "${GREEN}[ Input Test Script No. ]${NC}"
	echo "      1. : 1.spor_wo_write"
	echo "      2. : 2.spor_w_write"
	echo "      3. : 3.spor_w_write_512b"
	echo "      4. : 4.spo_at_npo"
	echo -e "${GREEN}Input> ${NC}"
	read INPUT
fi



# $PATH_FQA_TEST_SCRIPT/factory_reset.sh

cd $FQA_TRUNK_DIR
source source.bash
./setup.sh

cd ./spor


if [ "$OPTION" != "$NULL" ]; then
	export FQA_DEVICE=$OPTION
	echo -e "${GREEN}change device addr -> ${FQA_DEVICE}${NC}"
fi


case $INPUT in
	1)
		echo -e "${RED}[1.spor_wo_write Run!]${NC}"
		cd ./1.spor_wo_write
		tar cvf ${array_lack[$ip]}_${mkfile_time}_log.tar log > /dev/null 
		rm -rf log
		./run
		;;
	2)
		echo -e "${RED}[2.spor_w_write Run!]${NC}"
		cd ./2.spor_w_write
		tar cvf ${array_lack[$ip]}_${mkfile_time}_log.tar log  > /dev/null
		rm -rf log
		./run
		;;
	3)
		echo -e "${RED}[3.spor_w_write_512b Run!]${NC}"
		cd ./3.spor_w_write_512b
		tar cvf ${array_lack[$ip]}_${mkfile_time}_log.tar log > /dev/null
		rm -rf log
		./run
		;;
	4)
		echo -e "${RED}[4.spo_at_npo Run!]${NC}"
		cd ./4.spo_at_npo
		tar cvf ${array_lack[$ip]}_${mkfile_time}_log.tar log > /dev/null
		rm -rf log
		./run
		;;
esac
