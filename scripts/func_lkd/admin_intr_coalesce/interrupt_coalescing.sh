#!/bin/bash
# Interrupts coalescing

FQA_BLK_DEVICE=/dev/nvme0
CMDS_PER_INTERRUPT=8
TIME_AND_THR=0xFF08

NVME_CLI_LKD=/usr/sbin/nvme


FEAT_SEL_CUR=0
FEAT_SEL_DEF=1
FEAT_SEL_SAV=2
FEAT_SEL_CAP=3

FEAT_ID_INTR_COALESCE=8
FEAT_ID_INTR_VECTOR_CONFIG=9
FEAT_ID=$FEAT_ID_INTR_COALESCE

TIMEFORMAT='%3R'

$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --value=0

CPUS=`cat /proc/cpuinfo | grep processor | awk '{if (max < $3) max=$3;} END {print max}'`
CPUS=$(($CPUS + 1))
echo "Number of CPUS: $(($CPUS+1))"
intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#echo "current total nvme interrupts: $INTR_PREV"
#echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

for (( i=0 ; i<$CMDS_PER_INTERRUPT ; i++))
do
	time taskset -c 0 $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null
done

INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#echo "current total nvme interrupts: $INTR_AFTER"
echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"

$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_CUR 
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_DEF 
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_SAV
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_CAP 

$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --value=$TIME_AND_THR
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_CUR 
$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --value=0xcafe --save
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_CUR 
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID --sel=$FEAT_SEL_SAV 

for (( cpuid=0 ; cpuid < $CPUS ; cpuid++ ))
do
	intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "current total nvme interrupts: $INTR_PREV"
	echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

	echo "CPU (iv) $cpuid"	

	for (( i=0 ; i< $(($CMDS_PER_INTERRUPT - 1)) ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null &
	done

	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"

	echo "disable interrupt coalescing for iv $cpuid"
	$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_VECTOR_CONFIG --value=$(($cpuid+65536))

	intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "current total nvme interrupts: $INTR_PREV"
	echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

	echo "CPU (iv) $cpuid"	

	for (( i=0 ; i< $(($CMDS_PER_INTERRUPT - 1)) ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null
	done

	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"

	echo "Re enabling interrupt coalescing for iv $cpuid"
	$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_VECTOR_CONFIG --value=$(($cpuid))

	intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "current total nvme interrupts: $INTR_PREV"
	echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

	echo "CPU (iv) $cpuid"	

	for (( i=0 ; i< $(($CMDS_PER_INTERRUPT - 1)) ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null &
	done

	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"
done
