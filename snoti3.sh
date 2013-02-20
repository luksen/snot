#!/bin/bash
# bash script to prepend i3status with the output of a background process

bin="snot"
color="#87afff"



exec 3< <("$bin")

run=0
i3status | while :
do
	read i3line
	# let i3status handle the first two lines:
	#	{\"version\":1}
	#	[
	if [ $run -lt 2 ]
	then
		echo "$i3line"
		let run++
		continue
	fi
	read -t 1 bgline <&3
	# first data line without preceding comma
	if [ $run -eq 2 ]
	then
		let run++
		echo "[{\"full_text\":\"$bgline\",\"color\":\"$color\"},${i3line:1}" || exit 1
		continue
	fi
	# normal operation
	echo ",[{\"full_text\":\"$bgline\",\"color\":\"$color\"},${i3line:2}" || exit 1
done 
