/*_
 * Copyright (c) 2014-2015 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Modified for Linux-XIA, 2015
 * Garnaik Sumeet, Michel Machado 
 */

#ifndef _POPTRIE_XIA_H
#define _POPTRIE_XIA_H

#include <stdint.h>
#include "../Data-Generation/generate_fibs.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef struct XID {unsigned char w[HEXXID];} XID;


#define POPTRIE_S 18
#define POPTRIE_INIT_FIB_SIZE 4096

#define popcnt(v)	__builtin_popcountll(v)


/* Internal node */
typedef struct poptrie_node {
	u64 leafvec;
	u64 vector;
	u32 base0;
	u32 base1;
} poptrie_node_t;

/* Leaf node */
typedef u16 poptrie_leaf_t;

/*
 * Radix tree node
 */
struct radix_node {
	int valid;
	struct radix_node *left;
	struct radix_node *right;

	/* Next hop */
	XID prefix;
	int len;
	poptrie_leaf_t nexthop;

	/* Propagated route */
	struct radix_node *ext;

	/* Mark for update */
	int mark;
};

/*
 * FIB mapping table
 */
struct poptrie_fib {
	void **entries;
	int n;
	int sz;
};

/* Poptrie management data structure */
struct poptrie {
	/* Root */
	u32 root;

	/* FIB */
	struct poptrie_fib fib;

	/* Memory management */
	poptrie_node_t *nodes;
	poptrie_leaf_t *leaves;
	void *cnodes;
	void *cleaves;

	/* Size */
	int nodesz;
	int leafsz;

	/* Direct pointing */
	u32 *dir;
	u32 *altdir;

	/* RIB */
	struct radix_node *radix;

	/* Control */
	int _allocated;
};

/* in poptrie_xia.c */
struct poptrie * poptrie_init(struct poptrie *, int, int);
void poptrie_release(struct poptrie *);
void * poptrie_lookup(struct poptrie *, XID);
int poptrie_route_add(struct poptrie *, XID, int, void *);
void * poptrie_rib_lookup(struct poptrie *, XID);

/* in xid_operations.c*/
XID extractXID(int pos, int length, XID data);
uint32_t XIDtounsigned(XID *bitpat);

#endif
