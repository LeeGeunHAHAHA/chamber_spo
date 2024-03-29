#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script
PATH_GET_LOG=~/fqa/trunk/io/get_log

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

INPUT=$1
OPTION=$2

$PATH_FQA_TEST_SCRIPT/get_fw_info.sh

ulimit -c unlimited

if [ $INPUT == $NULL ]; then
	echo -e "${GREEN}[ Input Test Script No. ]${NC}"i
	echo "      0. : ns8_18hr.sh - tmp"
	echo "      1. : ns16_30m_4kb.sh"
	echo "      2. : ns16_1hr_4kb.sh"
	echo "      3. : ns16_3hr_4kb.sh"
	echo "      4. : ns16_6hr_4kb.sh"
	echo "      5. : ns16_12hr_4kb.sh"
	echo "      6. : ns16_18hr_4kb.sh"
	echo "      7. : ns16_24hr_4kb.sh"
	echo "      8. : ns16_36hr_4kb.sh"
	echo "      9. : ns16_48hr_4kb.sh"
	echo "     10. : ns16_72hr_4kb.sh"
	echo ""
	echo "     101. : ns16_30m"
	echo "     102. : ns16_1hr"
	echo "     103. : ns16_3hr"
	echo "     104. : ns16_6hr"
	echo "     105. : ns16_12hr"
	echo "     106. : ns16_18hr"
	echo "     107. : ns16_24hr"
	echo "     108. : ns16_36hr"
	echo "     109. : ns16_48hr"
	echo "     110. : ns16_72hr"
	echo ""
	echo "     201. : longrun_45m"
	echo "     202. : longrun_1hr"
	echo "     203. : longrun_3hr"
	echo "     204. : longrun_6hr"
	echo "     205. : longrun_12hr"
	echo "     206. : longrun_18hr"
	echo "     207. : longrun_24hr"
	echo "     208. : longrun_36hr"
	echo "     209. : longrun_48hr"
	echo "     210. : longrun_72hr"
	echo ""
	echo "     301. : single_72hr+multi_72hr(ns1+ns16)"
	echo "     302. : single_144hr+multi_144hr(ns1+ns16)"
	echo "     303. : single_72hr+multi_72hr(ns1+ns16 4k only)"
	echo -e "${GREEN}Input> ${NC}"
	read INPUT
fi

if [ $OPTION == $NULL ]; then
	if [ $INPUT -ge 201 ] && [ $INPUT -lt 299 ]; then
		echo -e "${GREEN}[ Single Namespace Option ]${NC}"
		echo "     1. : 4KB"
		echo "     2. : 512B"
		echo "     3. : 4KB	+ 8B separted buffer"
		echo "     4. : 512KB	+ 8B separted buffer"
		echo "     5. : 4KB	+ 8B separted buffer, data protection type 1"
		echo "     6. : 512KB	+ 8B separted buffer, data protection type 1"
		echo "     7. : 4KB	+ 8B separted buffer, data protection type 2"
		echo "     8. : 512KB	+ 8B separted buffer, data protection type 2"
		echo -e "${GREEN}Option> ${NC}"
		read OPTION
	fi
fi



# $PATH_FQA_TEST_SCRIPT/factory_reset.sh

start_time=$(date +%s)

cd $FQA_TRUNK_DIR
source source.bash
./setup.sh


cd ./io

rm -rf $PATH_GET_LOG
mkdir $PATH_GET_LOG

