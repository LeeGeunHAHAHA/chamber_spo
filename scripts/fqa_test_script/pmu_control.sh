#!/bin/bash

INPUT=$1

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

PCI_ADDRESS=$(echo 0000:$(lspci | grep Non-Volatile | awk '{print $1'}))
echo $PCI_ADDRESS


if [ $INPUT == $NULL ]; then
	echo -e "${GREEN}[ Input Operation No. ]${NC}"
	echo "      0. : 0. PCI_REMOVE"
	echo "      1. : 1. PMU_POWER_OFF"
	echo "      2. : 2. PMU_POWER_ON"
	echo "      3. : 3. PCI_RESCAN"
	echo -e "${GREEN}Input> ${NC}"
	read INPUT
fi

case $INPUT in
	
	0)
		echo -e "${GREEN}[PCI Remove]${NC}"
		echo "1" > /sys/bus/pci/devices/${PCI_ADDRESS}/remove # pci remove
		;;

	1)
		echo -e "${GREEN}[PMU Power off]${NC}"
		 ~/fqa/trunk/tools/powerctl/powerctl /dev/ttyUSB0 0 # 0 : pmu power off
		;;
	2)
		echo -e "${GREEN}[PMU Power on]${NC}"
		 ~/fqa/trunk/tools/powerctl/powerctl /dev/ttyUSB0 1 # 1 : pmu power on
		;;
	3)
		echo -e "${GREEN}[PCI Rescan]${NC}"
		for (( k=1; k <= 30; k++))
		do
			echo "1" > /sys/bus/pci/rescan
			sleep 1
			if [ -b /dev/nvme0n1 ]; then
				echo -ne "${GREEN}[${k}/30]Device found${NC}\n"
				break
			else
				echo -ne "${RED}[${k}/30]Device not found${NC}\r"
			fi
		done
	     	;;

esac
