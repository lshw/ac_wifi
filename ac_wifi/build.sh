#!/bin/bash
cd `dirname $0`

me=`whoami`
if [ "$me" == "root" ] ; then
  home=/home/liushiwei
else
  home=~
fi

if [ -x $home/sketchbook/libraries ] ; then
 sketchbook=$home/sketchbook
else
 sketchbook=$home/Arduino
fi
CRC_MAGIC=$( grep CRC_MAGIC config.h | awk '{printf $3}' )
cd ..
rm -f ac_wifi/ac_wifi.bin

if ! [ -x lib/uncrc32 ] ; then
gcc -o lib/uncrc32 lib/uncrc32.c
fi
branch=`git branch |grep "^\*" |awk '{print $2}'`
a=`git rev-parse --short HEAD`
date=`git log --date=short -1 |grep ^Date: |awk '{print $2}' |tr -d '-'`
git_id=${a:0:7}
ver=$date-${a:0:7}
echo $ver

arduino=/opt/arduino-1.8.19
astyle  --options=$arduino/lib/formatter.conf ac_wifi/*.h ac_wifi/*.ino ac_wifi/*.c
rm -f ac_wifi/*.orig

arduinoset=$home/.arduino15
mkdir -p /tmp/${me}_build /tmp/${me}_cache
rm -f /tmp/${me}_build/ac_wifi.ino.bin

#传递宏定义 GIT_COMMIT_ID 到源码中，源码git版本
CXXFLAGS="-DGIT_COMMIT_ID=\"$git_id\" -DGIT_VER=\"$ver\" "

if [ "a$debug" == "a" ] ; then
debug=",dbg=Disabled,lvl=None____"
#debug=",dbg=Serial,lvl=WIFI"
#debug=",dbg=Serial,lvl=SSLTLS_MEMHTTP_CLIENTHTTP_SERVERCOREWIFIHTTP_UPDATEUPDATEROTAOOMMDNSHWDT"
fi

fqbn="esp8266:esp8266:generic:xtal=160,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,mmu=4816,non32xfer=fast,CrystalFreq=26,FlashFreq=80,FlashMode=qio,eesz=4M2M,led=2,sdk=nonosdk305,ip=hb2f$debug,wipe=none,baud=921600"

$arduino/arduino-builder \
-dump-prefs \
-logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries lib/libraries \
-fqbn=$fqbn \
-ide-version=10813 \
-build-path /tmp/${me}_build \
-build-cache /tmp/${me}_cache \
-warnings=none \
-prefs build.extra_flags="$CXXFLAGS" \
-build-cache /tmp/${me}_cache \
-prefs=build.warn_data_percentage=75 \
-verbose \
./ac_wifi/ac_wifi.ino
if [ $? != 0 ] ; then
  exit
fi
$arduino/arduino-builder \
-compile \
-logger=machine \
-hardware $arduino/hardware \
-hardware $arduinoset/packages \
-tools $arduino/tools-builder \
-tools $arduino/hardware/tools/avr \
-tools $arduinoset/packages \
-built-in-libraries $arduino/libraries \
-libraries lib/libraries \
-fqbn=$fqbn \
-ide-version=10813 \
-build-path /tmp/${me}_build \
-warnings=none \
-prefs build.extra_flags="$CXXFLAGS" \
-build-cache /tmp/${me}_cache \
-prefs=build.warn_data_percentage=75 \
-verbose \
./ac_wifi/ac_wifi.ino |tee /tmp/${me}_info.log

if [ -e /tmp/${me}_build/ac_wifi.ino.bin ] ; then
  grep "Global vari" /tmp/${me}_info.log |sed -n "s/^.* \[\([0-9]*\) \([0-9]*\) \([0-9]*\) \([0-9]*\)\].*$/RAM:使用\1字节(\3%),剩余\4字节/p"
  grep "Sketch uses" /tmp/${me}_info.log |sed -n "s/^.* \[\([0-9]*\) \([0-9]*\) \([0-9]*\)\].*$/ROM:使用\1字节(\3%)/p"

  cp -a /tmp/${me}_build/ac_wifi.ino.bin ac_wifi/ac_wifi.bin
  #把bin文件的crc32值修改为0
  lib/uncrc32 ac_wifi/ac_wifi.bin $CRC_MAGIC
  if [ "a$1" != "a"  ] ;then
    $arduino/hardware/esp8266com/esp8266/tools/espota.py -p 8266 -i $1 -f ac_wifi/ac_wifi.bin
  fi
fi
echo $ver