case $INPUT in
	0)
		echo -e "${RED}[ns8_18hr.sh Run!]${NC}"
		./setup_ns8
		cd ns8
		ulimit -c unlimited
		sudo sysctl -w kernel.core_pattern=/tmp/core-%e.%p.%h.%t
		./ns8_18hr.sh
		;;
		 
	1)
		echo -e "${RED}[ns16_30m_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_30m_4kb.sh
		;;
	2)
		echo -e "${RED}[ns16_1hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_1hr_4kb.sh
		;;
	3)
		echo -e "${RED}[ns16_3hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_3hr_4kb.sh
		;;
	4)
		echo -e "${RED}[ns16_6hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_6hr_4kb.sh
		;;
	5)
		echo -e "${RED}[ns16_12hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16
		./ns16_12hr_4kb.sh
		;;
	6)
		echo -e "${RED}[ns16_18hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_18hr_4kb.sh
		;;
	7)
		echo -e "${RED}[ns16_24hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_24hr_4kb.sh
		;;
	8)
		echo -e "${RED}[ns16_36hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_36hr_4kb.sh
		;;
	9)
		echo -e "${RED}[ns16_48hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_48hr_4kb.sh
		;;
	10)
		echo -e "${RED}[ns16_72hr_4kb.sh Run!]${NC}"
		./setup_ns16_4kb
		cd ns16_4kb
		./ns16_72hr_4kb.sh
		;;
	101)
		echo -e "${RED}[ns16_30m.sh Run!]${NC}"
		./setup_ns16
		./ns16_30m.sh
		;;
	102)
		echo -e "${RED}[ns16_1hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_1hr.sh
		;;
	103)
		echo -e "${RED}[ns16_3hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_3hr.sh
		;;
	104)
		echo -e "${RED}[ns16_6hr.sh Run!]${NC}"
		./setup_ns16 $OPTION
		./ns16_6hr.sh
		;;
	105)
		echo -e "${RED}[ns16_12hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_12hr.sh
		;;
	106)
		echo -e "${RED}[ns16_18hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_18hr.sh
		;;
	107)
		echo -e "${RED}[ns16_24hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_24hr.sh
		;;
	108)
		echo -e "${RED}[ns16_36hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_36hr.sh
		;;
	109)
		echo -e "${RED}[ns16_48hr+1hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_48hr.sh
		;;
	110)
		echo -e "${RED}[ns16_72hr.sh Run!]${NC}"
		./setup_ns16
		./ns16_72hr.sh
		;;
	201)
		echo -e "${RED}[longrun_45m Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_45m
		;;
	202)
		echo -e "${RED}[longrun_1hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_1hr
		;;
	203)
		echo -e "${RED}[longrun_3hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_3hr
		;;
	204)
		echo -e "${RED}[longrun_6hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_6hr
		;;
	205)
		echo -e "${RED}[longrun_12hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_12hr
		;;
	206)
		echo -e "${RED}[longrun_18hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_18hr
		;;
	207)
		echo -e "${RED}[longrun_24hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_24hr
		;;
	208)
		echo -e "${RED}[longrun_36hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_36hr
		;;
	209)
		echo -e "${RED}[longrun_48hr+1hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_48hr
		;;
	210)
		echo -e "${RED}[longrun_72hr Run!]${NC}"
		./setup_single_ns $OPTION
		./longrun_72hr
		;;
	301)
		echo -e "${RED}[single_72hr+multi_72hr.sh(ns16) Run!]${NC}"
		./single_72hr+multi_72hr.sh
		;;
	302)
		echo -e "${RED}[single_144hr+multi_144hr.sh(ns16) Run!]${NC}"
		./single_144hr+multi_144hr.sh
		;;
	303)
		echo -e "${RED}[single_72hr+multi_72hr.sh(ns8) 4k only Run!]${NC}"
		./single_72hr+multi_72hr_4k.sh
		;;
	304)
		echo -e "${RED}[single_144hr+multi_144hr.sh(ns8) 4k only Run!]${NC}"
		./single_144hr+multi_144hr_ns8.sh
		;;
esac

end_time=$(date +%s)
exe_time=`echo "$end_time - $start_time" | bc`
htime=`echo "$exe_time/3600" | bc`
mtime=`echo "($exe_time/60) - ($htime * 60)" | bc`
stime=`echo "$exe_time - (($exe_time/60)*60)" | bc`
echo -e "${COLOR}All workloads have done! [${htime}H ${mtime}M ${stime}S] ${NC}"

$PATH_FQA_TEST_SCRIPT/send_current_status.sh "Function Test Pass"
