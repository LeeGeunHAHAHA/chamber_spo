#!/usr/bin/env bash
PORT=$1
FORMAT=$2
DATE=`date +%Y-%m-%d`
./run_chamber PORT FORMAT | tee >(ts > ./testlog/$DATE.log)