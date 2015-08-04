/* This file is concerned with the generation of synthetic data for evaluation
 * of LPM algorithms in Linux-XIA. By synthetic data, we refer to the FIB for
 * LPM on XIDs.
 *
 * This is a free software and licensed under GNU General Public License Version
 * 2.
 *
 * Garnaik Sumeet, Michel Machado 2015
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_sf_pow_int.h>
#include <gsl/gsl_randist.h>

#define LEXPFIB 4
#define HEXPFIB 20
#define MAXFILENAME 10
#define OFFSET 20
#define MAXRAND 140
#define ADDR 160
#define HEXXID 40
#define SEEDS 150000
#define SEEDFILE "seeds"
#define BITRAND 2
#define BYTE 8
#define DUPS 5000 // This is simply an observation of the actual tables created

/* 
 * This is a structure that consists of one entry in the table generated.
 * The name of the structure is a misnomer.
 * Initially it was planned to be a structure to generate nexthop entry
 */
struct nextcreate {
	unsigned char prefix[HEXXID];
	unsigned int len;
	unsigned int nexthop;
};

struct duplicate {
	struct nextcreate *ori;
	struct nextcreate *dup;
};

/*
 * Print the table
 * */
static int print_table(struct nextcreate **pretable, double size, int prexp)
{
	unsigned int i, j;
	unsigned int max = (unsigned int) (size + 0.5);
	unsigned char tmp_prefix[HEXXID + 1] = {0};
	char file_name[MAXFILENAME];
	FILE *tmp_file;

	memset(file_name, 0, MAXFILENAME);
	sprintf(file_name, "%d.table", prexp);
	tmp_file = fopen(file_name, "w+");

	for (i = 0; i < max; i++) {
		memcpy(tmp_prefix, pretable[i]->prefix, HEXXID);
		for (j = 0; j < 20; j++)
			fprintf(tmp_file, "%02x", pretable[i]->prefix[j]);
		fprintf(tmp_file, " %d %d\n", pretable[i]->len,
				pretable[i]->nexthop);
	}
	fclose(tmp_file);
}

/*
 * Comparison function to compare prefixes
 * */
static int compareprefix(const void *id1, const void *id2)
{
	int res;
	unsigned char **tmp1 = (unsigned char **) id1;
	unsigned char **tmp2 = (unsigned char **) id2;

	res = memcmp(*tmp1, *tmp2, 20);

	if (res < 0)
		return -1;
	else if (res > 0)
		return 1;
	else
		return 0;
}
/*
 * Comparison function to be passed to qsort for sorting of entries
 * */
static int sortentries(const void *e1, const void *e2)
{
	int res;
	struct nextcreate **entry1 = (struct nextcreate **) e1;
	struct nextcreate **entry2 = (struct nextcreate **) e2;
	unsigned char *pre1 = (*entry1)->prefix;
	unsigned char *pre2 = (*entry2)->prefix;

	res = compareprefix(&pre1, &pre2);

	if (res < 0)
		return -1;
	else if (res > 0)
		return 1;
	else
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
static int nexthop_dist(struct nextcreate **pretable, double size,
		uint32_t *seeds, int tablexp)
{
	struct nextcreate **tmp_table = NULL;
	int i, j;
	struct duplicate **dup_table = NULL;
	unsigned int maxnexthops;
	unsigned long int tmp;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seeds[tablexp - LEXPFIB + 2 * HEXPFIB]);

	tmp_table = malloc(size * sizeof(struct nextcreate *));
	for (i = 0; i < size; i++)
		tmp_table[i] = pretable[i];

	qsort(tmp_table, size, sizeof(struct nextcreate *), sortentries);
	dup_table = malloc(DUPS * sizeof(struct duplicate *));
	j = 0;
	for (i = 1; i < size; i++) {
		if ((0 == sortentries(&tmp_table[i - 1], &tmp_table[i])) &&
				(tmp_table[i-1]->len == tmp_table[i]->len)) {
			dup_table[j] = malloc(sizeof(struct duplicate));
			dup_table[j]->ori = tmp_table[i - 1];
			dup_table[j]->dup = tmp_table[i];
			j++;
		}
	}
	maxnexthops = (unsigned int) (gsl_sf_pow_int(2.0, tablexp/2) + 0.5);
	for (i = 0; i < size; i++) {
		tmp = gsl_rng_uniform_int(r, maxnexthops) + 1;
		tmp_table[i]->nexthop = (unsigned int) tmp;
	}
	for (i = 0; i < j; i++)
		(dup_table[i]->dup)->len = (dup_table[i]->ori)->len;
	free(tmp_table);
	for (i = 0; i < j; i++)
		free(dup_table[i]);
	free(dup_table);
}

