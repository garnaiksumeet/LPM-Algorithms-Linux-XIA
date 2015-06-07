/*

   Original Code:
   Stefan Nilsson and Gunnar Karlsson. Fast Address Look-Up
   for Internet Routers. International Conference of Broadband
   Communications (BC'97).

      http://www.hut.fi/~sni/papers/router/router.html

   The code presented in this file has been tested with care but is
   not guaranteed for any purpose. The writer does not offer any
   warranties nor does he accept any liabilities with respect to
   the code.

   Stefan Nilsson, 4 nov 1997.

   Laboratory of Information Processing Science
   Helsinki University of Technology
   Stefan.Nilsson@hut.fi


   Garnaik Sumeet, Michel Machado 2015
   Modified for LPM Algorithms Testing in Linux-XIA
*/
#include "lc_trie.h"


/* Return a nexthop or 0 if not found */
nexthop_t find(word s, routtable_t t)
{
   node_t node;
   int pos, branch, adr;
   word bitmask;
   int preadr;

   /* Traverse the trie */
   node = t->trie[0];
   pos = GETSKIP(node);
   branch = GETBRANCH(node);
   adr = GETADR(node);
   while (branch != 0) {
      node = t->trie[adr + EXTRACT(pos, branch, s)];
      pos += branch + GETSKIP(node);
      branch = GETBRANCH(node);
      adr = GETADR(node);
   }

   /* Was this a hit? */
   bitmask = t->base[adr].str ^ s;
   if (EXTRACT(0, t->base[adr].len, bitmask) == 0)
      return t->nexthop[t->base[adr].nexthop];

   /* If not, look in the prefix tree */
   preadr = t->base[adr].pre;
   while (preadr != NOPRE) {
      if (EXTRACT(0, t->pre[preadr].len, bitmask) == 0)
         return t->nexthop[t->pre[preadr].nexthop];
      preadr = t->pre[preadr].pre;
   }

   /* Debugging printout for failed search */
   /*
   printf("base: ");
   for (j = 0; j < 32; j++) {
      printf("%1d", t->base[adr].str<<j>>31);
      if (j%8 == 7) printf(" ");
   }
   printf("  (%lu)  (%i)\n", t->base[adr].str, t->base[adr].len);
   printf("sear: ");
   for (j = 0; j < 32; j++) {
      printf("%1d", s<<j>>31);
      if (j%8 == 7) printf(" ");
   }
   printf("\n");
   printf("adr: %lu\n", adr);
   */

   return 0; /* Not found */
}
