#!/bin/bash
rm -f 1.log
minicom -D /dev/ttyUSB0 -b 4800 -R utf-8 -C 1.log
