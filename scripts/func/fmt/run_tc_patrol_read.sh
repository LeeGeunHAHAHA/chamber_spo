#!/usr/bin/env bash

rm patrol_read -f
gcc patrol_read.c -o patrol_read

TEST_LOOPS=100

LOG_FILE=log_patrol_read.log

PATROL_READ_ROUND_TIME=300
./patrol_read /dev/nvme0n1 $PATROL_READ_ROUND_TIME $LOG_FILE

for ((loop=1; loop<=$TEST_LOOPS; loop++))
do  
	PATROL_READ_ROUND_TIME=150
	./patrol_read /dev/nvme0n1 $PATROL_READ_ROUND_TIME $LOG_FILE

	PATROL_READ_ROUND_TIME=200
	./patrol_read /dev/nvme0n1 $PATROL_READ_ROUND_TIME $LOG_FILE

	PATROL_READ_ROUND_TIME=250
	./patrol_read /dev/nvme0n1 $PATROL_READ_ROUND_TIME $LOG_FILE
done