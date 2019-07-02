#!/usr/bin/env bash

time=0
if [ "$#" -eq 1 ]; then
    time=$1
fi

rm print_status -f
gcc print_status.c -o print_status

LOG_DIR=STAT_LOG

if [[ ! -e $LOG_DIR ]]; then
    mkdir $LOG_DIR
fi
DATE=$(date +"%Y%m%d%H%M%S")
LOG_FILE=./STAT_LOG/status_$DATE.log

if [ $time == 0 ]; then
    ./print_status /dev/nvme0n1 $LOG_FILE
else
    echo "Press [CTRL+C] to stop"
    while :
    do
        ./print_status /dev/nvme0n1 $LOG_FILE
        sleep $time
    done
fi

rm print_status -f
