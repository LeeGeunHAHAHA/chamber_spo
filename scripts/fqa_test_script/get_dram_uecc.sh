#!/bin/bash

FQA_DEVICE=/dev/nvme0
OUTPUT_FILE=dram_uecc_info.out
LOG_FILE=dram_ecc_log.csv
NSID=1
DRAM_ECC_LOGGING_COUNT_LIMIT=1440
DATA_LEN=12

set -e

function parse_dram_uecc_info()
{
	string=$(hexdump -v -e '/1 "%02X\n"' $1)
	array=(${string//' '/ })

	logging_count=$(echo "ibase=16; ${array[3]}${array[2]}${array[1]}${array[0]}" | bc)									# logging_count
	echo "total_logging_count,$logging_count" > $LOG_FILE
	echo "" >> $LOG_FILE

	echo "elapsed_time(sec),dram_err_cnt,dram_err_info,ctrl_junction_temp('C)" >> $LOG_FILE
	for ((i=0; i<$DRAM_ECC_LOGGING_COUNT_LIMIT; i++))
	do
		index=$(echo "($i*$DATA_LEN) + 4" | bc)
		elapsed_time=$(echo "ibase=16; ${array[index+3]}${array[index+2]}" | bc)										# elapsed_time
		elapsed_time=$(echo "$elapsed_time * 10" | bc)		# convert to sec
		
		if [ $elapsed_time -eq 0 ];then
			break
		fi

		dram_err_cnt=$(echo "ibase=16; ${array[index+7]}${array[index+6]}${array[index+5]}${array[index+4]}" | bc)		# dram_err_cnt
		dram_err_info=$(echo "ibase=16; ${array[index+11]}${array[index+10]}${array[index+9]}${array[index+8]}" | bc)	# dram_err_info
		ctrl_junction_temp=$(echo "ibase=16; ${array[index+1]}${array[index]}" | bc)									# ctrl_junction_temp

		printf "%d,%d,0x%08X,%d\n" $elapsed_time $dram_err_cnt $dram_err_info $ctrl_junction_temp >> $LOG_FILE
	done
}

function send_vu_cmd_dram_uecc_info()
{
	VU_COMMON_OPCODE=0xC2
	VU_SUB_OPCODE=0x80000E01 #SPBLK_EC_COUNT
	DATA_LEN_BYTE=0x4400
	PARAM0=0x00
	PARAM1=0X00
	PARAM2=0X00
	PARAM3=0X00
	NDT=$[$DATA_LEN_BYTE/4]

	nvme io-passthru $FQA_DEVICE --opcode $VU_COMMON_OPCODE --namespace-id $NSID \
	--cdw2=$VU_SUB_OPCODE --read --raw-binary --data-len=$DATA_LEN_BYTE --cdw10=$NDT \
	--cdw12=$PARAM0 --cdw13=$PARAM1 --cdw14=$PARAM2 --cdw15=$PARAM3 > $OUTPUT_FILE

	# Parse byte table
	parse_dram_uecc_info "$OUTPUT_FILE"
	rm -rf "$OUTPUT_FILE"
}

echo "DRAM ECC Log Extraction start."
echo ""

if [ -f $LOG_FILE ]; then
	echo "[Warning] There exists a previous log file($LOG_FILE)."
	read -r -p "It will overwrite the log file. Are you sure? [y/N] " response
	case "$response" in
		[yY])
			;;
		*)
			echo "Aborted"
			exit
			;;
	esac
fi
send_vu_cmd_dram_uecc_info

cat $LOG_FILE | sed -e "s/,/   /g"

echo ""
echo "DRAM ECC Log Extraction done."
