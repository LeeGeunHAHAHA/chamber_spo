#!/bin/bash


# format single namespace 512 byte
./setup_single_ns 2
./longrun_72hr

sleep 10

# format ns 16
./setup_ns16
./ns16_72hr.sh
