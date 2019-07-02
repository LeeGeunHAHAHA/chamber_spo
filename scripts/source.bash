export FQA_TARGET=native
export FQA_DEVICE=0000:03:00.00000:04:00.00000:07:00.0
if [[ -z $FQA_DEVICE ]]; then
  echo "DUT not found. set FQA_DEVICE manually."
fi
export FQA_SPDK_MEM_SIZE=4096
export FQA_SPDK=1
export FQA_SPDK_SHM_ID=0
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/root/fqa/trunk/tools/spdk/build/lib
export PATH=/root/fqa/trunk/tools/bin:$PATH
