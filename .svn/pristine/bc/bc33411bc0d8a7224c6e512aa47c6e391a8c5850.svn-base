#!/usr/bin/env bash
FQA_TRUNK_DIR=~/fqa/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script

$PATH_FQA_TEST_SCRIPT/get_fw_info.sh

ulimit -c unlimited

cd $FQA_TRUNK_DIR
source source.bash
./setup.sh

cd ./func
ftest ./func.lst
