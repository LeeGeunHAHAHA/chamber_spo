#!/bin/sh
USER_ID=sjkim
SERVER_IP=192.168.10.141
IMAGE_DIR=/user/$USER_ID/svn/fu6150_efw_clean/trunk/ext/tools/mkbootimg
IMAGE_NAME=hxl_fal_test.bin

scp $USER_ID@$SERVER_IP:$IMAGE_DIR/$IMAGE_NAME .
