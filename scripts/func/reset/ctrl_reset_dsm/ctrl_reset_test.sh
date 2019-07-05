#!/usr/bin/env bash

PCI_ADDR=0000\:01\:00.0
REPEAT_COUNT=10
TIMEOUT=10

_term()
{
  echo " * Caught SIGINT signal!"
  pkill -9 -P $$
  exit
}

gcc ctrl_reset.c -o ctrl_reset

trap _term SIGINT

FQA_SPDK=1
../../../setup.sh config 2&> /dev/null

sleep 2

rescan-ns

CTRL_ID=$(nvme id-ctrl $FQA_DEVICE | grep cntlid | awk '{print $3}')

nvme delete-ns $FQA_DEVICE -n 0xFFFFFFFF
nvme create-ns $FQA_DEVICE -c 0x10000 -s 0x10000 -f 2
nvme attach-ns $FQA_DEVICE -n 1 -c $CTRL_ID


for ((i=1;i<=REPEAT_COUNT;i++))
do
  rescan-ns
  nvme id-ctrl $FQA_DEVICE > /dev/null
  res=$?
  if [[ $res != 0 ]]; then
      exit
  fi

  echo "----" $i/$REPEAT_COUNT " begin ----"
  echo "  => I/O ..."
  nbio dsm &> /dev/null &
  sleep 3 &
  wait $!
  echo "  => Reset"
  ./ctrl_reset $PCI_ADDR 2> /dev/null
  sleep 1

  killall nbio
  killall prim_proc

  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 1
  echo 1 > /sys/bus/pci/rescan
  sleep 2
  echo "-----" $i/$REPEAT_COUNT " end -----"
  ../../../setup.sh config 2&> /dev/null
  sleep 1
done 2> /dev/null