/* 
 * This routine is concerned with the generation of uniformly distributed
 * prefixes of certain prefix length.
 *
 * Inputs:
 * pretable: An array consisting of pointers to the entry structs
 * size: number of entries in the table
 * seeds: array containing seeds
 * tablexp: log2(size)
 */

static int prefix_dist(struct nextcreate **pretable, double size,
		uint32_t *seeds, int tablexp)
{
	unsigned int i, j;
	unsigned char tmp_prefix[HEXXID] = {0};
	unsigned long int tmp_bit;
	unsigned char tmp_char;
	unsigned int index, bit;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seeds[tablexp - LEXPFIB + HEXPFIB]);

	for (i = 0; i < size; i++) {
		memset(pretable[i]->prefix, 0, HEXXID);
		for (j = 0; j < pretable[i]->len; j++) {
			tmp_bit = gsl_rng_uniform_int(r, BITRAND);
			index = j / BYTE;
			bit  = j % BYTE;
			// In order to be endian agnostic
			if (1 == tmp_bit) {
				tmp_char = 1;
				tmp_char = tmp_char << (BYTE - bit - 1);
			} else {
				tmp_char = 0;
			}
			pretable[i]->prefix[index] =
				pretable[i]->prefix[index] | tmp_char;
		}
	}
	return 0;
}

/* 
 * This routine is concerned with the generation of uniformly distributed prefix
 * lengths. We are aware of this strong assumption and will soon replace this
 * with a more suitable distribution.
 *
 * pretable: An array consisting of pointers to the entry structs
 * size: number of entries in the table
 * seeds: array containing seeds
 * tablexp: log2(size)
 */
static int prelength_dist(struct nextcreate **pretable, double size,
		uint32_t *seeds, int tablexp)
{
	int i;
	gsl_rng *r;
	unsigned long int tmp;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seeds[tablexp - LEXPFIB]);

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
 */
static int table_dist(void)
{
	int i, j, seedsize;
	double size;
	char *file_names[HEXPFIB - LEXPFIB + 1];
	FILE *tmp_file, *seed_file;
	struct nextcreate **pretable = NULL;
	uint32_t *seeds;

	seeds = malloc(SEEDS * sizeof(uint32_t));
	memset(seeds, 0, SEEDS * sizeof(uint32_t));
	seed_file = fopen(SEEDFILE, "r");
	i = 0;
	while (fscanf(seed_file, "%8x\n", &seeds[i]) != EOF)
		i++;
	seedsize = i;
	fclose(seed_file);

	i = 0;
	for (i = LEXPFIB; i <= HEXPFIB; i++) {
		size = gsl_sf_pow_int(2.0, i);
		pretable = malloc(size * sizeof(struct nextcreate *));
		for (j = 0; j < size; j++)
			pretable[j] = malloc(sizeof(struct nextcreate));
		prelength_dist(pretable, size, seeds, i);
		prefix_dist(pretable, size, seeds, i);
		nexthop_dist(pretable, size, seeds, i);
		printf("%d\n", i);
		print_table(pretable, size, i);
		for (j = 0; j < size; j++)
			free(pretable[j]);
		free(pretable);
	}

	return 0;
}
