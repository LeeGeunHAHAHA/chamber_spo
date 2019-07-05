export FQA_DEVICE=/dev/nvme0

nvme delete-ns $FQA_DEVICE -n 0xffffffff

count=1
while [ ${count} -le 128 ]; do
	nvme create-ns $FQA_DEVICE -s 0x1000 -c 0x1000 -f 0 -m 0
	count=$((${count}+1))
done

count=1
while [ ${count} -le 128 ]; do
	nvme attach-ns $FQA_DEVICE -n ${count} -c 0
	count=$((${count}+1))
done

nvme ns-rescan $FQA_DEVICE 
