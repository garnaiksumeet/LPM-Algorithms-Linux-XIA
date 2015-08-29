#include "../Data-Generation/generate_fibs.h"
#include "lc_trie.h"
#include <fcntl.h>
#include <time.h>
#include <inttypes.h>

#define LEXPFIB 4
#define HEXPFIB 20
#define NLOOKUPS 1000000
#define RUNS 20
#define LOOPSEED ((RUNS) * (NEXTSEED + 1))

static inline unsigned long gettime(const struct timespec *x,
		const struct timespec *y)
{
	unsigned long tmp;
	assert((tmp =
	(y->tv_sec - x->tv_sec) * 1000000000L + (y->tv_nsec - x->tv_nsec)) > 0);
	return tmp;
}

static int time_measure(struct timespec *ntime)
{
	if(clock_gettime(CLOCK_MONOTONIC, ntime) == -1) {
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}

	return 0;
}

static int evaluate_lookups_lctrie(const void *t, const void *ts,
		const void *s, FILE *fp)
{
	struct timespec start, stop;
	struct nextcreate *table = (struct nextcreate *) t;
	unsigned long *size = (unsigned long *) ts;
	uint32_t *seed = (uint32_t *) s;
	unsigned long int tmp;
	unsigned long accum = 0;
	int i, j;
	gsl_rng *r;
	xid *id = NULL;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, *seed);

	// Create the data structure
	struct routtablerec *fib = lctrie_create_fib(table, *size);
	return 0;
	node_t *trie = fib->trie;
	for (j = 0; j < fib->triesize; j++)
		fprintf(fp, "%"PRIu64"\n", GETBRANCH(trie[j]));
/*	for (i = 0; i < NLOOKUPS; i++) {
		tmp = gsl_rng_uniform_int(r, *size);
		// Sample from table and set the XID in appropriate form
		id = (xid *) &(table[tmp].prefix);
		time_measure(&start);
		// Perform lookup
		lctrie_lookup(id, fib);
		time_measure(&stop);
		accum += gettime(&start, &stop);
	}
	printf("%lu\t%d\t", accum, i);*/
	assert(!lctrie_destroy_fib(fib));
	gsl_rng_free(r);
	return 0;
}

static int lookup_experiments(int exp, uint32_t *seeds, int low, int seedsize,
		int nnexthops)
{
	int i, j, k, l;
	pid_t id;
	unsigned long size = 1 << exp;
	struct nextcreate *table = NULL;
	char file_name[20] = {0};
	unsigned char tmp_prefix[HEXXID] = {0};
	FILE *fp , *tmp_file;
	int o_seed = low;
	int (*experiments[])(const void *, const void *, const void *, FILE *) = {
		evaluate_lookups_lctrie,
		NULL,
	};

	for (i = 0; experiments[i] != NULL;  i++) {
		low = o_seed;
		for (j = 0; j < RUNS; j++) {
			id = fork();
			assert(id >= 0);
			if (0 == id) {
				table = malloc(sizeof(struct nextcreate) *
						size);
				assert(0 == table_dist(exp, seeds, low, table,
							seedsize, nnexthops));
				low = low + NEXTSEED;
				assert(low < seedsize);
/*				sprintf(file_name, "%d.table", j);
				tmp_file = fopen(file_name, "w+");
				for (k = 0; k < size; k++) {
					memcpy(tmp_prefix, table[k].prefix, HEXXID);
					for (l = 0; l < HEXXID; l++)
						fprintf(tmp_file, "%02x", tmp_prefix[l]);
					fprintf(tmp_file, " %d %d\n", table[k].len, table[k].nexthop);
				}
				fclose(tmp_file);
				memset(file_name, 0, HEXXID);
				sprintf(file_name, "%d.4", j);
				fp = fopen(file_name, "w+");*/
				assert(0 == (experiments[i])
						(table, &size, &seeds[low], fp));
				free(table);
				fclose(fp);
				exit(EXIT_SUCCESS);
			} else {
				assert(wait(NULL) >= 0);
				// The extra 1 is to account for the seed
				// consumed in generating the XIDs for lookups
				low = low + NEXTSEED + 1;
				assert(low < seedsize);
				printf("Experiment:%d\tRun:%d\n", i, j);
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	unsigned long size;
	FILE *seed_file = NULL;
	uint32_t *seeds = NULL;
	int seedsize = 0;
	int low = 0;
	int nnexthops = 0;
	int val;

	assert(0 == access(SEEDFILE, R_OK | F_OK));
	seeds = malloc(SEEDS * sizeof(uint32_t));
	seed_file = fopen(SEEDFILE, "r");
	assert(seed_file);
	i = 0;
	while (fscanf(seed_file, "%8x\n", &seeds[i]) != EOF)
		assert(i++ < SEEDS);
	seedsize = i;
	assert(0 == fclose(seed_file));

	for (i = LEXPFIB; i <= HEXPFIB; i++) {
		size = 1 << i;
		nnexthops = 16; // For the time being we assume a constant
		assert(0 == lookup_experiments(i, seeds, low, seedsize,
					nnexthops));
		low = low + LOOPSEED;
		assert(low < seedsize);
		printf("Done %d\n", i);
	}
	free(seeds);

	return 0;
}
