#!/bin/bash
cd ..
./setup_ns8
cd -
./ns8_18hr.sh 4 21
cd ..
./setup_ns8
cd -
./ns8_18hr.sh 4 24
cd ..
./setup_ns8
cd -
./ns8_24hr.sh 4 21
cd ..
./setup_ns8
cd -
./ns8_24hr.sh 4 24
cd ..
./setup_ns8
cd -
./ns8_36hr.sh 4 21
cd ..
./setup_ns8
cd -
./ns8_36hr.sh 4 24
cd ..
./setup_ns8
cd -
./ns8_48hr.sh 4 21
cd ..
./setup_ns8
cd -
./ns8_48hr.sh 4 24

for ((i=0;i<10;i++)); do
	cd ..
	./setup_ns8
	cd -
	./ns8_72hr.sh 4 21
	cd ..
	./setup_ns8
	cd -
	./ns8_72hr.sh 4 24
done

echo "done"
