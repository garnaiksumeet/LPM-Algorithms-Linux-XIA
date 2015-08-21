#include "./Data-Generation/generate_fibs.h"
#include "./Bloom-Filter/bloom.h"
#include "./LC-Trie/lc_trie.h"
#include <fcntl.h>
#include <time.h>

#define LEXPFIB 4
#define HEXPFIB 20
#define NLOOKUPS 1000000
#define RUNS 20
#define LOOPSEED (RUNS* (NEXTSEED + 1))

static inline unsigned long gettime(struct timespec x, struct timespec y)
{
	unsigned long tmp;
	assert((tmp = (y.tv_sec - x.tv_sec) * 1000000000L +
				(y.tv_nsec - x.tv_nsec)) > 0);
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
		const void *s)
{
	struct timespec start, stop;
	struct nextcreate *table = (struct nextcreate *) t;
	unsigned long *size = (unsigned long *) ts;
	uint32_t *seed = (uint32_t *) s;
	unsigned long int tmp;
	unsigned long accum = 0;
	int i;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, *seed);

	// Create the data structure
	for (i = 0; i < NLOOKUPS; i++) {
		tmp = gsl_rng_uniform_int(r, *size);
		// Sample from table and set the XID in appropriate form
		time_measure(&start);
		// Perform lookup
		time_measure(&stop);
		accum += gettime(start, stop);
	}
	gsl_rng_free(r);
	return 0;
}

static int evaluate_lookups_bloom(const void *t, const void *ts,
		const void *s)
{
	struct timespec start, stop;
	struct nextcreate *table = (struct nextcreate *) t;
	unsigned long *size = (unsigned long *) ts;
	uint32_t *seed = (uint32_t *) s;
	int i;
	unsigned long int tmp;
	unsigned long accum = 0;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, *seed);

	// Create the data structure
	for (i = 0; i < NLOOKUPS; i++) {
		tmp = gsl_rng_uniform_int(r, *size);
		// Sample from table and set the XID in appropriate form
		time_measure(&start);
		// Perform lookup
		time_measure(&stop);
		accum += gettime(start, stop);
	}
	gsl_rng_free(r);
	return 0;
}

static int lookup_experiments(int exp, uint32_t *seeds, int low, int seedsize,
		int nnexthops)
{
	int i, j;
	pid_t id;
	unsigned long size = 1 << exp;
	struct nextcreate *table = NULL;
	int o_seed = low;
	int (*experiments[])(const void *, const void *, const void *) = {
		evaluate_lookups_bloom,
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
				assert(0 == (experiments[i])
						(table, &size, &seeds[low]));
				free(table);
				exit(EXIT_SUCCESS);
			} else {
				assert(wait(NULL) >= 0);
				// The extra 1 is to account for the seed
				// consumed in generating the XIDs for lookups
				low = low + NEXTSEED + 1;
				assert(low < seedsize);
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
	}

	return 0;
}
