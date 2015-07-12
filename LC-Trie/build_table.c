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
#include "xidsort.h"
#include "lc_trie.h"

/* Compare two routing table entries. This is used by qsort */
int compareentries(const void *id1, const void *id2)
{
	int tmpresult;
	entry_t *tmp_entry1 = (entry_t *) id1;
	entry_t *tmp_entry2 = (entry_t *) id2;
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

static int skipcompute(base_t base[], int prefix, int first, int n)
{
	int i,match;
	xid low,high,*plow,*phigh;

	// Compute the skip value for that node
	low = removexid(prefix, base[first]->str);
	high = removexid(prefix, base[first+n-1]->str);
	i = prefix;
	plow = malloc(sizeof(xid));
	phigh = malloc(sizeof(xid));
	*plow = extract(i, 1, low);
	*phigh = extract(i, 1, high);
	match = comparexid(&plow, &phigh);
	while (0 == match)
	{
		i++;
		*plow = extract(i, 1, low);
		*phigh = extract(i, 1, high);
		match = comparexid(&plow, &phigh);
	}
	return i;
}

static int subtriecompute(base_t base[], int prefix, int first, int n)
{
	xid tmp;
	int nleft = 0;
	unsigned char left = (unsigned char) 0;

	// Find the number of prefixes in base that belong to left subtrie
	tmp = extract(prefix, 1, base[first]->str);
	while (left == tmp.w[19])
	{
		nleft++;
		tmp = extract(prefix, 1, base[first+nleft]->str);
	}
	return nleft;
}

static node_patric *buildpatricia(base_t base[], int prefix, int first, int n)
{
	int i,newprefix,nleft;
	node_patric *node;

	// Reached the leaf node
	if (1 == n)
	{
		node = malloc(sizeof(node_patric));
		node->left = NULL;
		node->right = NULL;
		node->skip = base[first]->len - prefix;
		node->base = first;
		return node;
	}
	newprefix = skipcompute(base, prefix, first, n);
	node = malloc(sizeof(node_patric));
	node->skip = newprefix - prefix;
	node->base = NOBASE;
	nleft = subtriecompute(base, newprefix, first, n);
	node->left = buildpatricia(base, newprefix+1, first, nleft);
	node->right = buildpatricia(base, newprefix+1, first+nleft, n-nleft);
	return node;
}

static int branchcompute(node_lc *node)
{
	int b1,b2;
	if (NULL == node->subtrie)
	{
		node->branch = 0;
		return node->branch;
	}
	b1 = branchcompute(node->subtrie[0]);
	b2 = branchcompute(node->subtrie[1]);
	if ((b1 == b2) || (b1 > b2))
		node->branch = 1 + b2;
	else
		node->branch = 1 + b1;
	return node->branch;
}

static int countlevels(node_lc *node, int *level, int clevel)
{
	unsigned long tmp,i;
	level[clevel]++;

	if (NULL == node->subtrie)
		return 0;
	tmp = 1<<(node->branch);
	for (i=0;i<tmp;i++)
		countlevels(node->subtrie[i], level, clevel + 1);
}

static int addrarray(node_lc *node, int *level, int *fill, int clevel)
{
	int tmp,i,sum = 0;
	node_t k,l;

	for (i=0;i<clevel;i++)
		sum += level[i];
	tmp = sum + fill[clevel];
	fill[clevel]++;
	node->addr = tmp;
	if (0 == node->branch)
	{
		node->child = node->base;
		return 0;
	}
	l = 1<<(node->branch);
	for (k=0;k<l;k++)
		addrarray(node->subtrie[k], level, fill, clevel+1);
	node->child = (node->subtrie[0])->addr;
}

static int fillarray(node_t *table, node_lc *node)
{
	node_t k,l,tmp = 0;

	tmp = ((node_t) node->branch)<<40 | ((node_t) node->skip)<<32 | (node_t) node->child;
	table[node->addr] = tmp;
	if (0 == node->branch)
		return 0;
	l = 1<<(node->branch);
	for (k=0;k<l;k++)
		fillarray(table, node->subtrie[k]);
}

static int lltoarray(node_lc *root, node_t *table, int *n)
{
	int level[ADRSIZE] = {0};
	int i, sum = 0;

	countlevels(root, level, 0);
	for (i=0;i<ADRSIZE;i++)
	{
		if (0 == level[i])
			break;
		sum += level[i];
	}
	int fill[i];
	memset(fill, 0, sizeof(int)*i);
	*n = sum;
	addrarray(root, level, fill, 0);
	fillarray(table, root);
	return 0;
}

static node_lc **compressedaddr(node_lc *node, int level)
{
	unsigned long tmp_branch,i;
	node_lc **tmp;
	if (1 == level)
	{
		tmp = node->subtrie;
		free(node);
		return tmp;
	}
	node_lc **left, **right;
	left = compressedaddr(node->subtrie[0], level - 1);
	right = compressedaddr(node->subtrie[1], level - 1);
	free(node->subtrie);
	tmp = malloc((1<<level) * sizeof(node_lc *));
	tmp_branch = 1<<(level - 1);
	for (i=0;i<tmp_branch;i++)
	{
		tmp[i] = left[i];
		tmp[i+tmp_branch] = right[i];
	}
	free(node);
	return tmp;
}

static int compresspattolc(node_lc *node)
{
	unsigned long i,tmp;

	if (0 == node->branch)
		return 0;
	else if (1 == node->branch)
	{
		compresspattolc(node->subtrie[0]);
		compresspattolc(node->subtrie[1]);
		return 0;
	}
	node_lc **left, **right;
	left = compressedaddr(node->subtrie[0], node->branch - 1);
	right = compressedaddr(node->subtrie[1], node->branch - 1);
	free(node->subtrie);
	node->subtrie = malloc((1<<node->branch) * sizeof(node_lc *));
	tmp = 1<<(node->branch - 1);
	for (i=0;i<tmp;i++)
	{
		node->subtrie[i] = left[i];
		node->subtrie[i+tmp] = right[i];
	}
	for (i=0;i<(tmp<<1);i++)
		compresspattolc(node->subtrie[i]);
}

static node_lc *pattonewdata(node_patric *node)
{
	node_lc *tmp = malloc(sizeof(node_lc));
	if (NULL == node->left)
	{
		tmp->subtrie = NULL;
		tmp->skip = node->skip;
		tmp->base = node->base;
		tmp->branch = -1;
		tmp->addr = -1;
		tmp->child = -1;
		free(node);
		return tmp;
	}
	tmp->subtrie = malloc(2 * sizeof(node_lc *));
	tmp->subtrie[0] = pattonewdata(node->left);
	tmp->subtrie[1] = pattonewdata(node->right);
	tmp->skip = node->skip;
	tmp->base = node->base;
	tmp->branch = -1;
	tmp->addr = -1;
	tmp->child = -1;
	free(node);
	return tmp;
}

static int pattolc(node_patric *root, node_t *table, int *n)
{
	node_lc *newroot;
	newroot = pattonewdata(root);
	branchcompute(newroot);
	compresspattolc(newroot);
	lltoarray(newroot, table, n);
}

int binsearch(xid data, xid *table, int n)
{
	int low, mid, high, val;
	xid *pdata = &data;
	xid *pmid;

	low = 0;
	high = n-1;
	while (low <= high)
	{
		mid = (low + high)/2;
		pmid = &table[mid];
		val = comparexid(&pdata, &pmid);
		if (-1 == val)
			high = mid - 1;
		else if (1 == val)
			low = mid + 1;
		else
			return mid;
	}
	return -1;
}

/* Is the string s a prefix of the string t? */

int isprefix(entry_t s, entry_t t)
{
	int tmp = 0;
	int equal = -1;
	int length;
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

static nexthop_t *buildnexthoptable(entry_t entry[], int nentries, int *nexthopsize)
{
	nexthop_t *nexthop, *nexttemp;
	nexthop_t *pnexttmp[nentries];
	int count, i;

	/* Extract the nexthop addresses from the entry array */
	nexttemp = (nexthop_t *) malloc(nentries * sizeof(nexthop_t));
	for (i=0;i<nentries;i++)
	{
		nexttemp[i] = entry[i]->nexthop;
		pnexttmp[i] = &nexttemp[i];
	}

	xidsort(pnexttmp, nentries, sizeof(nexthop_t *), comparexid);

	/* Remove duplicates */
	count = nentries > 0 ? 1 : 0;
	for (i = 1; i < nentries; i++)
	{
		if (comparexid(&pnexttmp[i-1], &pnexttmp[i]) != 0)
			pnexttmp[count++] = pnexttmp[i];
	}

    /* Move the elements to an array of proper size */
	nexthop = (nexthop_t *) malloc(count * sizeof(nexthop_t));
	for (i = 0; i < count; i++)
		nexthop[i] = *pnexttmp[i];

	free(nexttemp);

	*nexthopsize = count;
	return nexthop;
}

routtable_t buildrouttable(entry_t entry[], int nentries)
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
	
	// Start timing measurements
	nexthop = buildnexthoptable(entry, nentries, &nnexthops);
	// End timing measurements

	// Start timing measurements
	xidentrysort(entry, nentries, sizeof(entry_t), compareentries);
	/* Remove duplicates */
	size = nentries > 0 ? 1 : 0;
	for (i = 1; i < nentries; i++)
	{
		if (compareentries(&entry[i-1], &entry[i]) != 0)
			entry[size++] = entry[i];
	}
	// End timing measurements

	// Start timing measurements
	/* The number of internal nodes in the tree can't be larger
	than the number of entries. */
	b = (base_t *) malloc(size * sizeof(base_t));
	p = (pre_t *) malloc(size * sizeof(pre_t));

	/* Initialize pre-pointers */
	for (i = 0; i < size; i++)
		entry[i]->pre = NOPRE;

	/* Go through the entries and put the prefixes in p
	and the rest of the strings in b */
	for (i = 0; i < size; i++)
	{
		if (i < size-1 && isprefix(entry[i], entry[i+1]))
		{
			ptemp = (pre_t) malloc(sizeof(struct prerec));
			ptemp->len = entry[i]->len;
			ptemp->pre = entry[i]->pre;
			/* Update 'pre' for all entries that have this prefix */
			for (j = i + 1; j < size && isprefix(entry[i], entry[j]); j++)
				entry[j]->pre = nprefs;
			ptemp->nexthop = binsearch(entry[i]->nexthop, nexthop, nnexthops);
			p[nprefs++] = ptemp;
		}
		else
		{
			btemp = (base_t) malloc(sizeof(struct baserec));
			btemp->len = entry[i]->len;
			btemp->str = entry[i]->data;
			btemp->pre = entry[i]->pre;
			btemp->nexthop = binsearch(entry[i]->nexthop, nexthop, nnexthops);
			b[nbases++] = btemp;
		}
	}

	node_patric *root;
	root = buildpatricia(b, 0, 0, nbases);
	node_t *lctable = malloc(sizeof(node_t) * (2*nbases - 1));
	int nnode_lc;
	pattolc(root, lctable, &nnode_lc);

	/* At this point we now how much memory to allocate */
	trie = malloc(nnode_lc * sizeof(node_t));
	base = malloc(nbases * sizeof(comp_base_t));
	pre = malloc(nprefs * sizeof(comp_pre_t));
	
	for (i = 0; i < nnode_lc; i++)
		trie[i] = lctable[i];
	free(lctable);
	
	for (i = 0; i < nbases; i++)
	{
		base[i].str = b[i]->str;
		base[i].len = b[i]->len;
		base[i].pre = b[i]->pre;
		base[i].nexthop = b[i]->nexthop;
		free(b[i]);
	}
	free(b);
	
	for (i = 0; i < nprefs; i++)
	{
		pre[i].len = p[i]->len;
		pre[i].pre = p[i]->pre;
		pre[i].nexthop = p[i]->nexthop;
		free(p[i]);
	}
	free(p);
	
	table = (routtable_t) malloc(sizeof(struct routtablerec));
	table->trie = trie;
	table->triesize = nnode_lc;
	table->base = base;
	table->basesize = nbases;
	table->pre = pre;
	table->presize = nprefs;
	table->nexthop = nexthop;
	table->nexthopsize = nnexthops;
	// End timing measurements
	return table;
}
