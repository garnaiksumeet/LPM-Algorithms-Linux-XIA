/*
   Routing table test bed:

   Originally written by:
      Stefan Nilsson and Gunnar Karlsson. Fast Address Look-Up
      for Internet Routers. International Conference of
      Broadband Communications (BC'97).

      http://www.hut.fi/~sni/papers/router/router.html

   The program is invoked in the following way:

      trietest routing_file [n]

   The routing_file is a file describing an XIDs and corresponding prefix.
   Each line of the file contains three strings space separated.
   The first string is the 160-bit XID represented by a 40-bit hex and
   the second string is the length of prefix in the bit pattern in decimal.
   The third string is the XID of the next principal.

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


   Any option given to the program other than in the above format
   then the program will misbehave.

   We assume the argv[1] is a file containing routing table. If not passed
   program fails.

   Garnaik Sumeet, Michel Machado 2015
   Modified for LPM Algorithms Testing in Linux-XIA
*/

#include "lc_trie.h"


unsigned char hextochar(char a, char b)
{
	unsigned char hn, ln;

	hn = a > '9' ? a - 'a' + 10 : a - '0';
	ln = b > '9' ? b - 'a' + 10 : b - '0';
	return ((hn << 4 ) | ln);
}
/*
   Read routing table entries from a space separated file. The file consists of
   three strings in each line. The first string is a 40-bit string in hex
   represents an xid entry in the table, the second string is the length of the
   prefix of the xid entry in the same line. Third string is the next hop xid.
*/
static int readentries(char *file_name, entry_t entry[], int maxsize)
{
	int nentries = 0;
	xid data, nexthop;
	int len;
	FILE *in_file;

	// Auxiliary variables
	char tmp_data[41] = {0};
	char tmp_nexthop[41] = {0};
	int loop;

	if (!(in_file = fopen(file_name, "rb")))
	{
		perror(file_name);
		exit(-1);
	}

	while (fscanf(in_file, "%s%i%s", &tmp_data, &len, &tmp_nexthop) != EOF)
	{
		if (nentries >= maxsize) return -1;
		entry[nentries] = (entry_t) malloc(sizeof(struct entryrec));

		for (loop=0;loop<20;loop++)
		{
			data.w[loop] = hextochar(tmp_data[2*loop], tmp_data[2*loop + 1]);
			nexthop.w[loop] = hextochar(tmp_nexthop[2*loop], tmp_nexthop[2*loop + 1]);
		}
		// extract the prefix from the bit pattern
		for (loop=0;loop<(len/8);loop++);
		data.w[loop] = data.w[loop] >> (8 - (len%8)) << (8 - (len%8));
		loop++;
		for (;loop<20;loop++)
			data.w[loop] = (char) 0;

		entry[nentries]->data = data;
		entry[nentries]->len = len;
		entry[nentries]->nexthop = nexthop;
		nentries++;
	}
	return nentries;
}

void run(routtable_t table, int repeat)
{
	int i;
	volatile xid res; /* The result of a search is stored in a */
                      /* volative variable to avoid optimization */
}

int main(int argc, char *argv[])
{
	#define MAXENTRIES 50000            /* An array of table entries */
	static entry_t entry[MAXENTRIES];
	int nentries;

	routtable_t table; /* The routing table */

	word *testdata;    /* It is generated from the rout table */
	int ntraffic;

	int repeat;        /* Number of times to repeat the experiment */
	int verbose = TRUE;

	int i, j;          /* Auxiliary variables */

	if (argc < 2 || argc > 3)
	{
		fprintf(stderr, "%s%s%s\n", "Usage: ", argv[0], " routing_file [n]");
		return 1;
	}
	else if (argc == 3)
		repeat = atoi(argv[2]);
	else if (argc == 2)
		repeat = 1;

	if ((nentries = readentries(argv[1], entry, MAXENTRIES)) < 0)
	{
		fprintf(stderr, "Input file too large.\n");
		return 1;
	}

	fprintf(stderr, "%s%s", "Table file: ", argv[1]);
	fprintf(stderr, "  (%0d lines)\n", nentries);

	if (repeat > 1)
		fprintf(stderr, "%s%0d\n", "Repeated: ", repeat);

	table = buildrouttable(entry, nentries);
	run(table, repeat);
	disposerouttable(table);
	return 0;
}
