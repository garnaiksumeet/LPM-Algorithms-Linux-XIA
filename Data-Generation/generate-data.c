#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_sf_pow_int.h>
#include <gsl/gsl_randist.h>
#include <gmp.h>

#define SEED 10000
#define LEXPFIB 4
#define HEXPFIB 20
#define MAXFILENAME 10
#define OFFSET 1
#define ADDR 160
#define HEXXID 40

struct nextcreate {
	char prefix[HEXXID + 1];
	int len;
	char nexthop[HEXXID + 1];
};

static int prelength_dist(unsigned long int *prelength, double size,
		int *prelennum);
static int table_dist(void);
static int prefix_dist(FILE *file, double size, int *prelennum);
static int buffer_table(FILE *file, int fibsize);
static int generate_nexthops(struct nextcreate **pretable,
		struct nextcreate **uniqtable, int fibsize);
static int in_table(struct nextcreate **table, int size,
		                struct nextcreate *entry);
static int final_table(struct nextcreate **pretable, int fibsize);

int main(void)
{
	table_dist();
	return 0;
}

static int table_dist(void)
{
	int i, j, max_nodes;
	int prelennum[ADDR];
	unsigned long int *prelength[HEXPFIB - LEXPFIB + 1];
	double size;
	char *file_names[HEXPFIB - LEXPFIB + 1];
	FILE *tmp_file;
	struct nextcreate **pretable = NULL;
	struct nextcreate **uniqtable = NULL;

	for (i = LEXPFIB; i <= HEXPFIB; i++) {
		memset(prelennum, 0, sizeof(int) * ADDR);
		size = gsl_sf_pow_int(2.0, i);
		prelength[i - LEXPFIB] = malloc(size * sizeof(unsigned long int));
		prelength_dist(prelength[i - LEXPFIB], size, prelennum);
		file_names[i - LEXPFIB] = malloc(MAXFILENAME);
		memset(file_names[i - LEXPFIB], 0, MAXFILENAME);
		sprintf(file_names[i - LEXPFIB], "%d.len", i);
		tmp_file = fopen(file_names[i - LEXPFIB], "w+");

		for (j = 0; j < size; j++)
			fprintf(tmp_file, "%lu\n", prelength[i - LEXPFIB][j]);
		fclose(tmp_file);

		memset(file_names[i - LEXPFIB], 0, MAXFILENAME);
		sprintf(file_names[i - LEXPFIB], "%d.tmp", i);
		tmp_file = fopen(file_names[i - LEXPFIB], "w+");
		prefix_dist(tmp_file, size, prelennum);
		fclose(tmp_file);
		tmp_file = fopen(file_names[i - LEXPFIB], "r");
		buffer_table(tmp_file, i);
		fclose(tmp_file);

		pretable = malloc(size * sizeof(struct nextcreate *));
		uniqtable = malloc(size * sizeof(struct nextcreate *));

		max_nodes = generate_nexthops(pretable, uniqtable, i);
		final_table(pretable, i);
		printf("Routing table of size 2^%d is generated.\n", i);
		free(prelength[i - LEXPFIB]);
		for (j = 0; j < max_nodes; j++)
			free(uniqtable[j]);
		free(pretable);
		free(uniqtable);
	}

	return 0;
}

static int final_table(struct nextcreate **pretable, int fibsize)
{
	int i, size;
	gsl_rng *r;
	char file_name[MAXFILENAME];
	FILE *file;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, SEED);

	sprintf(file_name, "%d.table", fibsize);
	file = fopen(file_name, "w+");

	size = (int) (gsl_sf_pow_int(2.0, fibsize) + 0.5);

	gsl_ran_shuffle(r, pretable, size, sizeof(struct nextcreate *));

	for (i = 0; i < size; i++)
		fprintf(file, "%s %d %s\n", pretable[i]->prefix,
				pretable[i]->len, pretable[i]->nexthop);
	gsl_rng_free(r);
	fclose(file);
	return 0;
}

