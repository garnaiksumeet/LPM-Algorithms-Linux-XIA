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
#include <stddef.h>
#include "qsort.h"
#include "lc_trie.h"

static int ROOTBRANCH = 16;     // The branching factor at the root
static double FILLFACT = 0.50;  // The trie fill factor


/* Compare two routing table entries. This is used by qsort */
int pstrcmp(entry_t *i, entry_t *j)
{
   if ((*i)->data < (*j)->data)
      return -1;
   else if ((*i)->data > (*j)->data)
      return 1;
   else if ((*i)->len < (*j)->len)
      return -1;
   else if ((*i)->len > (*j)->len)
      return 1;
   else
      return 0;
}

/* Compare two netxhop addresses. This is used by qsort */
int pnexthopcmp(nexthop_t *i, nexthop_t *j)
{
   if (*i < *j)
      return -1;
   else if (*i > *j)
      return 1;
   else
      return 0;
}

/*
   Compute the branch and skip value for the root of the
   tree that covers the base array from position 'first' to
   'first + n - 1'. Disregard the first 'prefix' characters.
   We assume that n >= 2 and base[first] != base[first+n-1].

*/
void computebranch(base_t base[], int prefix, int first, int n,
                   int *branch, int *newprefix)
{
   word low, high;
   int i, pat, b;
   boolean patfound;
   int count;

   /* Compute the new prefix */
   high = REMOVE(prefix, base[first]->str);
   low = REMOVE(prefix, base[first+n-1]->str);
   i = prefix;
   while (EXTRACT(i, 1, low) == EXTRACT(i, 1, high))
      i++;
   *newprefix = i;

   /* Always use branching factor 2 for two elements */
   if (n == 2) {
      *branch = 1;
      return;
   }

   /* Use a large branching factor at the root */
   if (ROOTBRANCH > 0 && prefix == 0  && first == 0) {
      *branch = ROOTBRANCH;
      return;
   }

   /* Compute the number of bits that can be used for branching.
      We have at least two branches. Therefore we start the search
      at 2^b = 4 branches. */
   b = 1;
   do {
      b++;
      if (n < FILLFACT*(1<<b) ||
          *newprefix + b > ADRSIZE)
         break;
      i = first;
      pat = 0;
      count = 0;
      while (pat < 1<<b) {
         patfound = FALSE;
         while (i < first + n &&   
                pat == EXTRACT(*newprefix, b, base[i]->str)) {
            i++;
            patfound = TRUE;
         }
         if (patfound)
            count++;
         pat++;
      }
   } while (count >= FILLFACT*(1<<b));
   *branch = b - 1;
}

/*
   Build a tree that covers the base array from position
   'first' to 'first + n - 1'. Disregard the first 'prefix'
   characters. 'pos' is the position for the root of this
   tree and 'nextfree' is the first position in the array
   that hasn't yet been reserved.
*/

