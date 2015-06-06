/*
   trietest.c

   Routing table test bed. For details please consult

      Stefan Nilsson and Gunnar Karlsson. Fast Address Look-Up
      for Internet Routers. International Conference of
      Broadband Communications (BC'97).

      http://www.hut.fi/~sni/papers/router/router.html

   The program is invoked in the following way:

      trietest routing_file [traffic_file] [n]

   The routing_file is a file describing an IPv4 routing table.
   Each line of the file contains three numbers bits, len, and next
   in decimal notation, where bits is the bitpattern and len is
   the lenght of the entry, and next is the corresponding next-hop
   address.

   The optional traffic_file should contain one decimal integer
   per line.

   To be able to measure the search time also for small instances,
   one can give an optional command line parameter n that indicates
   that the table should be searched n times.

   The code presented in this file has been tested with care but is
   not guaranteed for any purpose. The writer does not offer any
   warranties nor does he accept any liabilities with respect to
   the code.

   Stefan Nilsson, 4 nov 1997.

   Laboratory of Information Processing Science
   Helsinki University of Technology
   Stefan.Nilsson@hut.fi
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "trie.h"
#include "Good_32bit_Rand.h"

/*
   Read routing table entries from a file. Each entry is represented
   by three numbers: bits, len, and next in decimal notation, where
   bits is the bitpattern and len is the lenght of the entry, and
   next is the corresponding next-hop address.
*/
static int readentries(char *file_name,
                       entry_t entry[], int maxsize)
{
   int nentries = 0;
   word data, nexthop;
   int len;
   FILE *in_file;

   if (!(in_file = fopen(file_name, "rb"))) {
      perror(file_name);
      exit(-1);
   }

   while (fscanf(in_file, "%lu%i%lu", &data, &len, &nexthop) != EOF) {
      if (nentries >= maxsize) return -1;
      entry[nentries] = (entry_t) malloc(sizeof(struct entryrec));
      /* clear the 32-len last bits, this shouldn't be necessary
         if the routtable data was consistent */
      data = data >> (32-len) << (32-len);
      entry[nentries]->data = data;
      entry[nentries]->len = len;
      entry[nentries]->nexthop = nexthop;
      nentries++;
   }
   return nentries;
}

/*
   Read traffic from a file. Each address is an unsigned number
   representing the bit pattern.
*/
static int readtraffic(char *file_name,
                       word str[], int maxsize)
{
   int nstrings = 0;
   word s;
   FILE *in_file;

   if (!(in_file = fopen(file_name, "rb"))) {
      perror(file_name);
      exit(-1);
   }

   while (fscanf(in_file, "%lu", &s) != EOF && nstrings < maxsize) {
      if (nstrings >= maxsize) return -1;
      str[nstrings] = s;
      nstrings++;
   }
   return nstrings;
}

/* Write the first n lines of the array to stdout. */
void writeentries(entry_t t[], int nentries, int n)
{
   int i, j;

   if (nentries > n)
      nentries = n;
   printf("\nEntries (nexthop, entry):\n");

   for (i = 0;  i < nentries; i++) {
      printf("%5lu   ", t[i]->nexthop);
      for (j = 0; j < t[i]->len; j++) {
         printf("%1d", t[i]->data<<j>>31);
         if (j%8 == 7) printf(" ");
      }
      printf("\n");
   }
}

/*
   Search for the entries in 'testdata[]' in the table
   'table' 'repeat' times. The experiment is repeated 'n'
   times and statistics are computed.
*/
void run(word testdata[], int entries, int repeat,
         routtable_t table, int useInline, int n, int verbose)
{
   double time[100];  /* Repeat the experiment at most 100 times */
   double min, x_sum, x2_sum, aver, stdev;
   int i, j, k;

   volatile word res; /* The result of a search is stored in a */
                      /* volative variable to avoid optimization */

   /* Used by the inlined search code */
   node_t node;
   int pos, branch, adr;
   word bitmask;
   int preadr;
   word s;

   /* Used to record search pattern */
   /* static int searchDist[MAXENTRIES]; */

   if (!useInline) {
      for (i = 0; i < n; ++i) {
         clockon();
         for (j = 0; j < repeat; ++j)
            for (k = 0; k < entries; k++)
               res = find(testdata[k], table);
         clockoff();
         time[i] = gettime();
      }
   } else {
      for (i = 0; i < n; ++i) {
         clockon();
         for (j = 0; j < repeat; ++j)
            for (k = 0; k < entries; k++) {
               /********** Inline search **********/
               s = testdata[k];
               node = table->trie[0];
               pos = GETSKIP(node);
               branch = GETBRANCH(node);
               adr = GETADR(node);
               while (branch != 0) {
                  node = table->trie[adr + EXTRACT(pos, branch, s)];
                  pos += branch + GETSKIP(node);
                  branch = GETBRANCH(node);
                  adr = GETADR(node);
               }
               /* searchDist[adr]++; */
               /* was this a hit? */
               bitmask = table->base[adr].str ^ s;
               if (EXTRACT(0, table->base[adr].len, bitmask) == 0) {
                  res = table->nexthop[table->base[adr].nexthop];
                  goto end;
               }
               /* if not look in the prefix tree */
               preadr = table->base[adr].pre;
               while (preadr != NOPRE) {
                  if (EXTRACT(0, table->pre[preadr].len, bitmask) == 0) {
                     res = table->nexthop[table->pre[preadr].nexthop];
                     goto end;
                  }
                  preadr = table->pre[preadr].pre;
               }	
               res = 0; /* not found */
               end:
               /********* End inline search ********/
            }
         clockoff();
         time[i] = gettime();
      }
   }

   x_sum = x2_sum = 0;
   min = DBL_MAX;
   for (i = 0; i < n; ++i) {
      x_sum += time[i];
      x2_sum += time[i]*time[i];
      min = time[i] < min ? time[i] : min;
   }
   if (n > 1) {
      aver = x_sum / (double) n;
      stdev = sqrt (fabs(x2_sum - n*aver*aver) / (double) (n - 1));
      fprintf(stderr, "  min:%5.2f", min);
      fprintf(stderr, "  aver:%5.2f", aver);
      fprintf(stderr, "  stdev:%5.2f", stdev);
   }
   if (verbose) {
      fprintf(stderr, "  (");
      for (i = 0; i < n-1; ++i)
         fprintf(stderr, "%.2f,", time[i]);
      fprintf(stderr, "%.2f)", time[i]);
   }
   fprintf(stderr, "\n");
   fprintf(stderr, "  %.0f lookups/sec", repeat*entries/min);
   if (verbose)
      fprintf(stderr, " (%.2f sec, %i lookups)", min, repeat*entries);
   fprintf(stderr, "\n");

   /* Print information about the search distribution */
   /*
   fprintf(stdout, "Search distribution:\n");
   for (i = 0; i < table->basesize; i++) {
      fprintf(stdout, "%7d", searchDist[i]);
      if (i % 11 == 10)
         fprintf(stdout, "\n");
   }
   */
}

