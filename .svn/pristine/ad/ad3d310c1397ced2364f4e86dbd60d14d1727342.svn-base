#!/bin/bash
#
# @file write.sh
# @data 2018. 06. 15
# @author mkpark
#

device=0000:01:00.0
val=255

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

#symbol, offset,  val(dec)
write() {
	printf "%-16s%s\t%s\t\t%s\n" "Symbol" "Write_Val" "Before" "After"
	cmd=$(sudo ./reg_access $device read $2)
	printf "%-16s%08x\t%s\t" INTMS 255 $cmd
	cmd=$(sudo ./reg_access $device write $2 $3)
	cmd=$(sudo ./reg_access $device read $2)
	printf "0x%s\n" $cmd
}

printf "%-16s%s\t\t%s\t\t\t%s\t\t\t%s\n" Symbol Write_Val Before After [Pass/Fail]
echo -----------------------------------------------------------

# AQA
aqa_val=$val
aqa_cmd_origin=$(sudo ./reg_access $device read $AQA_OFFSET)
printf "%-16s%08x\t\t%s\t\t" AQA $aqa_val $aqa_cmd_origin
write_cmd=$(sudo ./reg_access $device write $AQA_OFFSET $aqa_val)
aqa_cmd=$(sudo ./reg_access $device read $AQA_OFFSET)
printf "%s\t" $aqa_cmd

aqa_val=$(printf "%08x" $aqa_val)
if [ $aqa_val = $aqa_cmd ]
then
    echo -e "\x1b[32m\tPass\x1b[0m\n"
else
    echo -e "\x1b[31m\tFail\x1b[0m\n"
fi

aqa_val=$(printf "%d" "0x"$aqa_cmd_origin)
$(sudo ./reg_access $device write $AQA_OFFSET $aqa_val)

# ASQ
asq_val=$val
asq_cmd_origin1=$(sudo ./reg_access $device read $ASQ_OFFSET)
asq_cmd_origin2=$(sudo ./reg_access $device read $(($ASQ_OFFSET+1)))
printf "%-16s%08x%08x\t%s%s\t" ASQ $asq_val $asq_val $asq_cmd_origin2 $asq_cmd_origin1
write_cmd1=$(sudo ./reg_access $device write $ASQ_OFFSET $asq_val)
write_cmd2=$(sudo ./reg_access $device write $(($ASQ_OFFSET+1)) $asq_val)
asq_cmd1=$(sudo ./reg_access $device read $ASQ_OFFSET)
asq_cmd2=$(sudo ./reg_access $device read $(($ASQ_OFFSET+1)))
printf "%s%s\t" $asq_cmd2 $asq_cmd1

asq_val=$(printf "%08x%08x" $asq_val $asq_val)
if [ $asq_val = $asq_cmd1$asq_cmd2 ]
then
    echo -e "\x1b[32mPass\x1b[0m\n"
else
    echo -e "\x1b[31mFail\x1b[0m\n"
fi

asq_val1=$(printf "%d" "0x"$asq_cmd_origin1)
asq_val2=$(printf "%d" "0x"$asq_cmd_origin2)
$(sudo ./reg_access $device write $ASQ_OFFSET $asq_val1)
$(sudo ./reg_access $device write $((ASQ_OFFSET+1)) $asq_val2)

# ACQ
acq_val=$val
acq_cmd_origin1=$(sudo ./reg_access $device read $ACQ_OFFSET)
acq_cmd_origin2=$(sudo ./reg_access $device read $(($ACQ_OFFSET+1)))
printf "%-16s%08x%08x\t%s%s\t" ACQ $acq_val $acq_val $acq_cmd_origin2 $acq_cmd_origin1
write_cmd1=$(sudo ./reg_access $device write $ACQ_OFFSET $acq_val)
write_cmd2=$(sudo ./reg_access $device write $(($ACQ_OFFSET+1)) $val)
acq_cmd1=$(sudo ./reg_access $device read $ACQ_OFFSET)
acq_cmd2=$(sudo ./reg_access $device read $(($ACQ_OFFSET+1)))
printf "%s%s\t" $acq_cmd2 $acq_cmd1

acq_val=$(printf "%08x%08x" $acq_val $acq_val)
if [ $acq_val = $acq_cmd1$acq_cmd2 ]
then
    echo -e "\x1b[32mPass\x1b[0m\n"
else
    echo -e "\x1b[31mFail\x1b[0m\n"
fi

acq_val1=$(printf "%d" "0x"$acq_cmd_origin1)
acq_val2=$(printf "%d" "0x"$acq_cmd_origin2)
$(sudo ./reg_access $device write $ACQ_OFFSET $acq_val1)
$(sudo ./reg_access $device write $((ACQ_OFFSET+1)) $acq_val2)

