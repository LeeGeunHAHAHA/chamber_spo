#!/usr/bin/env bash

rm telemetry_log.bin

cd ../nvme-cli-1.6/
./nvme telemetry-log /dev/nvme0n1 -o ../logan/telemetry_log.bin -c -d 3
cd ../logan
./logan telemetry_log.bin > telemetry_log.txt
cat telemetry_log.txt