void build(base_t base[], pre_t pre[], int prefix, int first, int n,
           int pos, int *nextfree, node_t *tree)
{
   int branch, newprefix;
   int k, p, adr, bits;
   word bitpat;

   if (n == 1)
      tree[pos] = first; /* branch and skip are 0 */
   else {
      computebranch(base, prefix, first, n, &branch, &newprefix);
      adr = *nextfree;
      tree[pos] = SETBRANCH(branch) |
                  SETSKIP(newprefix-prefix) |
                  SETADR(adr);
      *nextfree += 1<<branch;
      p = first;
      /* Build the subtrees */
      for (bitpat = 0; bitpat < 1<<branch; bitpat++) {
         k = 0;         
         while (p+k < first+n &&
                EXTRACT(newprefix, branch, base[p+k]->str) == bitpat)
            k++;

         if (k == 0) {
	   /* The leaf should have a pointer either to p-1 or p,
              whichever has the longest matching prefix */
            int match1 = 0, match2 = 0;

            /* Compute the longest prefix match for p - 1 */
            if (p > first) {
               int prep, len;
               prep =  base[p-1]->pre;
               while (prep != NOPRE && match1 == 0) {
                  len = pre[prep]->len;
                  if (len > newprefix &&
                      EXTRACT(newprefix, len - newprefix, base[p-1]->str) ==
                      EXTRACT(32 - branch, len - newprefix, bitpat))
                     match1 = len;
                  else
                     prep = pre[prep]->pre;
               }
	    }         

            /* Compute the longest prefix match for p */
            if (p < first + n) {
               int prep, len;
               prep =  base[p]->pre;
               while (prep != NOPRE && match2 == 0) {
                  len = pre[prep]->len;
                  if (len > newprefix &&
                      EXTRACT(newprefix, len - newprefix, base[p]->str) ==
                      EXTRACT(32 - branch, len - newprefix, bitpat))
                     match2 = len;
                  else
                     prep = pre[prep]->pre;
               }
	    }         

            if ((match1 > match2 && p > first) || p == first + n)
               build(base, pre, newprefix+branch, p-1, 1,
                     adr + bitpat, nextfree, tree);
            else
               build(base, pre, newprefix+branch, p, 1,
                     adr + bitpat, nextfree, tree);
         } else if (k == 1 && base[p]->len - newprefix < branch) {
            word i;
            bits = branch - base[p]->len + newprefix;
            for (i = bitpat; i < bitpat + (1<<bits); i++)
               build(base, pre, newprefix+branch, p, 1,
                     adr + i, nextfree, tree);
            bitpat += (1<<bits) - 1;
         } else
            build(base, pre, newprefix+branch, p, k,
                  adr + bitpat, nextfree, tree);
         p += k;
      }
   }
}

int binsearch(nexthop_t x, int v[], int n)
{
   int low, high, mid;

   low = 0;
   high = n - 1;
   while (low <= high) {
      mid = (low+high) / 2;
      if (x < v[mid])
         high = mid - 1;
      else if (x > v[mid])
         low = mid + 1;
      else
         return mid;
   }
   return -1;
}

/* Is the string s a prefix of the string t? */

int isprefix(entry_t s, entry_t t)
{
   return s != NULL &&
          (s->len == 0 ||   /* EXTRACT() can't handle 0 bits */
           s->len <= t->len &&
           EXTRACT(0, s->len, s->data) ==
           EXTRACT(0, s->len, t->data));
}

nexthop_t *buildnexthoptable(entry_t entry[], int nentries, int *nexthopsize)
{
   nexthop_t *nexthop, *nexttemp;
   int count, i;

   /* Extract the nexthop addresses from the entry array */
   nexttemp = (nexthop_t *) malloc(nentries * sizeof(nexthop_t));
   for (i = 0; i < nentries; i++)
      nexttemp[i] = entry[i]->nexthop;

   quicksort((char *) nexttemp, nentries,
             sizeof(nexthop_t), pnexthopcmp);

   /* Remove duplicates */
   count = nentries > 0 ? 1 : 0;
   for (i = 1; i < nentries; i++)
      if (pnexthopcmp(&nexttemp[i-1], &nexttemp[i]) != 0)
         nexttemp[count++] = nexttemp[i];

   /* Move the elements to an array of proper size */
   nexthop = (nexthop_t *) malloc(count * sizeof(nexthop_t));
   for (i = 0; i < count; i++) {
      nexthop[i] = nexttemp[i];
   }
   free(nexttemp);

   *nexthopsize = count;
   return nexthop;
}

