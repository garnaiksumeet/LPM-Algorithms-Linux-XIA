#!/bin/bash

# This is a script that compiles individual unit tests and runs them and
# compares them with the expected outputs to check if they failed or passed.
#
# Input file	: the file that is passed as argument to each unit (I)
# Compare file	: the file that consists of expected result to Input file (C)
# Output file	: the file that is produced by the unit and `diff`ed to check
# 		  for failure (O)
#
# File Naming format:
#	Input file-> replace .c with .inp
#	Compare file-> replace .c with .cmp
#	Output file-> replace .c with .out (output file is auto generated donot
#	do any changes or create a file with same name)
#
# File Formats (This is the format of each line of the files)
# ============
# 1) test-read-lctrie.c
#    I: xid (space) prefix-length (space) xid
#    C: xid (space) prefix-length (space) xid
#
# 2) test-compare-xids.c
#    I: xid (space) xid
#    C: xid
#
# 3) test-qsort.c
#    I: xid
#    C: xid
#
# 4) test-build-nexthop-table.c
#    I: xid (space) prefix-length (space) xid
#    C: xid
#
# 5) test-entries.c
#    I: xid (space) prefix-length (space) xid
#    C: xid (space) prefix-length (space) xid
#
# 6) test-bitwise-shift-left.c
#    I: xid (space) shifts
#    C: xid
#
# 7) test-bitwise-shift-right.c
#    I: xid (space) shifts
#    C: xid
#
# 8) test-extract.c
#    I: xid (space) length (space) pos
#    C: xid

declare -a arr=("test-read-lctrie" "test-compare-xids" "test-qsort"
"test-build-nexthop-table" "test-entries")

for i in "${arr[@]}"
do
	gcc -w "$i".c -o "$i"
done

for i in "${arr[@]}"
do
	./"$i" "$i".inp > "$i".out
	if diff "$i".out "$i".cmp > /dev/null;
	then
		echo "$i" Passed
	else
		rm "$i".out
		echo "$i" Failed
	fi
done
