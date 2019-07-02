#!/bin/bash

L_INDEX=0
LOG_FILE=plp_time_log_${FQA_DEVICE}.csv
DUMPPING_VALUE=04
#ln -sf $filename plp_time_log.csv

mkdir -p hex_log || true


GREEN='\033[1;32m'
COLOR_GREEN=$GREEN
NC='\033[0m'


echo $DUMPPING_VALUE
function parse_plp_time_info ()
{
	echo -e "${COLOR_GREEN}PLP time log Extraction start.${NC}"

	string=$(hexdump -v -e '/1 "%02X\n"' $1)
	array=(${string//' '/ })
	
	for ((margin_time=0; margin_time<100; margin_time++))
	do
		plp_margin_time[${margin_time}]=0
	done

	for ((idx=0; idx<1024; idx++))
	do
		l_arr_index_fm=$(expr ${L_INDEX} \* 4)
		l_arr_index_fm_op=$(expr ${L_INDEX} \* 4 + 1)
		arr_index_fm_op=$(expr ${idx} \* 4 + 1)
		arr_index_fm=$(expr ${idx} \* 4)

		if [ "${array[arr_index_fm_op]}" == $DUMPPING_VALUE ] && [ "${array[l_arr_index_fm_op]}" != "${array[arr_index_fm_op]}" ]; then
			hex_dumpping_start_time=${array[arr_index_fm]}
			dumpping_start_time=$(echo "ibase=16; ${hex_dumpping_start_time}" | bc)
		fi

		if [ "${array[l_arr_index_fm_op]}" != "${array[arr_index_fm_op]}" ]; then
			plp_time=${array[l_arr_index_fm]} 
			select_plp_time=${array[l_arr_index_fm_op]}

			case ${select_plp_time} in
				
				00) intr_ms=$(echo "ibase=16; ${plp_time}" | bc);;
				01) wait_pss0_ms=$(echo "ibase=16; ${plp_time}" | bc);;
				02) wait_pss1_ms=$(echo "ibase=16; ${plp_time}" | bc);;
				03) plp_ready_ms=$(echo "ibase=16; ${plp_time}" | bc);;
				04) dumping_ms=$(echo "ibase=16; ${plp_time}" | bc);;
				05) dumpped_ms=$(echo "ibase=16; ${plp_time}" | bc);;
				06) event_ms=$(echo "ibase=16; ${plp_time}" | bc);;
			esac
		fi

		L_INDEX=${idx}

	done

	echo ${intr_ms}, ${wait_pss0_ms}, ${wait_pss1_ms}, ${plp_ready_ms}, ${dumpping_start_time}, ${dumping_ms}, ${dumpped_ms}, ${event_ms} >> ${LOG_FILE}
	echo -e "${COLOR_GREEN}PLP time log Extraction done.${NC}"
}

function send_vu_cmd_plp_time_info ()
{
	OUTPUT_FILE=./hex_log/plp_time_info_${FQA_DEVICE}_${1}.hex
	VU_COMMON_OPCODE=0xC4
	VU_SUB_OPCODE=0x301
	DATA_LEN_BYTE=0x1000
	PARAM0=0x00
	PARAM1=0X00
	PARAM2=0X00
	PARAM3=0X00
	NDT=1024
	plp_FQA_DEVICE=$(echo /dev/$(ls /sys/bus/pci/devices/${FQA_DEVICE}/nvme))
	
	/usr/sbin/nvme admin-passthru $plp_FQA_DEVICE -o $VU_COMMON_OPCODE --cdw2=$VU_SUB_OPCODE \
	--read --raw-binary --data-len=$DATA_LEN_BYTE --cdw10=$NDT > $OUTPUT_FILE 

	# Parse byte table
	parse_plp_time_info $OUTPUT_FILE
	# rm -rf $OUTPUT_FILE
}

# echo "intr_ms, wait_pss0_ms, wait_pss1_ms, plp_ready_ms, dumpping_start_time, dumping_ms, dumpped_ms, event_ms" > ${LOG_FILE}
# send_vu_cmd_plp_time_info 



