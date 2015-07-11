/*
   lc_trie.h

   A routing table for wordsized (32bits) bitstrings implemented as a
   static level- and pathcompressed trie. For details please consult

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
#ifndef _LC_TRIE_H_
#define _LC_TRIE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

/*
   The trie is represented by an array and each node consists of an
   unsigned word. The first 5 bits (31-27) indicate the logarithm
   of the branching factor. The next 5 bits (26-22) indicate the
   skip value. The final 22 (21-0) bits is an adress, either to
   another internal node, or the base vector.
   The maximum capacity is 2^21 strings (or a few more). The trie
   is prefixfree. All strings that are prefixes of another string
   are stored separately.
*/

#define ADRSIZE 160       /* the number of bits in an address */

							// XIDs will be 160 bits

/* A 32-bit word is used to hold the bit patterns of
   the addresses. In IPv6 this should be 128 bits.
   The following typedef is machine dependent.
   A word must be 32 bits long! */

typedef unsigned int word;

typedef struct xid { unsigned char w[20]; } xid;
typedef xid *list;

typedef uint64_t word;
typedef word node_t;

#define NOPRE -1          /* an empty prefix pointer */
#define NOBASE -1

#define SETBRANCH(branch)   ((branch)<<27)
#define GETBRANCH(node)     ((node)>>27)
#define SETSKIP(skip)       ((skip)<<20)
#define GETSKIP(node)       ((node)>>22 & 037)
#define SETADR(adr)         (adr)
#define GETADR(node)        ((node) & 017777777)

/* extract n bits from str starting at position p */
#define EXTRACT(p, n, str) ((str)<<(p)>>(32-(n)))

/* remove the first p bits from string */
#define REMOVE(p, str)   ((str)<<(p)>>(p))

/* A next-hop table entry is a 160 bit string */

typedef xid nexthop_t;

/* The routing table entries are initially stored in
   a simple array */

typedef struct entryrec *entry_t;
struct entryrec {
   xid data;          /* the routing entry */
   int len;            /* and its length */
   nexthop_t nexthop;  /* the corresponding next-hop */
   int pre;            /* this auxiliary variable is used in the */
};                    /* construction of the final data structure */

/* base vector */

typedef struct baserec *base_t;
struct baserec {
   xid str;    /* the routing entry */
   int len;     /* and its length */
   int pre;     /* pointer to prefix table, -1 if no prefix */
   int nexthop; /* pointer to next-hop table */
};

typedef struct { /* compact version of above */
   xid str;
   int len;
   int pre;
   int nexthop;
} comp_base_t;

/* prefix vector */

typedef struct prerec *pre_t;
struct prerec {
   int len;     /* the length of the prefix */
   int pre;     /* pointer to prefix, -1 if no prefix */
   int nexthop; /* pointer to nexthop table */
};

typedef struct { /* compact version of above */
   int len;
   int pre;
   int nexthop;
} comp_pre_t; 

/* The complete routing table data structure consists of
   a trie, a base vector, a prefix vector, and a next-hop table. */

typedef struct routtablerec *routtable_t;
struct routtablerec {
   node_t *trie;         /* the main trie search structure */
   int triesize;
   comp_base_t *base;    /* the base vector */
   int basesize;
   comp_pre_t *pre;      /* the prefix vector */
   int presize;
   nexthop_t *nexthop;   /* the next-hop table */
   int nexthopsize;
};

typedef struct node_patric node_patric;
struct node_patric
{
	int skip;
	node_patric *left;
	node_patric *right;
	int base;
};
typedef struct node_lc node_lc;
struct node_lc
{
	int skip;
	node_lc **subtrie;
	int base;
	int branch;
	int addr;
	int child;
};

/* Extract operation for xids*/
xid extract(int pos, int length, xid data);

/* Remove bits from xids*/
xid removexid(int bits, xid data);

/* Increment xids*/
int incrementxid(xid *pxid);

/* Bitwise shifting operations on xids*/
xid shift_left(xid id, int shift);
xid shift_right(xid id, int shift);

/* Build the routing table */
routtable_t buildrouttable(entry_t entry[], int nentries);

/* Dispose of the routing table */
void disposerouttable(routtable_t t);

/* Perform a lookup. */

// Need to find the exact details of word usage here
nexthop_t find(word s, routtable_t t);


/* utilities */
typedef int boolean;
#define TRUE 1
#define FALSE 0

#endif // _LC_TRIE_H_
