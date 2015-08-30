gcc -c -I ./Data-Generation/ -I ./Bloom-Filter/ -I ./LC-Trie/ -O3 -funroll-loops *.c
gcc -c -I ./Data-Generation/ -I ./Bloom-Filter/ -I ./LC-Trie/ -O3 -funroll-loops *.c ./Data-Generation/*.c ./Bloom-Filter/*.c ./LC-Trie/*.c
gcc -o test *.o /usr/local/lib/libhashit.so.1.0 -lgsl -lgslcblas -lm -lrt -O3 -funroll-loops
rm *.o
./test
rm test
R < plot.R --no-save