routtable_t buildrouttable(entry_t entry[], int nentries,
                           double fillfact, int rootbranch,
                           int verbose)
{
   nexthop_t *nexthop; /* Nexthop table */
   int nnexthops;

   int size;           /* Size after dublicate removal */

   node_t *t;          /* We first build a big data structure... */
   base_t *b, btemp;
   pre_t *p, ptemp;

   node_t *trie;       /* ...and then we store it efficiently */
   comp_base_t *base;
   comp_pre_t *pre;

   routtable_t table;  /* The complete data structure */

   /* Auxiliary variables */
   int i, j, nprefs = 0, nbases = 0;
   int nextfree = 1;

   FILLFACT = fillfact;
   ROOTBRANCH = rootbranch;

   // Start timing measurements
   nexthop = buildnexthoptable(entry, nentries, &nnexthops);
   // End timing measurements
   /*if (verbose)
      fprintf(stderr, "\nBuilding nexthop table: %.2f\n", gettime());*/

   // Start timing measurements
   quicksort((char *) entry, nentries, sizeof(entry_t), pstrcmp);
   /* Remove duplicates */
   size = nentries > 0 ? 1 : 0;
   for (i = 1; i < nentries; i++)
      if (pstrcmp(&entry[i-1], &entry[i]) != 0)
         entry[size++] = entry[i];
   // End timing measurements
/*   if (verbose) {
      fprintf(stderr, "Sorting: %.2f", gettime());
      if (size != nentries)
         fprintf(stderr, "  (%i unique entries)", size);
      fprintf(stderr, "\n");
   }*/

   // Start timing measurements
   /* The number of internal nodes in the tree can't be larger
      than the number of entries. */
   t = (node_t *) malloc((2 * size + 2000000) * sizeof(node_t));
   b = (base_t *) malloc(size * sizeof(base_t));
   p = (pre_t *) malloc(size * sizeof(pre_t));

   /* Initialize pre-pointers */
   for (i = 0; i < size; i++)
      entry[i]->pre = NOPRE;

   /* Go through the entries and put the prefixes in p
      and the rest of the strings in b */
   for (i = 0; i < size; i++)
      if (i < size-1 && isprefix(entry[i], entry[i+1])) {
         ptemp = (pre_t) malloc(sizeof(struct prerec));
         ptemp->len = entry[i]->len;
         ptemp->pre =entry[i]->pre;
         /* Update 'pre' for all entries that have this prefix */
         for (j = i + 1; j < size && isprefix(entry[i], entry[j]); j++)
            entry[j]->pre = nprefs;
         ptemp->nexthop = binsearch(entry[i]->nexthop, nexthop, nnexthops);
         p[nprefs++] = ptemp;
      } else {
         btemp = (base_t) malloc(sizeof(struct baserec));
         btemp->len = entry[i]->len;
         btemp->str = entry[i]->data;
         btemp->pre = entry[i]->pre;
         btemp->nexthop = binsearch(entry[i]->nexthop, nexthop, nnexthops);
         b[nbases++] = btemp;
      }

   /* Build the trie structure */
   build(b, p, 0, 0, nbases, 0, &nextfree, t);

   /* At this point we now how much memory to allocate */
   trie = (node_t *) malloc(nextfree * sizeof(node_t));
   base = (comp_base_t *) malloc(nbases * sizeof(comp_base_t));
   pre = (comp_pre_t *) malloc(nprefs * sizeof(comp_pre_t));

   for (i = 0; i < nextfree; i++) {
      trie[i] = t[i];
   }
   free(t);

   for (i = 0; i < nbases; i++) {
      base[i].str = b[i]->str;
      base[i].len = b[i]->len;
      base[i].pre = b[i]->pre;
      base[i].nexthop = b[i]->nexthop;
      free(b[i]);
   }
   free(b);

   for (i = 0; i < nprefs; i++) {
      pre[i].len = p[i]->len;
      pre[i].pre = p[i]->pre;
      pre[i].nexthop = p[i]->nexthop;
      free(p[i]);
   }
   free(p);

   table = (routtable_t) malloc(sizeof(struct routtablerec));
   table->trie = trie;
   table->triesize = nextfree;
   table->base = base;
   table->basesize = nbases;
   table->pre = pre;
   table->presize = nprefs;
   table->nexthop = nexthop;
   table->nexthopsize = nnexthops;
   // End timing measurements
/*   if (verbose)
      fprintf(stderr, "Building routing table: %.2f\n", gettime());*/

   return table;
}

