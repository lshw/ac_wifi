#!/bin/bash
#update.sh 192.168.1.2
#or by ttyUSB0/ttyS0
#update.sh

cd `dirname $0`
if [ "`ps |grep min[i]com`" ] ; then
 killall minicom 2>/dev/null
 sleep 3
fi

if [ -e /dev/ttyUSB0 ] ; then
 port=/dev/ttyUSB0
else
 port=/dev/ttyS0
fi
python3 ../esptool.py --chip esp8266 --port $port --after soft_reset --baud 460800 write_flash 0 ac_wifi.bin
#./run.sh
