gcc -c -I ./dSFMT-src-2.2.1/ -I ./Data-Generation/ -I ./Bloom-Filter/ -I ./Radix-Trie/ -O3 -funroll-loops *.c
gcc -c -I ./dSFMT-src-2.2.1/ -I ./Data-Generation/ -I ./Bloom-Filter/ -I ./Radix-Trie/ -O3 -funroll-loops *.c ./Data-Generation/*.c ./Bloom-Filter/*.c ./Radix-Trie/*.c ./dSFMT-src-2.2.1/*.c
gcc -o test *.o /usr/local/lib/libhashit.so.1.0 -lgsl -lgslcblas -lm -lrt -O3 -funroll-loops
rm *.o
./test
rm test
R < plot.R --no-save
