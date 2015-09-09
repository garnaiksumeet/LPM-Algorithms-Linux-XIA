/*
 * Garnaik Sumeet, Michel Machado 2015
 * LPM Algorithms for Linux-XIA
 */
#include "radix.h"
#include <inttypes.h>

/* 
 * Return a nexthop or 0 if not found
 */
unsigned int find(xid s, struct routtablerec *t)
{
	struct node_patric *cur_node = t->root;
	assert(cur_node);
	xid *bitmask = malloc(sizeof(xid));
	int pos = 0;
	int i;

	pos = cur_node->skip;
	while (NOBASE == cur_node->base) {
		*bitmask = extract(pos, 1, s);
		pos++;
		if (bitmask->w[HEXXID - 1])
			cur_node = cur_node->right;
		else
			cur_node = cur_node->left;
		pos += cur_node->skip;
	}

	// Check if actually a hit
	int bidx = cur_node->base;
	memset(bitmask, 0, HEXXID);
	xid *tmp = malloc(sizeof(xid));
	for (i = 0; i < HEXXID; i++)
		tmp->w[i] = t->base[bidx].str.w[i] ^ s.w[i];
	int neq = comparexid(&bitmask, &tmp);
	if (!neq) {
		free(bitmask);
		free(tmp);
		return t->base[bidx].nexthop;
	}

	// Check in prefix tree
	int pidx = t->base[bidx].pre;
	int len;
	while (NOPRE != pidx) {
		len = t->pre[pidx].len;
		memset(tmp, 0, HEXXID);
		for (i = 0; i <= len / BYTE; i++)
			tmp->w[i] = t->base[bidx].str.w[i] ^ s.w[i];
		tmp->w[i] = tmp->w[i] >> (BYTE - len % BYTE) << (BYTE - len % BYTE);
		neq = comparexid(&bitmask, &tmp);
		if (!neq) {
			free(bitmask);
			free(tmp);
			return t->pre[pidx].nexthop;
		} else {
			pidx = t->pre[pidx].pre;
		}
	}

	// Not found
	free(bitmask);
	free(tmp);
	return 0;
}
