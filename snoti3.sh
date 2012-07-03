#!/bin/bash
# bash script to prepend i3status with the output of a background process

exec 3< <($1)

echo "{\"version\":1}" || exit 1
echo "[" || exit 1

run=0
i3status | while :
do
    read i3line
    if [ $run -lt 2 ]
    then
        let run++
        continue
    fi
    read -t 1 bgline <&3
    if [ $run -eq 2 ]
    then
        let run++
        echo "[{\"full_text\":\"$bgline<--\"},${i3line:1}" || exit 1
        continue
    fi
    echo ",[{\"full_text\":\"$bgline<--\"},${i3line:2}" || exit 1
done 

