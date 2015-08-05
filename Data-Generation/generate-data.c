/* This file is concerned with the generation of synthetic data for evaluation
 * of LPM algorithms in Linux-XIA. By synthetic data, we refer to the FIB for
 * LPM on XIDs.
 *
 * This is a free software and licensed under GNU General Public License Version
 * 2.
 *
 * Garnaik Sumeet, Michel Machado 2015
 */

#include "generate_fibs.h"

/*
 * Print the table
 */
int print_table(struct nextcreate **pretable, int size, int prexp)
{
	unsigned int i, j;
	unsigned char tmp_prefix[HEXXID + 1] = {0};
	char file_name[MAXFILENAME];
	FILE *tmp_file;

	memset(file_name, 0, MAXFILENAME);
	sprintf(file_name, "%d.table", prexp);
	tmp_file = fopen(file_name, "w+");

	for (i = 0; i < size; i++) {
		memcpy(tmp_prefix, pretable[i]->prefix, HEXXID);
		for (j = 0; j < HEXXID; j++)
			fprintf(tmp_file, "%02x", pretable[i]->prefix[j]);
		fprintf(tmp_file, " %d %d\n", pretable[i]->len,
				pretable[i]->nexthop);
	}
	fclose(tmp_file);
}

/*
 * Comparison function to be passed to qsort for sorting of entries
 */
static int sortentries(const void *e1, const void *e2)
{
	int res;
	struct nextcreate **entry1 = (struct nextcreate **) e1;
	struct nextcreate **entry2 = (struct nextcreate **) e2;
	unsigned char *pre1 = (*entry1)->prefix;
	unsigned char *pre2 = (*entry2)->prefix;

	res = memcmp(pre1, pre2, 20);

	if (res < 0)
		return -1;
	else if (res > 0)
		return 1;
	else
		return 0;
}

/*
 * This routine generates an unsigned char containing `8 - bit` random bits using
 * the random variable `r` from MSB to LSB.
 *
 * Inputs:
 * r: Random variable used by GSL
 * bit: Number of random bits = 8 - bit
 */
static unsigned char uniform_char(gsl_rng *r, int bit)
{
	int i;
	unsigned char tmp_char = 0;
	unsigned char res = 0;
	unsigned long int tmp_bit;

	for (i = (BYTE - 1); i >= bit; i--) {
		tmp_bit = gsl_rng_uniform_int(r, BITRAND);
		// In order to be endian agnostic
		if (1 == tmp_bit) {
			tmp_char = 1;
			tmp_char = tmp_char << i;
		} else {
			tmp_char = 0;
		}
		res = res | tmp_char;
	}

	return res;
}

/*
 * This routine is concerned with the replacement of duplicates in the FIBs
 * generated such that all the entries in the FIB are distinct.
 *
 * Inputs:
 * dup_table: array consisting of indexes of duplicate structs
 * seed: seed to be used to generate the replacements of duplicates
 * sortedtable: array consisting of sorted entries
 * size: number of duplicates
 * sortsize: number of entries in sortedtable
 */
static int remove_duplicates(unsigned int *dup_table, int seed,
		struct nextcreate **sortedtable, int size, int sortsize)
{
	int i, j;
	int bytes, bits;
	int flag;
	unsigned long int tmp;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seed);

	for (i = 0; i < size; i++) {
		flag = FALSE;
		while (flag) {
			memset(sortedtable[dup_table[i]]->prefix, 0, HEXXID);
			bytes = sortedtable[dup_table[i]]->len / BYTE;
			bits = sortedtable[dup_table[i]]->len % BYTE;
			for (j = 0; j < bytes; j++)
				sortedtable[dup_table[i]]->prefix[j] =
					uniform_char(r, 0);
			sortedtable[dup_table[i]]->prefix[j] =
				uniform_char(r, (BYTE - bits));
			if (NULL == bsearch(&sortedtable[dup_table[i]],
				&sortedtable[0], sortsize,
				sizeof(struct nextcreate *), sortentries))
				flag = TRUE;
		}
	}

	gsl_rng_free(r);

	return 0;
}

/* 
 * This routine is concerned with the generation of nexthops for the prefix
 * entries in the table. The total number of unique nexthops in each table is a
 * function of the size of the table. The number of unique nexthops is 2^(i/2)
 * where the size of the table is 2^i.
 *
 * Inputs 
 * pretable: An array consisting of pointers to the entry structs
 * size: number of entries in the table
 * seeds: array containing seeds
 * tablexp: log2(size)
 */
