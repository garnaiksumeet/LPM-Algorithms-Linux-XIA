/*
 * Garnaik Sumeet, Michel Machado 2015
 * LPM Algorithm for Linux-XIA
 */
#include <stddef.h>
#include "xidsort.h"
#include "radix.h"

/*
 * Compare two routing table entries.
 */
int compareentries(const void *id1, const void *id2)
{
	int tmpresult;
	struct entryrec **tmp_entry1 = (struct entryrec **) id1;
	struct entryrec **tmp_entry2 = (struct entryrec **) id2;
	xid *tmp_id1 = &((*tmp_entry1)->data);
	xid *tmp_id2 = &((*tmp_entry2)->data);

	tmpresult = comparexid(&tmp_id1, &tmp_id2);

	if (tmpresult < 0)
		return -1;
	else if (tmpresult > 0)
		return 1;
	else
		return 0;
}
/*
static int count_pat_nodes(struct node_patric *n)
{
	if (NULL == n->left) {
		return 1;
	} else {
		int i = count_pat_nodes(n->left) + count_pat_nodes(n->right) + 1;
		return i;
	}
}
*/
/*
 * Compute the number of bits to be skipped for a range of XID addresses.
 *
 * base: base vector
 * prefix: prefix common to the XIDs starting at position first and having
 * total n such XIDs
 */
static int skipcompute(struct baserec *base[], int prefix, int first, int n)
{
	int i, match;
	xid low, high, *plow, *phigh;

	// Compute the skip value for that node
	low = removexid(prefix, base[first]->str);
	high = removexid(prefix, base[first + n - 1]->str);
	i = prefix;
	plow = malloc(sizeof(xid));
	phigh = malloc(sizeof(xid));
	*plow = extract(i, 1, low);
	*phigh = extract(i, 1, high);
	match = comparexid(&plow, &phigh);
	while (0 == match) {
		i++;
		*plow = extract(i, 1, low);
		*phigh = extract(i, 1, high);
		match = comparexid(&plow, &phigh);
	}
	free(plow);
	free(phigh);
	return i;
}

/*
 * Computes the total number of base XIDs for a given prefix
 * 
 * base: base vector
 * prefix: number of bits that are common to the callee node
 * first: starting address of the base for callee node
 * n: number of addresses of the base for the callee node
 */
static int subtriecompute(struct baserec *base[], int prefix, int first, int n)
{
	xid tmp;
	int nleft = 0;
	unsigned char left = (unsigned char) 0;

	// Find the number of prefixes in base that belong to left subtrie
	tmp = extract(prefix, 1, base[first]->str);
	while (left == tmp.w[HEXXID - 1]) {
		nleft++;
		tmp = extract(prefix, 1, base[first + nleft]->str);
	}
	return nleft;
}

/*
 * This routine builds the Patricia trie recursively for all nodes in the range
 * of address "first" to "first + n -1" in base vector: base passed as arg
 *
 * base: base vector
 * prefix: number of bits that are common to all the XIDs in the range
 * "first" to "first + n - 1"
 * first: first index of base vector
 * n: number of prefixes in base vector starting at index first 
 */
static struct node_patric *buildpatricia(struct baserec *base[], int prefix,
		int first, int n)
{
	int i, newprefix, nleft;
	struct node_patric *node;

	// Reached the leaf node
	if (1 == n) {
		node = malloc(sizeof(struct node_patric));
		node->left = NULL;
		node->right = NULL;
		node->skip = base[first]->len - prefix;
		node->base = first;
		return node;
	}

	//Find the number of bits to be skipped for the new node being constructed
	//and range of XIDs for each of its child
	newprefix = skipcompute(base, prefix, first, n);
	node = malloc(sizeof(struct node_patric));
	node->skip = newprefix - prefix;
	node->base = NOBASE;
	nleft = subtriecompute(base, newprefix, first, n);
	node->left = buildpatricia(base, newprefix + 1, first, nleft);
	node->right = buildpatricia(base, newprefix + 1, first + nleft, n - nleft);
	return node;
}

/*
 * The input of this routine is two pointers to entry record structures and
 * the output is an integer that essentially answers the question:
 * Is the routing entry in "s" a prefix of the routing entry in "t"?
 */
int isprefix(struct entryrec *s, struct entryrec *t)
{
	int tmp = 0;
	int equal = -1;
	int length;	//Represents the length of the prefix in the routing entry "s"
	xid s_data, t_data;
	xid *ps_data = &s_data;
	xid *pt_data = &t_data;

	if (NULL == s)
		return tmp;

	length = s->len;
	s_data = extract(0, length, s->data);
	t_data = extract(0, length, t->data);
	equal = comparexid(&ps_data, &pt_data);
	tmp = ((0 == length) || ((length <= t->len) && (0 == equal)));
	return tmp;
}

