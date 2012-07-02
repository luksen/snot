#!/bin/bash
# bash script to prepend i3status with the output of a background process

exec 3< <($1)

i3status | while :
do
    read i3line
    read -t 1 bgline <&3
    echo "$bgline | $i3line" || exit 1
done 

