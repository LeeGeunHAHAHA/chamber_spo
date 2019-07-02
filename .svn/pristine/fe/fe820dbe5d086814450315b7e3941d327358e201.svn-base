#!/bin/bash

PWRCTL=/root/svn/fqa/tools/powerctl/powerctl
PWRDEV=/dev/ttyUSB0
REMOVE_PCI=/root/remove_fu6150.sh
RESCAN_PCI=/root/pci_rescan.sh
SPDK_SETUP=/root/svn/fqa/setup.sh

for ((i=1;i<10000;i++))
do
	echo "*** [$i/10000] ***"
	nbio replay.param 2> err.log

	if [ $? -ne 0 ];
	then
		echo "compare failed. check 'err.log'"
		exit -1
	fi

	#./hello.sh &
	nbio rndw.param &

	range=10
	sec=$RANDOM
	let "sec %= $range"
	let "sec += 5"
	echo "sleep $sec"
	sleep $sec
	$PWRCTL $PWRDEV 0
	sleep 1
	kill -SIGUSR1 $!
	echo "ssd off"
	sleep 5

	echo "spdk reset"
	$SPDK_SETUP reset

	echo "remove pci"
	$REMOVE_PCI
	sleep 1

	echo "ssd on"
	$PWRCTL $PWRDEV 1
	sleep 10

	echo "pci rescan"
	$RESCAN_PCI
	sleep 3

	echo "spdk setup"
	$SPDK_SETUP
	sleep 1
done
