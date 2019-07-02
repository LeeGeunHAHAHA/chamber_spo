#!/bin/bash
# 
# @file read.sh
# @date 2018. 06. 15
# @author mkpark
#

device=0000:01:00.0

CAP_OFFSET=0
VS_OFFSET=8
INTMS_OFFSET=12
INTMC_OFFSET=16
CC_OFFSET=20
Reserved1_OFFSET=24
CSTS_OFFSET=28
NSSR_OFFSET=32
AQA_OFFSET=36
ASQ_OFFSET=40
ACQ_OFFSET=48
CMBLOC_OFFSET=56
CMBSZ_OFFSET=60
BPINFO_OFFSET=64
BPRSEL_OFFSET=68
BPMBL_OFFSET=72
Reserved2_OFFSET=80
Reserved3_OFFSEt=3840

#				   CAP                VS         INTMS      INTMC       CC        Reserved1  CSTS       NSSR       AQA        ASQ                ACQ                CMBLOC     CMBSZ
DEFAULT_VAL_ARRAY=("00002030ff0303ff" "00010200" "00000000" "00000000" "00000000" "00000000" "00000000" "00000000" "00000000" "0000000000000000" "0000000000000000" "00000002" "0040001f")
SYMBOL_ARRAY=("CAP" "VS" "INTMS" "INTMC" "CC" "Reserved1" "CSTS" "NSSR" "AQA" "ASQ" "ACQ" "CMBLOC" "CMBSZ" "BPINFO" "BPRSEL" "BPMBL")

count=0
flag=0

printf "%-15s %-16s : %-16s\t   \t%s\n" Symbol Default Current [Pass/Fail]
echo ------------------------------------------------------------------
while [ $count -lt 13 ]
do
	case ${SYMBOL_ARRAY[${count}]} in
		CAP)
			cmd1=$(sudo ./reg_access $device read $CAP_OFFSET)
			cmd2=$(sudo ./reg_access $device read $((${CAP_OFFSET}+4)))

			flag=1
			;;
		VS)
			cmd1=$(sudo ./reg_access $device read $VS_OFFSET)
			flag=0
			;;
		INTMS)
			cmd1=$(sudo ./reg_access $device read $INTMS_OFFSET)
			flag=0
			;;
		INTMC)
			cmd1=$(sudo ./reg_access $device read $INTMC_OFFSET)
			flag=0
			;;
		CC)
			cmd1=$(sudo ./reg_access $device read $CC_OFFSET)
			flag=0
			;;
		Reserved1)
			cmd1=$(sudo ./reg_access $device read $Reserved1_OFFSET)
			flag=0
			;;
		CSTS)
			cmd1=$(sudo ./reg_access $device read $CSTS_OFFSET)
			flag=0
			;;
		NSSR)
			cmd1=$(sudo ./reg_access $device read $NSSR_OFFSET)
			flag=0
			;;
		AQA)
			cmd1=$(sudo ./reg_access $device read $AQA_OFFSET)
			flag=0
			;;
		ASQ)
			cmd1=$(sudo ./reg_access $device read $ASQ_OFFSET)
			cmd2=$(sudo ./reg_access $device read $((${ASQ_OFFSET}+4)))
			flag=1
			;;
		ACQ)
			cmd1=$(sudo ./reg_access $device read $ACQ_OFFSET)
			cmd2=$(sudo ./reg_access $device read $((${ACQ_OFFSET}+4)))
			flag=1
			;;
		CMBLOC)
			cmd1=$(sudo ./reg_access $device read $CMBLOC_OFFSET)
			flag=0
			;;
		CMBSZ)
			cmd1=$(sudo ./reg_access $device read $CMBSZ_OFFSET)
			flag=0
			;;
		BPINFO)
			cmd1=$(sudo ./reg_access $device read $BPINFO_OFFSET)
			flag=0
			;;
		BPRSEL)
			cmd1=$(sudo ./reg_access $device read $BPRSEL_OFFSET)
			flag=0
			;;
		BPMBL)
			cmd1=$(sudo ./reg_access $device read $BPMBL_OFFSET)
			cmd2=$(sudo ./reg_access $device read $((${BPMBL_OFFSET}+4)))
			flag=1
			;;
	esac

	if [ $flag -eq 0 ]
	then
		if [ $cmd1 = ${DEFAULT_VAL_ARRAY[${count}]} ]
		then
			printf "%-15s 0x%-16s : 0x%s\t\t =>" ${SYMBOL_ARRAY[${count}]} ${DEFAULT_VAL_ARRAY[${count}]} $cmd1
			echo -e "\x1b[32m\tPass\x1b[0m\n"
      	else
	    	printf "%-15s 0x%-16s : 0x%s\t\t =>" ${SYMBOL_ARRAY[${count}]} ${DEFAULT_VAL_ARRAY[${count}]} $cmd1
			echo -e "\x1b[31m\tFail\x1b[0m\n"
    	fi
	else
    	if [ $cmd2$cmd1 = ${DEFAULT_VAL_ARRAY[${count}]} ] 
		then
	        printf "%-15s 0x%-16s : 0x%s%s\t =>" ${SYMBOL_ARRAY[${count}]} ${DEFAULT_VAL_ARRAY[${count}]} $cmd2 $cmd1
			echo -e "\x1b[32m\tPass\x1b[0m\n"
      	else
	        printf "%-15s 0x%-16s : 0x%s%s\t =>" ${SYMBOL_ARRAY[${count}]} ${DEFAULT_VAL_ARRAY[${count}]} $cmd2 $cmd1
			echo -e "\x1b[31m\tFail\x1b[0m\n"
    	fi
	fi

	count=$(( $count + 1 ))
done

