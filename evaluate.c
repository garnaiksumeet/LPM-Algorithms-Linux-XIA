#include "../Data-Generation/generate_fibs.h"
#include "../Bloom-Filter/bloom.h"
#include "../LC-Trie/lc_trie.h"
#include <fcntl.h>
#include <time.h>

#define START 0
#define STOP 1
#define RESOLUTION 1000000000L
#define RUNS 21
#define NSAMPLES (size / 5)
#define NEXTSEED (RUNS * 3)
#define LOOPSEED 3
#define GETTIME(x, y) ((y.tv_sec - y.tv_sec) * RESOLUTION + (y.tv_nsec - x.tv_nsec))

static int time_measure(struct timespec *ntime)
{
	if(clock_gettime(CLOCK_MONOTONIC, ntime) == -1) {
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}

	return 0;
}

static struct nextcreate **sample_table(struct nextcreate *table,
		unsigned long size, uint32_t seed)
{
	struct nextcreate **sample = NULL;
	unsigned long int tmp;
	int i;
	gsl_rng *r;

	r = gsl_rng_alloc(gsl_rng_ranlux);
	gsl_rng_set(r, seed);

	sample = malloc(sizeof(struct nextcreate *) * NSAMPLES);
	for (i = 0; i < NSAMPLES; i++) {
		tmp = gsl_rng_uniform_int(r, size);
		sample[i] = &table[tmp];
	}
	gsl_rng_free(r);

	return sample;
}

static int evaluate_lookups_bloom(const void *t, const void *s, const void *ls)
{
	// Perform relevant actions
	return 0;
}

static int lookup_experiments(struct nextcreate *table, int exp,
		uint32_t *seeds, int low, int seedsize, int nnexthops)
{
	int i, j;
	struct nextcreate **lookup_sample = NULL;
	pid_t id;
	int val;
	unsigned long size = 1 << exp;
	unsigned long accum;
	int (*experiments[])(const void *, const void *, const void *) = {
		evaluate_lookups_bloom,
		NULL,
	};

	for (i = 0; experiments[i] != NULL;  i++) {
		for (j = 1; j < RUNS; j++) {
			assert(0 == table_dist(i, seeds, low, table, seedsize,
					nnexthops));
			low = low + LOOPSEED;
			assert(low < seedsize);
			lookup_sample = sample_table(table, size, seeds[low]);
			assert(NULL != lookup_sample);
			low++;
			assert(low < seedsize);
			id = fork();
			assert(-1 != id);
			if (0 == id) {
				val = (experiments[i])(table, &size,
						lookup_sample);
				assert(0 == val);
				exit(EXIT_SUCCESS);
			} else {
				wait(NULL);
			}
			sleep(4);
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	unsigned long size;
	struct nextcreate *table = NULL;
	FILE *seed_file = NULL;
	uint32_t *seeds = NULL;
	int seedsize = 0;
	int low = 0;
	int nnexthops = 0;
	int val;

	seeds = malloc(SEEDS * sizeof(uint32_t));
	memset(seeds, 0, SEEDS * sizeof(uint32_t));
	seed_file = fopen(SEEDFILE, "r");
	assert(NULL != seed_file);
	i = 0;
	while (fscanf(seed_file, "%8x\n", &seeds[i]) != EOF)
		i++;
	seedsize = i;
	fclose(seed_file);

	for (i = LEXPFIB; i <= HEXPFIB; i++) {
		size = 1 << i;
		table = malloc(sizeof(struct nextcreate) * size);
		nnexthops = 1 << (i / 2);
		val = lookup_experiments(table, i, seeds, low, seedsize,
				nnexthops);
		low = low + NEXTSEED;
		assert(low < seedsize);
		sleep(5);
		free(table);
	}
}
