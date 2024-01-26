#!/usr/bin/env bash

input=$(mktemp)
binary=$(mktemp)

trap "rm -f $input $binary" EXIT

function sseq()
{
	for ((i = $1; i < $2; i=i<<1)); do
		echo $i
	done
}

function do_base64()
{
	if [ -n "$(type -p base64)" ]; then
		base64 $1
	elif [ -n "$(type -p uuencode)" ]; then
		uuencode -m $1 data | sed 1d | sed '$d'
	else
		echo "No tool found for base64 encoding." >&2
		exit 1
	fi
}

for i in $(sseq 1 1024) 2048 10240;
do
	echo $i
	dd if=/dev/urandom of=$binary bs=1 count=$i &>/dev/null
	echo "-----BEGIN INITSTATE-----" > $input
	do_base64 $binary >> $input
	echo "-----END INITSTATE-----" >> $input
	./base64decode $input $binary
	if [ $? -ne 0 ]; then
		exit 1
	fi
done
