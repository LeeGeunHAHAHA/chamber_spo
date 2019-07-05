#!/usr/bin/env bash

rm raid_recovery -f
rm raid_recovery_runtime -f

gcc raid_recovery.c -o raid_recovery
gcc raid_recovery_runtime.c -o raid_recovery_runtime
gcc raid_recovery_ext.c -o raid_recovery_ext

TEST_LOOPS=1
LOG_FILE=raid_recovery.log

for ((loop=1; loop<=$TEST_LOOPS; loop++))
do
	#printf "\n[Loops: $loop][Mode:TEST_MODE_BASIC_CLOSED]\n"
	TEST_MODE=1 #TEST_MODE_BASIC_CLOSED
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_SEQ_READ_CLOSED]\n"
	TEST_MODE=2 #TEST_MODE_SEQ_READ_CLOSED
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_MULTI_PLANE_ERROR]\n"
	TEST_MODE=3 #TEST_MODE_MULTI_PLANE_ERROR
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_BASIC_OPEN]\n"
	TEST_MODE=4 #TEST_MODE_BASIC_OPEN
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_APPEND_AFTER_OPEN_RAID]\n"
	TEST_MODE=5 #TEST_MODE_APPEND_AFTER_OPEN_RAID
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_UNRECOVERABLE_CLOSED]\n"
	TEST_MODE=6 #TEST_MODE_UNRECOVERABLE_CLOSED
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_UNRECOVERABLE_CLOSED_EXT]\n"
	TEST_MODE=7 #TEST_MODE_UNRECOVERABLE_CLOSED_EXT
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_UNRECOVERABLE_OPEN]\n"
	TEST_MODE=8 #TEST_MODE_UNRECOVERABLE_OPEN
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_RAND_READ_CLOSED]\n"
	TEST_MODE=9 #TEST_MODE_RAND_READ_CLOSED
	#./raid_recovery /dev/nvme0n1 $TEST_MODE $LOG_FILE

	printf "\n[Loops: $loop][Mode:TEST_MODE_RANDR_RUNTIME_CLOSED_RAID]\n"
	TEST_MODE=20 #TEST_MODE_RANDR_RUNTIME_CLOSED_RAID
	./raid_recovery_runtime /dev/nvme0n1 $TEST_MODE $LOG_FILE

	printf "\n[Loops: $loop][Mode:TEST_MODE_RANDR_RUNTIME_OPEN_RAID]\n"
	TEST_MODE=21 #TEST_MODE_RANDR_RUNTIME_OPEN_RAID
	./raid_recovery_runtime /dev/nvme0n1 $TEST_MODE $LOG_FILE

	printf "\n[Loops: $loop][Mode:TEST_MODE_RANDRW_RUNTIME_RAID]\n"
	TEST_MODE=22 #TEST_MODE_RANDRW_RUNTIME_RAID
	./raid_recovery_runtime /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_FTL_CLOSED]\n"
	TEST_MODE=30 #TEST_MODE_FTL_CLOSED
	#./raid_recovery_ext /dev/nvme0n1 $TEST_MODE $LOG_FILE

	#printf "\n[Loops: $loop][Mode:TEST_MODE_FTL_CLOSED_RECOVERY]\n"
	TEST_MODE=31 #TEST_MODE_FTL_CLOSED_RECOVERY
	#./raid_recovery_ext /dev/nvme0n1 $TEST_MODE $LOG_FILE
done