int compare(const void * a, const void * b)
{
	return ( *(unsigned int*)a - *(unsigned int*)b );
}

/*
 * This routine builds the nexthop table that contains nexthop XIDs in a sorted
 * fashion. The duplicate addresses are removed and this table returns a
 * pointer to the first address of an array containing the nexthop xids.
 *
 * entry: array of entryrec pointers
 * nentries: number of total entries
 * nexthopsize: contains size of nexthop table
 */
static unsigned int *buildnexthoptable(struct entryrec *entry[], int nentries, int *nexthopsize)
{
	unsigned int *nexthop = NULL;
	unsigned int *tmpnext = malloc(nentries * sizeof(unsigned int));
	int count, i;

	//Extract the nexthop addresses from the entry array
	for (i = 0; i < nentries; i++)
		tmpnext[i] = entry[i]->nexthop;

	qsort(tmpnext, nentries, sizeof(unsigned int), compare);

	//Remove duplicates
	count = nentries > 0 ? 1 : 0;
	for (i = 1; i < nentries; i++) {
		if (tmpnext[i - 1] != tmpnext[i])
			tmpnext[count++] = tmpnext[i];
	}

	//Move the elements to an array of proper size
	nexthop = malloc(count * sizeof(unsigned int));
	for (i = 0; i < count; i++)
		nexthop[i] = tmpnext[i];

	free(tmpnext);

	*nexthopsize = count;
	return nexthop;
}

/*
 * This routine builds the entire routing table
 */
struct routtablerec *buildrouttable(struct entryrec *entry[], int nentries)
{
	unsigned int *nexthop = NULL; // Nexthop table 
	int nnexthops;
	
	struct baserec **b = NULL;
	struct baserec *btemp = NULL;
	struct prerec **p = NULL;
	struct prerec *ptemp = NULL;

	node_t *trie = NULL;
	struct baserec *base = NULL;
	struct prerec *pre = NULL;
	
	struct routtablerec *table = NULL;  // The complete data structure
	
	/* Auxiliary variables */
	int i, j, nprefs = 0, nbases = 0, *tmp_val = NULL;

	nexthop = buildnexthoptable(entry, nentries, &nnexthops);

	xidentrysort(entry, nentries, sizeof(struct entryrec *), compareentries);
	// Remove duplicates
/*	size = nentries > 0 ? 1 : 0;
	for (i = 1; i < nentries; i++) {
		if (compareentries(&entry[i - 1], &entry[i]) != 0)
			entry[size++] = entry[i];
	}*/

	// The number of internal nodes in the tree can't be larger
	// than the number of entries.
	b = (struct baserec **) malloc(nentries * sizeof(struct baserec *));
	p = (struct prerec **) malloc(nentries * sizeof(struct prerec *));

	//Initialize pre-pointers
	for (i = 0; i < nentries; i++)
		entry[i]->pre = NOPRE;

	//Go through the entries and put the prefixes in p
	//and the rest of the strings in b
	for (i = 0; i < nentries; i++) {
		if (i < nentries - 1 && isprefix(entry[i], entry[i + 1])) {
			ptemp = malloc(sizeof(struct prerec));
			ptemp->len = entry[i]->len;
			ptemp->pre = entry[i]->pre;
			//Update 'pre' for all entries that have this prefix
			for (j = i + 1; j < nentries && isprefix(entry[i], entry[j]); j++)
					entry[j]->pre = nprefs;
			tmp_val = bsearch(&entry[i]->nexthop, nexthop,
				nnexthops, sizeof(unsigned int), compare);
			ptemp->nexthop = *tmp_val;
			p[nprefs++] = ptemp;
		} else {
			btemp = malloc(sizeof(struct baserec));
			btemp->len = entry[i]->len;
			btemp->str = entry[i]->data;
			btemp->pre = entry[i]->pre;
			tmp_val = bsearch(&entry[i]->nexthop, nexthop,
				nnexthops, sizeof(unsigned int), compare);
			btemp->nexthop = *tmp_val;
			b[nbases++] = btemp;
		}
	}
	struct node_patric *root = NULL;
	root = buildpatricia(b, 0, 0, nbases);
	int nnodes = count_pat_nodes(root);

	//At this point we now how much memory to allocate
	trie = malloc(nnodes * sizeof(node_t));
	base = malloc(nbases * sizeof(struct baserec));
	pre = malloc(nprefs * sizeof(struct prerec));

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

	table = (struct routtablerec *) malloc(sizeof(struct routtablerec));
	table->root = root;
	table->base = base;
	table->basesize = nbases;
	table->pre = pre;
	table->presize = nprefs;
	table->nexthop = nexthop;
	table->nexthopsize = nnexthops;
	return table;
}
