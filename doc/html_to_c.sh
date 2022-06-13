#!/bin/bash
if  [ $1 ] ; then
  t=$1
else
  t=tu1.html
fi
sed "s/^[ \t]*//" $t |sed "s/[ ]*$//" |sed "s/$/\\\\/g"
