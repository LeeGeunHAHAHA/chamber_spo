#!/bin/bash

FIO_SCRIPT=randrw
PCI_ADDR=$FQA_DEVICE
DEV_FILE=/dev/nvme0
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

FQA_SPDK=0
../../setup.sh reset 2&> /dev/null
rmmod nvme nvme_core
modprobe nvme_core admin_timeout=1 io_timeout=1 multipath=0 max_retries=0
modprobe nvme

sleep 3

RANGE=5
for ((i=1;i<=REPEAT_COUNT;i++))
do
  elapsed=0
  while [[ ! -a $DEV_FILE ]]
  do
    sleep 1
    elapsed=$((elapsed + 1))
    if [[ $elapsed -ge $TIMEOUT ]]; then
      echo "controller not ready!"
      exit
    fi
  done

  #nvme set-feature $DEV_FILE -f 0xa -v 1
  echo "----" $i/$REPEAT_COUNT " begin ----"
  echo "  => I/O ..."
  fio --minimal $FIO_SCRIPT &> /dev/null &
  sleep 3 &
  wait $!
  echo "  => Reset"
  ./ctrl_reset $PCI_ADDR
  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 1
  echo 1 > /sys/bus/pci/rescan
  sleep 1
  echo "-----" $i/$REPEAT_COUNT " end -----"
  ../../setup.sh reset 2&> /dev/null
  sleep 1
done

FQA_SPDK=1
../../setup.sh config 2&> /dev/null

