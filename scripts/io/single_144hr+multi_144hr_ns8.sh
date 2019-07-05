#!/bin/bash


# format single namespace 4k 
./setup_single_ns 1
./longrun_144hr

sleep 10

# format ns 16
./setup_ns16
cd ns8
./ns16_144hr.sh
