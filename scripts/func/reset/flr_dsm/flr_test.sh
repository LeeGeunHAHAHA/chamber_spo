#!/usr/bin/env bash

FIO_SCRIPT=randrw
PCI_ADDR=0000\:01\:00.0
DEV_FILE=/dev/nvme0
REPEAT_COUNT=10
TIMEOUT=10

_term()
{
  echo " * Caught SIGINT signal!"
  pkill -9 -P $$
  exit
}

trap _term SIGINT

FQA_SPDK=1
../../../setup.sh config 2&> /dev/null

sleep 3

rescan-ns

CTRL_ID=$(nvme id-ctrl $FQA_DEVICE | grep cntlid | awk '{print $3}')

nvme delete-ns $FQA_DEVICE -n 0xFFFFFFFF
nvme create-ns $FQA_DEVICE -c 0x10000 -s 0x10000 -f 2
nvme attach-ns $FQA_DEVICE -n 1 -c $CTRL_ID

sleep 3

for ((i=1;i<=REPEAT_COUNT;i++))
do
  FQA_SPDK=1
  ../../../setup.sh config 2&> /dev/null
  rescan-ns
  nvme id-ctrl $FQA_DEVICE > /dev/null
  res=$?
  if [[ $res != 0 ]]; then
    exit
  fi

  echo "----" $i/$REPEAT_COUNT " begin ----"
  echo "  => I/O ..."
  nbio dsm &> /dev/null &
  sleep 3
  echo "  => FLR"
  setpci -s $PCI_ADDR CAP_EXP+8.W=8000:8000
  sleep 1
  pkill -9 -P $$

  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 1
  echo 1 > /sys/bus/pci/rescan
  sleep 1
  echo "-----" $i/$REPEAT_COUNT " end -----"
done 2> /dev/null