static int generate_nexthops(struct nextcreate **pretable,
		struct nextcreate **uniqtable, int fibsize)
{
	char file_name[MAXFILENAME] = {0};
	FILE *tmp_file;
	int i, j, ctmp;
	int size = (int) (gsl_sf_pow_int(2.0, fibsize) + 0.5);
	int rndbit;
	mpz_t randnum;
	gmp_randstate_t gmprandstate;


	for (i = 0; i < size; i++) {
		pretable[i] = NULL;
		uniqtable[i] = NULL;
	}

	sprintf(file_name, "%d.pre", fibsize);
	tmp_file = fopen(file_name, "r");

	for (i = 0; i < size; i++) {
		pretable[i] = malloc(sizeof(struct nextcreate));
		fscanf(tmp_file, "%s%d", pretable[i]->prefix,
				&pretable[i]->len);
		memset(pretable[i]->nexthop, 0, HEXXID + 1);
	}
	fclose(tmp_file);

	j = 0;
	for (i = 0; i < size; i++) {
		ctmp = in_table(uniqtable, j, pretable[i]);
		if (-1 == ctmp) {
			uniqtable[j] = pretable[i];
			j++;
		} else {
			free(pretable[i]);
			pretable[i] = uniqtable[ctmp];
		}
	}

	mpz_init(randnum);
	rndbit = fibsize / 2;
	gmp_randinit_default(gmprandstate);
	gmp_randseed_ui(gmprandstate, SEED);
	for (i = 0; i < j; i++) {
		mpz_urandomb(randnum, gmprandstate, rndbit);
		gmp_sprintf(uniqtable[i]->nexthop, "%040Zx", randnum);
	}

	return j;
}

static int in_table(struct nextcreate **table, int size,
				struct nextcreate *entry)
{
	int i;

	for (i = 0; i < size; i++) {
		if ((entry->len == table[i]->len) &&
			(0 == strncmp(table[i]->prefix, entry->prefix, 40)))
			return i;
	}
	return -1;
}

static int buffer_table(FILE *file, int fibsize)
{
	char tmp_file_name[MAXFILENAME];
	char tmp_buf[HEXXID + 1] = {0};
	int len, tmp_len;
	int i;
	FILE *tmp_file;

	memset(tmp_file_name, 0, MAXFILENAME);
	sprintf(tmp_file_name, "%d.pre", fibsize);
	tmp_file = fopen(tmp_file_name, "w+");

	while (fscanf(file, "%s%d", &tmp_buf, &len) != EOF) {
		fprintf(tmp_file, "%s", tmp_buf);
		tmp_len = strlen(tmp_buf);
		for (i = 0; i < (HEXXID - tmp_len); i++)
			fprintf(tmp_file, "0");
		fprintf(tmp_file, " %d\n", len);
	}
	fclose(tmp_file);
	return 0;
}

static int prefix_dist(FILE *file, double size, int *prelennum)
{
	int i, j;
	int rndBit;
	mpz_t tmp_rand;
	gmp_randstate_t gmpRandState;

	mpz_init(tmp_rand);
	gmp_randinit_default(gmpRandState);

	for (i = 0; i < ADDR; i++) {
		rndBit = i + OFFSET;
		for (j = 0; j < prelennum[i]; j++) {
			mpz_urandomb(tmp_rand, gmpRandState, rndBit);
			gmp_fprintf(file, "%02Zx %d\n", tmp_rand, i + OFFSET);
		}
	}
	return 0;
}

static int prelength_dist(unsigned long int *prelength, double size,
		int *prelennum)
{
	int i;
	gsl_rng *r;
	unsigned long int tmp;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, SEED);

	for (i = 0; i < size; i++) {
		tmp = gsl_rng_uniform_int(r, 160) + OFFSET;
		prelennum[tmp - OFFSET]++;
		prelength[i] = tmp;
	}

	gsl_rng_free(r);

	return 0;
}
