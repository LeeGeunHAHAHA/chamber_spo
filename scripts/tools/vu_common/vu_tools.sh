#!/bin/bash

VU_COMMON_OPCODE=0xC2

function get_vu_read_buf_data()
{
        string=$(hexdump -v -e '/4 "0x%08X\n"' temp_out)
        #echo $string
        array=(${string//' '/ })
        for i in "${array[@]}"
        do
                vu_read_buf[$i]=$(printf "%d\n" $i)
                #echo ${vu_read_buf[$i]}
        done
}

function vu_read_common()
{
	local dev=$1
	local nsid=$2
	local sub_opcode=$3
	local data_len=$4
	local param0=$5
	local param1=$6
	local param2=$7
	local param3=$8
	local ndt=$[$data_len/4]

	nvme io-passthru $dev --opcode $VU_COMMON_OPCODE --namespace-id $2 \
	--cdw2=$sub_opcode --read --raw-binary --data-len=$data_len --cdw10=$ndt \
	--cdw12=$param0 --cdw13=$param1 --cdw14=$param2 --cdw15=$param3 > temp_out

	get_vu_read_buf_data

	rm temp_out
}

function vu_write_common()
{
	local dev=$1
	local nsid=$2
	local sub_opcode=$3
	local data_len=$4
	local param0=$5
	local param1=$6
	local param2=$7
	local param3=$8
	local ndt=$[$data_len/4]

	touch ./temp_in

	nvme io-passthru $dev --opcode $VU_COMMON_OPCODE --namespace-id $2 \
	--cdw2=$sub_opcode --write --input-file=./temp_in --data-len=$data_len --cdw10=$ndt \
	--cdw12=$param0 --cdw13=$param1 --cdw14=$param2 --cdw15=$param3

	rm temp_in
}


