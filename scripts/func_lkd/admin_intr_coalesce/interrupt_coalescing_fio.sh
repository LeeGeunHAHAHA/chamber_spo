#!/bin/bash
# Interrupts coalescing

source ../common

FQA_BLK_DEVICE=/dev/nvme0
CMDS_PER_INTERRUPT=8
TIME_AND_THR_BEEF=0xBEEF
TIME_LIMIT_VALUE_ARRAY=(0x5FF 0xAFF 0x32FF 0x64FF 0xFFFF)
TIME_LIMIT_MSG_ARRAY=(500us 1000us 5000us 10000us 25500us)
THR_LIMIT_VALUE_ARRAY=(0xFF04 0xFF09 0xFF31 0xFF63 0xFFFF)
THR_LIMIT_MSG_ARRAY=(5 10 50 100 256)


NVME_CLI_LKD=/usr/sbin/nvme
TARGET_NS="/dev/nvme0n1"

FIO_QD=128
FIO=/usr/bin/fio
FIO_PARAM_COMMON="--ioengine=libaio --numjobs=1 --direct=1 --invalidate=1 --norandommap=1 --randrepeat=0 --group_reporting=1 --name=fu6150 --ramp_time=0 --output=summary.log --filename=$TARGET_NS"

QD_TIME_COALESCE=1
QD_THR_COALESCE=256
RUNTIME=60

FEAT_SEL_CUR=0
FEAT_SEL_DEF=1
FEAT_SEL_SAV=2
FEAT_SEL_CAP=3

FEAT_ID_INTR_COALESCE=8
FEAT_ID_INTR_VECTOR_CONFIG=9
FEAT_ID=$FEAT_ID_INTR_COALESCE

TIMEFORMAT='%3R'

echo "Set TIME and THR w/ 0xBE and 0xEF, respectively"
$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --value=$TIME_AND_THR_BEEF
print_separator
echo "Get current TIME and THR, Expected: 0xBEEF"
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --sel=$FEAT_SEL_CUR 
print_separator
echo "Get default TIME and THR, Expected: 0x0"
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --sel=$FEAT_SEL_DEF 
print_separator
echo "Get saved TIME and THR, Expected: 0x0"
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --sel=$FEAT_SEL_SAV
print_separator
echo "Get capabilyty for this feature: 0x6 (bit 2: changeable, bit 1: ns specific, bit 0: savable)"
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --sel=$FEAT_SEL_CAP 
print_separator
echo "Set and save TIME and THR w/ 0xCA and 0xFE, respectively, and this will fail"
$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --value=0xcafe --save
print_separator
echo "Get current TIME and THR, Expected: 0xBEEF (not changed by previously failed set-feature)"
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --sel=$FEAT_SEL_CUR 
print_separator
echo "Get saved TIME and THR, Expected: 0x0"
$NVME_CLI_LKD get-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --sel=$FEAT_SEL_SAV 
print_separator

CPUS=$(get_nr_of_cpus)
echo "Number of CPUS: $CPUS"

echo "THR based interrupt coalescing test"
FIO_QD=256
for idx in ${!THR_LIMIT_VALUE_ARRAY[*]}
do
	echo "Set TIME and THR w/ ${THR_LIMIT_VALUE_ARRAY[$idx]}, whose THR is ${THR_LIMIT_MSG_ARRAY[$idx]}"

	$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --value=${THR_LIMIT_VALUE_ARRAY[$idx]} > /dev/null 
	for (( cpuid=0 ; cpuid < $CPUS ; cpuid++ ))
	do
		echo "Running fio on CPU(iv) ${cpuid}"
