#!/bin/bash

##### VALUE
ADMIN_VS_OP=0xc4
SUB_OP_SET_PERST=0x80000
DEV_ID=/dev/nvme0

##### CONFIGURATION
SUB_OP=$SUB_OP_SET_PERST
num_param=$#

if [ ${num_param} -eq 0 ];then
	MDELAY=10
else
	MDELAY=$1
fi

echo MDELAY $MDELAY

FQA_SPDK=0
../../../setup.sh reset 2&> /dev/null

##### FUNCTION
nvme admin-passthru $DEV_ID -o $ADMIN_VS_OP --cdw2=$SUB_OP --cdw12=$MDELAY
