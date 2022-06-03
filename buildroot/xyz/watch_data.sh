#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Script needs 1 argument : pid of process to watch"
    exit
fi

pid=$1 
watch "grep '^VmPeak\|^VmSize\|^VmHWM\|^VmRSS\|^VmSwap' /proc/$pid/status && echo "" && free -m && echo "" && tail -10 out_main.txt && echo "" && ls -ltrh /ballooning"