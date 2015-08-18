#include "bloom.h"
#include "murmur.h"

#define WDIST 140
#define MINLENGTH 20
#define MAXLENGTH 159
#define NTIMES 2

struct bloom_structure {
	// Although this could be computed using low and high, we are storing it
	// for convenience.
	unsigned int length[WDIST];
	int low[WDIST];
	int high[WDIST];
	counting_bloom_t *bloom[WDIST];
};

static int sortbylength(const void *e1, const void *e2)
{
	int res;
	struct nextcreate **entry1 = (struct nextcreate **) e1;
	struct nextcreate **entry2 = (struct nextcreate **) e2;
	unsigned int len1 = (*entry1)->len;
	unsigned int len2 = (*entry2)->len;

	if (len1 < len2)
		return -1;
	else if (len1 > len2)
		return 1;
	else
		return 0;
}

static int bloom_proportion(struct bloom_structure *bloom_filter,
		struct nextcreate **tmp_table, unsigned long size)
{
	int j = 0;
	int i;

	for (i = MINLENGTH; i <= MAXLENGTH; i++) {
		if (i != tmp_table[j]->len)
			continue;
		bloom_filter->low[i - MINLENGTH] = j;
		while (i == tmp_table[j]->len) {
			j++;
			if (size == j)
				break;
		}
		bloom_filter->high[i - MINLENGTH] = (j - 1);
		bloom_filter->length[i - MINLENGTH] =
			j - bloom_filter->low[i - MINLENGTH];
		bloom_filter->bloom[i - MINLENGTH] = NULL;
		if (size == j)
			break;
	}
}

int destroy_fib(struct bloom_structure *filter)
{
	int i;

	for (i = 0; i < WDIST; i++) {
		if (0 != bloom_filter->length[i])
			free_counting_bloom(bloom_filter->bloom[i]);
	}

	return 0;
}

struct bloom_structure *create_fib(struct nextcreate *table,
		unsigned long size, double error_rate)
{
	int i;
	char xid[HEXXID + 1];
	struct nextcreate **tmp_table = NULL;
	struct bloom_structure *filter = NULL;

	filter = malloc(sizeof(struct bloom_structure));
	assert(filter);
	memset(filter->length, 0, WDIST * sizeof(unsigned int));
	memset(filter->low, -1, WDIST * sizeof(int));
	memset(filter->high, -1, WDIST * sizeof(int));
	memset(filter->bloom, 0, WDIST * sizeof(struct counting_bloom_t *));

	tmp_table = malloc(size * sizeof(struct nextcreate *));
	assert(tmp_table);
	for (i = 0; i < size; i++)
		tmp_table[i] = &table[i];

	qsort(tmp_table, size, sizeof(struct nextcreate *), sortbylength);
	bloom_proportion(filter, tmp_table, size);

	for (i = 0; i < WDIST; i++) {
		if (0 == filter->length[i])
			continue;
		if (!(filter->bloom[i] =
		new_counting_bloom(filter->length[i] * NTIMES, error_rate))) {
			printf("ERROR: Could not create bloom filter\n");
			return 1;
		}
		for (j = filter->low[i]; j <= filter->high[i]; j++) {
			memset(xid, 0, HEXXID + 1);
			memcpy(xid, tmp_table[j]->prefix, HEXXID);
			assert(0 == counting_bloom_add(filter->bloom[i], xid,
						HEXXID));
		}
	}

	return filter;
}
