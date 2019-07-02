#!/usr/bin/env bash

rm retention_reclaim -f
gcc retention_reclaim.c -o retention_reclaim

LOG_FILE=log_retention.log

TEST_LOOPS=100

for ((loop=1; loop<=$TEST_LOOPS; loop++))
do
	TEST_STEP_NO=1 #1~5
	./retention_reclaim /dev/nvme0n1 $TEST_STEP_NO $LOG_FILE
	TEST_STEP_NO=2 #1~5
	./retention_reclaim /dev/nvme0n1 $TEST_STEP_NO $LOG_FILE
	TEST_STEP_NO=3 #1~5
	./retention_reclaim /dev/nvme0n1 $TEST_STEP_NO $LOG_FILE
	TEST_STEP_NO=4 #1~5
	./retention_reclaim /dev/nvme0n1 $TEST_STEP_NO $LOG_FILE
	TEST_STEP_NO=5 #1~5
	./retention_reclaim /dev/nvme0n1 $TEST_STEP_NO $LOG_FILE
done
