#!/usr/bin/env bash

rm print_stat_defense_code -f
rm reset_stat_defense_code -f

gcc print_stat_defense_code.c -o print_stat_defense_code
gcc reset_stat_defense_code.c -o reset_stat_defense_code

LOG_DIR=STAT_LOG

if [[ ! -e $LOG_DIR ]]; then
    mkdir $LOG_DIR
fi
DATE=$(date +"%Y%m%d%H%M%S")
LOG_FILE=./STAT_LOG/stat_defense_code_$DATE.log

./print_stat_defense_code /dev/nvme0n1 $LOG_FILE
./reset_stat_defense_code /dev/nvme0n1

rm print_stat_defense_code -f
rm reset_stat_defense_code -f