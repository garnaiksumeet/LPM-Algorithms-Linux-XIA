#include "lpm_bloom.h"

#define SALT_CONSTANT 0x97c29b3a

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

static int bloom_proportion(struct bloom_structure *filter,
		struct nextcreate **tmp_table, unsigned long size)
{
	int j = 0;
	int i;

	for (i = MINLENGTH; i <= MAXLENGTH; i++) {
		if (i != tmp_table[j]->len)
			continue;
		filter->low[i - MINLENGTH] = j;
		while (i == tmp_table[j]->len) {
			j++;
			if (size == j)
				break;
		}
		filter->high[i - MINLENGTH] = (j - 1);
		filter->length[i - MINLENGTH] = j - filter->low[i - MINLENGTH];
		filter->bloom[i - MINLENGTH] = NULL;
		filter->hashtable[i - MINLENGTH] = NULL;
		filter->flag[i - MINLENGTH] = 1;
		if (size == j)
			break;
	}
}

int bloom_destroy_fib(struct bloom_structure *filter)
{
	int i;

	for (i = 0; i < WDIST; i++) {
		if (filter->flag) {
			free_counting_bloom(filter->bloom[i]);
			hashmap_destroy(filter->hashtable[i]);
		}
	}
	free(filter);

	return 0;
}

struct bloom_structure *bloom_create_fib(struct nextcreate *table,
		unsigned long size, double error_rate)
{
	int i, j;
	unsigned char xid[HEXXID + 1] = {0};
	struct nextcreate **tmp_table = NULL;
	struct bloom_structure *filter = NULL;

	filter = calloc(1, sizeof(struct bloom_structure));
	assert(filter);
	memset(filter->low, -1, WDIST * sizeof(int));
	memset(filter->high, -1, WDIST * sizeof(int));

	tmp_table = malloc(size * sizeof(struct nextcreate *));
	assert(tmp_table);
	for (i = 0; i < size; i++)
		tmp_table[i] = &table[i];

	qsort(tmp_table, size, sizeof(struct nextcreate *), sortbylength);
	bloom_proportion(filter, tmp_table, size);

	for (i = 0; i < WDIST; i++) {
		if (!filter->flag[i])
			continue;
		if (!(filter->bloom[i] =
		new_counting_bloom(filter->length[i] * NTIMES, error_rate))) {
			printf("ERROR: Could not create bloom filter\n");
			return NULL;
		}
		assert(0 == hashmap_init(filter->length[i], &filter->hashtable[i]));
		for (j = filter->low[i]; j <= filter->high[i]; j++) {
			memcpy(xid, tmp_table[j]->prefix, HEXXID);
			assert(0 == counting_bloom_add(filter->bloom[i],
				filter->hashtable[i], xid, HEXXID,
				tmp_table[j]->nexthop));
		}
	}

	return filter;
}

unsigned int lookup_bloom(unsigned char *id, void *bf)
{
	int i;
	uint64_t out[2];
	unsigned int nexthop;
	struct bloom_structure *filter = (struct bloom_structure *) bf;
	// The returned values of counting_bloom_check() are 0 if found else 1
	unsigned char matchvec[WDIST] = {1};

	// Although the paper suggests to perform parallel membership queries
	for (i = WDIST - 1; i >= 0; i--) {
		id[i / BYTE] = id[i / BYTE] >> (BYTE - i % BYTE) <<
			(BYTE - i % BYTE);
		if (!filter->flag[i])
			continue;
		matchvec[i] = counting_bloom_check(filter->bloom[i], id,
								HEXXID);
	}
	// Parse the matchvec in increasing order and perform hashmap searches
	for (i = 0; i < WDIST; i++) {
		if (matchvec[i])
			continue;
		MurmurHash3_x64_128(id, HEXXID, SALT_CONSTANT, out);
		if (!hashmap_get(filter->hashtable[i], id, out[1], &nexthop))
			return nexthop;
	}

	return 0;
}