int main(int argc, char *argv[])
{
   #define MAXENTRIES 50000            /* An array of table entries */
   static entry_t entry[MAXENTRIES];
   int nentries;

   #define MAXTRAFFIC 1000000          /* Traffic */
   static word traffic[MAXTRAFFIC];
   int ntraffic;

   routtable_t table; /* The routing table */

   word *testdata;    /* The test data comes from either a traffic */
                      /* file, or it is generated from the rout table */

   int repeat;        /* Number of times to repeat the experiment */
   boolean traffsub;  /* did the user submit a traffic file? */
   int verbose = TRUE;

   int i, j;          /* Auxiliary variables */
   word temp;

   if (argc < 2 || argc > 4) {
      fprintf(stderr, "%s%s%s\n", "Usage: ", argv[0],
              " routing_file [traffic_file] [n]");
      return 1;
   } else if (argc == 4) {
      repeat = atoi(argv[3]);
      traffsub = TRUE;
   } else if (argc == 3)
      if (isdigit(*(argv[2]))) {
         traffsub = FALSE;
         repeat = atoi(argv[2]);
      } else {
         traffsub = TRUE; 
         repeat = 1;
   } else if (argc == 2) {
      traffsub = FALSE;
      repeat = 1;
   }

   if ((nentries = readentries(argv[1], entry, MAXENTRIES)) < 0) {
      fprintf(stderr, "Input file too large.\n");
      return 1;
   }

   fprintf(stderr, "%s%s", "Table file: ", argv[1]);
   fprintf(stderr, "  (%0d lines)\n", nentries);

   /* writeentries(entry, nentries, 100000); printf("\n"); */

   if (traffsub) {
      ntraffic = readtraffic(argv[2], traffic, MAXTRAFFIC);
      fprintf(stderr, "%s%s", "Traffic file: ", argv[2]);
      fprintf(stderr, "  (%0d lines)\n", ntraffic);
      if (ntraffic < 0) {
         fprintf(stderr, "%s\n", "Traffic file too large.");
         return -1;
      }
      testdata = traffic;
   } else { /* use data from routing table as traffic */
      testdata = (word *) malloc(nentries*sizeof(word));
      for (i = 0; i < nentries; i++)
         testdata[i] = entry[i]->data;
      /* permute the entries */
      for (i = 0; i < nentries - 1; i++) {
         j = i + (int) (good_drand()*(nentries - i - 1));
         temp = testdata[i];
         testdata[i] = testdata[j];
         testdata[j] = temp;
      }
      ntraffic = nentries;
   }
   if (repeat > 1)
      fprintf(stderr, "%s%0d\n", "Repeated: ", repeat);

   table = buildrouttable(entry, nentries, 0.50, 16, verbose);
   /* writerouttable(table); */
   routtablestat(table, verbose);

   fprintf(stderr, "Function search\n");
   run(testdata, ntraffic, repeat, table, FALSE, 8, verbose);
   fprintf(stderr, "Inline search\n");
   run(testdata, ntraffic, repeat, table, TRUE, 8, verbose);
   disposerouttable(table);
   /*
   for (i = 1; i <= 20; i++) {
      disposerouttable(table);
      table = buildrouttable(entry, nentries, 0.05*i, 0, FALSE);
      routtablestat(table, FALSE);
      run(testdata, ntraffic, repeat, table, TRUE, 8, FALSE);
   }
   */
   return 0;
}
