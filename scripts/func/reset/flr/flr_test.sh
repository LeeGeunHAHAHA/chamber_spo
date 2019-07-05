#!/usr/bin/env bash

FIO_SCRIPT=randrw
PCI_ADDR=0000\:01\:00.0
DEV_FILE=/dev/nvme0n1
REPEAT_COUNT=10
TIMEOUT=10

FQA_SPDK=0
../../../setup.sh reset 2&> /dev/null
rmmod nvme nvme_core
modprobe nvme_core admin_timeout=1 io_timeout=1 multipath=0 max_retries=0
modprobe nvme

sleep 3

CTRL_ID=$(nvme id-ctrl $DEV_FILE | grep cntlid | awk '{print $3}')

nvme delete-ns $DEV_FILE -n 0xFFFFFFFF
nvme create-ns $DEV_FILE -c 0x10000 -s 0x10000 -f 2
nvme attach-ns $DEV_FILE -n 1 -c $CTRL_ID

sleep 3

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

  echo "----" $i/$REPEAT_COUNT " begin ----"
  echo "  => I/O ..."
  #nvme format $DEV_FILE &
  fio --minimal $FIO_SCRIPT &> /dev/null &
  sleep 3
  echo "  => FLR"
  setpci -s $PCI_ADDR CAP_EXP+8.W=8000:8000
  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 1
  echo 1 > /sys/bus/pci/rescan
  sleep 1
  echo "-----" $i/$REPEAT_COUNT " end -----"
  ../../../setup.sh reset 2&> /dev/null
  sleep 1
done
