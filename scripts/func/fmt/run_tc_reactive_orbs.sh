#!/usr/bin/env bash

rm reactive_orbs -f
rm reactive_orbs_ext -f

gcc reactive_orbs.c -o reactive_orbs
gcc reactive_orbs_ext.c -o reactive_orbs_ext

TEST_LOOPS=1000

LOG_FILE1=reactive_orbs_mode1.log
LOG_FILE2=reactive_orbs_mode2.log

for ((loop=1; loop<=$TEST_LOOPS; loop++))
do
	#printf "\n[Loops: $loop][Mode:TEST_MODE_BASIC]\n"
	TEST_MODE=1
	#./reactive_orbs /dev/nvme0n1 $TEST_MODE $LOG_FILE1

	#printf "\n[Loops: $loop][Mode:TEST_MODE_BASIC_OPEN]\n"
	TEST_MODE=2
	#./reactive_orbs /dev/nvme0n1 $TEST_MODE $LOG_FILE1

	#printf "\n[Loops: $loop][Mode:TEST_MODE_HARD_WORDLOAD]\n"
	#TEST_MODE=3
	#./reactive_orbs /dev/nvme0n1 $TEST_MODE $LOG_FILE2

	printf "\n[Loops: $loop][Mode:TEST_MODE_EXT]\n"
	TEST_MODE_EXT=10
	./reactive_orbs_ext /dev/nvme0n1 $TEST_MODE $LOG_FILE2
done

rm reactive_orbs -f
rm reactive_orbs_ext -f