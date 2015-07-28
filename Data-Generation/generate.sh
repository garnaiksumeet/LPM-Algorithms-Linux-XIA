#!/bin/bash

gcc -w generate-data.c -o generate -lgsl -lgslcblas -lgmp
printf "This is going to take a long time. It takes about an hour or so on an
Intel-i7 processor.\n\n"
./generate

for i in `seq 4 20`;
do
	rm "${i}.tmp" "${i}.len" "${i}.pre"
done

rm generate
