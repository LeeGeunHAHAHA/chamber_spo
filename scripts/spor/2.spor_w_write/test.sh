PORT=$1


case $PORT in
	1)
		FQA_DEVICE=0000:07:00.0
		;;
	2)
		FQA_DEVICE=0000:08:00.0
		;;
	3)
		FQA_DEVICE=0000:03:00.0
		;;
	4)
		FQA_DEVICE=0000:04:00.0
		;;
esac

echo $PORT $FQA_DEVICE

PCI_ADDRESS=$FQA_DEVICE

echo $PCI_ADDRESS
