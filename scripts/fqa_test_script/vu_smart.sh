#!/bin/bash

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'
start_time=$(date "+%Y%m%d_%H%M%S")
VU_SMART_LOG_DIR=vu_smart_log


ip=$(echo "`hostname -I | awk -F'.' '{printf $4}'` - 101" | bc)

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
			
			
nvme fadu vu_smart /dev/nvme0
cd ..
mkdir -p $VU_SMART_LOG_DIR
nvme fadu vu_smart /dev/nvme0 > $VU_SMART_LOG_DIR/VU_SMART_${array_lack[$ip]}_$start_time.txt