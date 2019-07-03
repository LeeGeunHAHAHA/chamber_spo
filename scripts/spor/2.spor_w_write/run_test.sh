#!/usr/bin/env bash
PORT=$1
FORMAT=$2
DATE=`date +%Y-%m-%d_%H_%M`
./run_chamber $PORT $FORMAT | tee >(ts > test_log/$DATE.log)
