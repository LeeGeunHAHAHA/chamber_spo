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

fwup()
{
#  CA=3
  CA=$(($1 % 4))
#  if [[ $CA != 1 ]]; then
#    CA=3
#  fi
  nvme fw-commit $DEV_FILE -a $CA &
  echo "  => Commit Action $CA"
}

gcc ctrl_reset.c -o ctrl_reset

trap _term SIGINT

#echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
#sleep 1
#echo 1 > /sys/bus/pci/rescan

killall prim_proc 2&> /dev/null

FQA_SPDK=0
../../../setup.sh reset 2&> /dev/null
rmmod nvme nvme_core
modprobe nvme_core admin_timeout=15 io_timeout=10 multipath=0 max_retries=0
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
  fio --minimal randrw &> /dev/null &
  sleep 3
  nvme fw-download $DEV_FILE -f fw.bin
  sleep 3
  #nvme format $DEV_FILE
  fwup $i&
  sleep `printf "%.1f" "$((RANDOM%50))e-1"` &
  wait $!
  echo "  => Reset"
  ./ctrl_reset $PCI_ADDR
  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 2
  echo 1 > /sys/bus/pci/rescan
  sleep 1
  echo "-----" $i/$REPEAT_COUNT " end -----"
  ../../../setup.sh reset 2&> /dev/null
  sleep 1
done

