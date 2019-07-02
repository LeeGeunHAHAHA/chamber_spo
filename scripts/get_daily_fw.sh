#!/bin/sh
USER_ID=jspark
SERVER_IP=192.168.110.2
IMAGE_DIR=/user/share/fw_release/daily_fqa
IMAGE_NAME=bravo_fw.bin

rm -f $IMAGE_NAME
scp $USER_ID@$SERVER_IP:$IMAGE_DIR/$1 ./$IMAGE_NAME
