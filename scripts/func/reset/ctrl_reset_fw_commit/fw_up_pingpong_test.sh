#!/bin/bash

FIO_SCRIPT=randrw
PCI_ADDR=0000\:01\:00.0
DEV_FILE=/dev/nvme0n1
REPEAT_COUNT=1000
TIMEOUT=10

FW_1=fw1.bin
FW_2=fw2.bin

_term()
{
  echo " * Caught SIGINT signal!"
  pkill -9 -P $$
  exit
}

fwup()
{
#  CA=3
  CA=$((RANDOM % 4))
#  if [[ $CA != 1 ]]; then
#    CA=3
#  fi
  nvme fw-commit $DEV_FILE -a $CA &
  echo "  => Commit Action $CA"
}

gcc ctrl_reset.c -o ctrl_reset

trap _term SIGINT

echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
sleep 1
echo 1 > /sys/bus/pci/rescan

../../../setup.sh reset
rmmod nvme nvme_core
modprobe nvme_core admin_timeout=15 io_timeout=10 multipath=0 max_retries=0
modprobe nvme
export FQA_SPDK=


for ((i=1;i<=REPEAT_COUNT;i++))
do
  fw_idx=$((i%2))
  if [ $fw_idx -eq 1 ];then
      fw_bin=$FW_1
  else
      fw_bin=$FW_2
  fi

  echo $fw_bin

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
  fio randrw &> /dev/null &
  #nvme format $DEV_FILE
  nvme fw-download $DEV_FILE -f $fw_bin
  fwup &
  sleep `printf "%.1f" "$((RANDOM%50))e-1"` &
  wait $!
  echo "  => Reset"
  ./ctrl_reset $PCI_ADDR
  sleep 1
  echo 1 > /sys/bus/pci/devices/$PCI_ADDR/remove
  sleep 2
  echo 1 > /sys/bus/pci/rescan
  echo "-----" $i/$REPEAT_COUNT " end -----"
done

