#!/bin/bash

FIO_SCRIPT=randrw
PCI_ADDR=0000\:01\:00.0
REPEAT_COUNT=10
TIMEOUT=10

gcc nssr.c -o nssr

FQA_SPDK=1
../../../setup.sh config 2&> /dev/null

rescan-ns

CTRL_ID=$(nvme id-ctrl $FQA_DEVICE | grep cntlid | awk '{print $3}')

nvme delete-ns $FQA_DEVICE -n 0xFFFFFFFF
nvme create-ns $FQA_DEVICE -s 0x100000 -c 0x100000 -f 0
nvme create-ns $FQA_DEVICE -s 0x100000 -c 0x100000 -f 1
nvme create-ns $FQA_DEVICE -s 0x100000 -c 0x100000 -f 2
nvme create-ns $FQA_DEVICE -s 0x100000 -c 0x100000 -f 3
nvme create-ns $FQA_DEVICE -s 0x100000 -c 0x100000 -f 4
nvme attach-ns $FQA_DEVICE -n 1 -c $CTRL_ID
nvme attach-ns $FQA_DEVICE -n 2 -c $CTRL_ID
nvme attach-ns $FQA_DEVICE -n 3 -c $CTRL_ID
nvme attach-ns $FQA_DEVICE -n 4 -c $CTRL_ID
nvme attach-ns $FQA_DEVICE -n 5 -c $CTRL_ID

sleep 3

for ((i=1;i<=REPEAT_COUNT;i++))
do
  rescan-ns
  ../../../setup.sh config 2&> /dev/null
  echo "----" $i/$REPEAT_COUNT " begin ----"
  echo "  => I/O ..."
  nbio randrw.nbio &> /dev/null &
  sleep 5
  echo "  => NSSR"
  ./nssr $PCI_ADDR
  sleep 1
  pkill -9 -P $$
  killall prim_proc
  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 1
  echo 1 > /sys/bus/pci/rescan
  sleep 1
#  nvme list
  echo "-----" $i/$REPEAT_COUNT " end -----"
done 2> /dev/null

