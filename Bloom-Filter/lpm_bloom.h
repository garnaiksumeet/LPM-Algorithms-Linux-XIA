/* 
 * This is a free software and provides no guarantee of any kind.
 * The distribution and changes to the software is as provided by the LICENSE.
 * View the LICENSE file in github.com/sumefsp/LPM-Algorithms-Linux-XIA and
 * any usage of code from this file must include this declaration.
 *
 * 2015, LPM Algorithms for Linux XIA
 * Garnaik Sumeet, Michel Machado
 */

#ifndef __LPM_BLOOM_H__
#define __LPM_BLOOM_H__

#include "bloom.h"
#include "murmur.h"
#include "hashmap.h"
#include "../Data-Generation/generate_fibs.h"
#include <stdbool.h>

#define WDIST 140
#define MINLENGTH 20
#define MAXLENGTH 159
#define NTIMES 2

struct bloom_structure {
	// Although this could be computed using low and high, we are storing it
	// for convenience.
	bool flag[WDIST];
	unsigned int length[WDIST];
	int low[WDIST];
	int high[WDIST];
	counting_bloom_t *bloom[WDIST];
	struct hashmap *hashtable[WDIST];
};

struct bloom_structure *create_fib(struct nextcreate *table,
		unsigned long size, double error_rate);
unsigned int lookup_bloom(const char *id, void *bf);
int destroy_fib(struct bloom_structure *filter);

#endif