static int nexthop_dist(struct nextcreate **pretable, int size,
		uint32_t *seeds, int low, int tablexp)
{
	int i, j;
	struct nextcreate **tmp_table = NULL;
	unsigned int *dup_table = NULL;
	unsigned int maxnexthops;
	unsigned long int tmp;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seeds[low]);

	tmp_table = malloc(size * sizeof(struct nextcreate *));
	for (i = 0; i < size; i++)
		tmp_table[i] = pretable[i];

	qsort(tmp_table, size, sizeof(struct nextcreate *), sortentries);
	dup_table = malloc(DUPS * sizeof(unsigned int));
	j = 0;
	for (i = 1; i < size; i++) {
		if ((0 == sortentries(&tmp_table[i - 1], &tmp_table[i])) &&
				(tmp_table[i-1]->len == tmp_table[i]->len)) {
			dup_table[j] = i;
			assert(j++ < DUPS);
		}
	}
	remove_duplicates(dup_table, seeds[low + 1], tmp_table, j, size);
	maxnexthops = 1 << (tablexp / 2);
	for (i = 0; i < size; i++) {
		tmp = gsl_rng_uniform_int(r, maxnexthops) + 1;
		tmp_table[i]->nexthop = (unsigned int) tmp;
	}

	gsl_rng_free(r);

	free(tmp_table);
	free(dup_table);

	return 0;
}

/* 
 * This routine is concerned with the generation of uniformly distributed
 * prefixes of certain prefix length.
 *
 * Inputs:
 * pretable: An array consisting of pointers to the entry structs
 * size: number of entries in the table
 * seed: seed
 * tablexp: log2(size)
 */
static int prefix_dist(struct nextcreate **pretable, int size, 
		uint32_t seed, int tablexp)
{
	int i, j, k;
	int bytes, bits;
	unsigned char tmp_prefix[HEXXID] = {0};
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seed);

	for (i = 0; i < size; i++) {
		memset(pretable[i]->prefix, 0, HEXXID);
		bytes = pretable[i]->len / BYTE;
		bits = pretable[i]->len % BYTE;
		for (j = 0; j < bytes; j++)
			pretable[i]->prefix[j] = uniform_char(r, 0);
		pretable[i]->prefix[j] = uniform_char(r, (BYTE - bits));
	}

	gsl_rng_free(r);

	return 0;
}

/* 
 * This routine is concerned with the generation of uniformly distributed prefix
 * lengths. We are aware of this strong assumption and will soon replace this
 * with a more suitable distribution.
 *
 * pretable: An array consisting of pointers to the entry structs
 * size: number of entries in the table
 * seed: seed
 * tablexp: log2(size)
 */
static int prelength_dist(struct nextcreate **pretable, int size,
		uint32_t seed, int tablexp)
{
	int i;
	gsl_rng *r;
	unsigned long int tmp;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seed);

	for (i = 0; i < size; i++) {
		tmp = gsl_rng_uniform_int(r, MAXRAND) + OFFSET;
		pretable[i]->len = (unsigned int) tmp;
	}

	gsl_rng_free(r);

	return 0;
}

/* 
 * This is the primary function that facilitates creation of multiple FIBs of
 * table sizes from the range [2^LEXPFIB, 2^HEXPFIB] where the table size grows
 * exponentially.
 *
 * Inputs:
 * tablexp: log2(length of pretable i.e number of entries)
 * seeds: array containing seeds
 * low: lower index in the array for using seeds
 * pretable: array of pointers to nextcreate structs i.e pointers to entries
 */
int table_dist(int tablexp, uint32_t *seeds, int low,
		struct nextcreate **pretable)
{
	int i;
	int size;
	struct nextcreate *table= NULL;

	size = 1 << tablexp;
	table = malloc(size * sizeof(struct nextcreate));
	for (i = 0; i < size; i++)
		pretable[i] = (table + i);
	prelength_dist(pretable, size, seeds[low], tablexp);
	prefix_dist(pretable, size, seeds[low + 1], tablexp);
	nexthop_dist(pretable, size, seeds, low + 2, tablexp);

	return 0;
}
