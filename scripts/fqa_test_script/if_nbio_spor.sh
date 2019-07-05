#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script
SPOR_SCRIPT_DIR=~/fqa/trunk/spor/2.spor_w_write

cd $FQA_TRUNK_DIR
source source.bash
./setup.sh

cd $SPOR_SCRIPT_DIR
./run_2 2>&1 | tee spor_if_4.log
