#!/bin/bash

FIO_SCRIPT=randrw
PCI_ADDR=0000\:01\:00.0
DEV_FILE=/dev/nvme0
CTRL_FILE=/dev/nvme0
REPEAT_COUNT=10
TIMEOUT=10

_term()
{
  echo " * Caught SIGINT signal!"
  pkill -9 -P $$
  exit
}

trap _term SIGINT

FQA_SPDK=0
../../../setup.sh reset 2&> /dev/null
rmmod nvme nvme_core
modprobe nvme_core admin_timeout=30 io_timeout=1 multipath=0 max_retries=0
modprobe nvme

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
  sleep 5

  CTRL_ID=$(nvme id-ctrl $CTRL_FILE | grep cntlid | awk '{print $3}')

  nvme delete-ns $CTRL_FILE -n 0xffffffff
  nvme create-ns $CTRL_FILE -s 0xba600000 -c 0xba600000
  nvme attach-ns $CTRL_FILE -c $CTRL_ID -n 1
  nvme format $DEV_FILE
  nvme fw-commit $CTRL_FILE -a 2
  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 1
  echo 1 > /sys/bus/pci/rescan
  sleep 1
  ../../../setup.sh reset 2&> /dev/null
  sleep 1
  echo "-----" $i/$REPEAT_COUNT " end -----"
done

