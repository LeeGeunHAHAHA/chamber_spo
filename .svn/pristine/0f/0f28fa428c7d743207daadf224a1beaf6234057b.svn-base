#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
FQA_TEST_SCRIPT_DIR=fqa_test_script

# ps -ef | grep /bin/bash


ps aux | grep '/bin/bash' | grep -v 'grep'

echo ""

echo 'kill process...'

echo ""

for pid in `ps aux | grep '/bin/bash' | grep -v 'grep' | grep -v 'kill_process.sh' | grep -v 'monitoring_log.sh' | awk '{print $2}'`; do
	echo kill $pid
	kill -9 $pid
done

echo ""

echo 'done.'