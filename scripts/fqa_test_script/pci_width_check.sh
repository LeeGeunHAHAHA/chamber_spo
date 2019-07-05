#!/bin/bash

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

pci_width=4
lspci_width=$(lspci -vv | grep -A 25 Non-Volatile | tail -n1 | awk '{print $5}' | sed 's/x//g' | sed 's/,//g')

while((1)) 
do
if [[ "${lspci_width}" -eq "${pci_width}" ]]; then
	echo -e "${GREEN}Width OK${NC}"
	break;
elif [[ "${lspci_width}" -ne "${pci_width}" ]]; then 
	echo -e "${RED}Width Check again${NC}"
	sleep 2
	~/fqa/trunk/fqa_test_script/pmu_control.sh 0
	~/fqa/trunk/fqa_test_script/pmu_control.sh 1
	~/fqa/trunk/fqa_test_script/pmu_control.sh 2
	~/fqa/trunk/fqa_test_script/pmu_control.sh 3
fi
done
