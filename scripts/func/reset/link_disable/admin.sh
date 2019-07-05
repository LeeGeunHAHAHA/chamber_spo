#!/usr/bin/env bash

DEV_FILE=/dev/nvme0

while [[ -a $DEV_FILE ]];
do
  nvme id-ctrl $DEV_FILE &> /dev/null
  nvme smart-log $DEV_FILE &> /dev/null
  nvme get-feature $DEV_FILE -f 7 &> /dev/null
  nvme id-ns $DEV_FILE &> /dev/null
  nvme fw-log $DEV_FILE &> /dev/null
  nvme get-feature $DEV_FILE -f 7 &> /dev/null
  nvme id-ctrl $DEV_FILE &> /dev/null
  nvme error-log $DEV_FILE &> /dev/null
  nvme get-feature $DEV_FILE -f 7 &> /dev/null
  nvme id-ctrl $DEV_FILE &> /dev/null
  nvme smart-log $DEV_FILE &> /dev/null
  nvme get-feature $DEV_FILE -f 7 &> /dev/null

  wait
done
