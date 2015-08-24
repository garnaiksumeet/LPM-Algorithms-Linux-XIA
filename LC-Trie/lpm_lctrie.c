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
#include <assert.h>

static int fib_format(entry_t entry[], struct nextcreate *table,
		unsigned long size)
{
	int i;
	xid tmp;

	for (i = 0; i < size; i++) {
		memcpy(&(entry[i]->data), table[i].prefix, HEXXID);
		entry[i]->len = table[i].len;
		// This is to copy the value of unsigned int from struct
		// nextcreate to struct entry_t
		memcpy(&tmp, &(table[i].nexthop), sizeof(unsigned int));
		tmp = shift_left(tmp, (ADRSIZE - sizeof(unsigned int)));
		entry[i]->nexthop = tmp;
		entry[i]->pre = -1;
	}

	return 0;
}

struct routtablerec *lctrie_create_fib(struct nextcreate *table,
		unsigned long size)
{
	entry_t entry[size];
	int nentries;
	int i;

	struct entryrec *tmp_entry = calloc(size, sizeof(struct entryrec));
	for (i = 0; i < size; i++)
		entry[i] = (tmp_entry + i);
	assert(0 == fib_format(entry, table, size));

	return buildrouttable(entry, size);
}

int lctrie_destroy_fib(struct routtablerec *rtable)
{
	free(rtable->trie);
	free(rtable->base);
	free(rtable->nexthop);
	free(rtable);

	return 0;
}
