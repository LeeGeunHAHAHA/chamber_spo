setenv FQA_TARGET native
setenv FQA_DEVICE 0000:03:00.00000:04:00.00000:07:00.0
if (! $?FQA_DEVICE ) then
  echo "DUT not found. set FQA_DEVICE manually."
endif
setenv FQA_SPDK_MEM_SIZE 4096
setenv FQA_SPDK 1
setenv FQA_SPDK_SHM_ID 0
if (! $?LD_LIBRARY_PATH ) then
  setenv LD_LIBRARY_PATH /root/fqa/trunk/tools/spdk/build/lib
else
  setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/root/fqa/trunk/tools/spdk/build/lib
endif
set path = ( /root/fqa/trunk/tools/bin $path )
