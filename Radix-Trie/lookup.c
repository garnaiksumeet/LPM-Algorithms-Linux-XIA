/*
 * Garnaik Sumeet, Michel Machado 2015
 * LPM Algorithms for Linux-XIA
 */
#include "radix.h"
#include <inttypes.h>

unsigned int find(xid s, struct routtablerec *t)
{
	struct node_patric *cur_node = t->root;
	assert(cur_node);
	xid bitmask;
	int pos = 0;

	pos = cur_node->skip;
	while (NOBASE == cur_node->base) {
		bitmask = extract(pos, 1, s);
		pos++;
		if (bitmask.w[HEXXID - 1])
			cur_node = cur_node->right;
		else
			cur_node = cur_node->left;
		pos += cur_node->skip;
	}

	// Check if actually a hit
	int bidx = cur_node->base;
	xid tmp = extract(0, pos, t->base[bidx].str);
	bitmask = extract(0, pos, s);
	int neq = comparexid(&bitmask, &tmp);
	if (!neq)
		return t->base[bidx].nexthop;

	// Check in prefix tree
	int pidx = t->base[bidx].pre;
	while (NOPRE != pidx) {
		tmp = extract(0, t->pre[pidx].len, t->base[bidx].str);
		bitmask = extract(0, t->pre[pidx].len, s);
		neq = comparexid(&bitmask, &tmp);
		if (!neq)
			return t->pre[pidx].nexthop;
		else
			pidx = t->pre[pidx].pre;
	}

	// Not found
	return 0;
}
