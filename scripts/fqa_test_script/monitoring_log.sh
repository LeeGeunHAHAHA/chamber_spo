#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
FQA_TEST_SCRIPT_DIR=fqa_test_script


cd $FQA_TRUNK_DIR/$FQA_TEST_SCRIPT_DIR

touch ./log.txt

clear

tail -n 100 -f ./log.txt