#		echo "taskset -c $cpuid $FIO $FIO_PARAM_COMMON --iodepth=$FIO_QD"
		INTR_BEFORE=$(get_nr_of_nvme_interrupts)
		echo "INTR BEFORE: $INTR_BEFORE"
		taskset -c $cpuid $FIO $FIO_PARAM_COMMON --iodepth=$FIO_QD --runtime=$RUNTIME
		sleep 1
		NR_ISSUED_CMDS=$(get_nr_of_issued_read_cmds)
		echo "issued cmds: $NR_ISSUED_CMDS"
		INTR_AFTER=$(get_nr_of_nvme_interrupts)
		echo "INTR AFTER: $INTR_AFTER"

		INTR_DELTA=$((INTR_AFTER-INTR_BEFORE))
		CMDS_PER_INTR=${THR_LIMIT_MSG_ARRAY[$idx]}
		echo $CMDS_PER_INTR
		INTR_DELTA_EXP=$((NR_ISSUED_CMDS / CMDS_PER_INTR))
		echo "Interrupts increased: $INTR_DELTA, expected: $INTR_DELTA_EXP"
		print_separator
	done

done


echo "TIME based interrupt coalescing test"
FIO_QD=1
for idx in ${!TIME_LIMIT_VALUE_ARRAY[*]}
do
	echo "Set TIME and THR w/ ${TIME_LIMIT_VALUE_ARRAY[$idx]}, whose TIME is ${TIME_LIMIT_MSG_ARRAY[$idx]}"

	$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_COALESCE --value=${TIME_LIMIT_VALUE_ARRAY[$idx]} > /dev/null 
	for (( cpuid=0 ; cpuid < $CPUS ; cpuid++ ))
	do
		echo "Running fio on CPU(iv) ${cpuid}"
#		echo "taskset -c $cpuid $FIO $FIO_PARAM_COMMON --iodepth=$FIO_QD"
		taskset -c $cpuid $FIO $FIO_PARAM_COMMON --iodepth=$FIO_QD --runtime=$RUNTIME
		print_fio_latency
		print_separator
	done

done

exit
#CPUS=`cat /proc/cpuinfo | grep processor | awk '{if (max < $3) max=$3;} END {print max}'`
#CPUS=$(($CPUS + 1))
echo "Number of CPUS: $(($CPUS))"
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

for (( cpuid=0 ; cpuid < $CPUS ; cpuid++ ))
do
#	intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_PREV"
#	echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

	echo "CPU (iv) $cpuid coalescing enabled"	

	for (( i=0 ; i< $(($CMDS_PER_INTERRUPT - 1)) ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null &
	done

	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"

	echo "disable interrupt coalescing for iv $cpuid"
	$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_VECTOR_CONFIG --value=$(($cpuid+65536))

#	intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_PREV"
#	echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

	echo "CPU (iv) $cpuid coalescing disabled"	

	for (( i=0 ; i< $(($CMDS_PER_INTERRUPT - 1)) ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null
	done

	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"

	echo "Re enabling interrupt coalescing for iv $cpuid"
	$NVME_CLI_LKD set-feature $FQA_BLK_DEVICE --feature-id=$FEAT_ID_INTR_VECTOR_CONFIG --value=$(($cpuid))

#	intr_prev=`cat /proc/interrupts | grep nvme | awk '{print $2 " " $3 " " $4 " " $5 " " ; sum+=$2+$3+$4+$5} END {print "sum=",sum}'`
	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_PREV"
#	echo "current commands per interrupt: $CMDS_PER_INTERRUPT"

	echo "CPU (iv) $cpuid coalescing re-enabled"	

	for (( i=0 ; i< $(($CMDS_PER_INTERRUPT - 1)) ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null &
	done

	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"

	INTR_PREV=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
	echo "CPU (iv) $cpuid coalescing re-enabled many cmds"	

	for (( i=0 ; i< 256 ; i++))
	do
		time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null &
	done

#	time taskset -c $cpuid $NVME_CLI_LKD read /dev/nvme0n1 --start-block=0 --block-count=0 --data-size=4096 --data=/dev/null 2> /dev/null

	sleep 1
	INTR_AFTER=`cat /proc/interrupts | grep nvme | awk '{sum+=$2+$3+$4+$5} END {print sum}'`
#	echo "current total nvme interrupts: $INTR_AFTER"
	echo "interrupt difference: $(($INTR_AFTER - $INTR_PREV))"
done
