/* Copyright @2012 by Justin Hines at Bitly under a very liberal license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * 2015, LPM Algorithms for Linux XIA
 * Garnaik Sumeet, Michel Machado
 */

#define _GNU_SOURCE
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include "murmur.h"
#include "bloom.h"

#define ERROR_TIGHTENING_RATIO 0.5
#define SALT_CONSTANT 0x97c29b3a

void free_bitmap(bitmap_t *bitmap)
{
	if ((munmap(bitmap->array, bitmap->bytes)) < 0) {
		perror("Error, unmapping memory");
	}
	close(bitmap->fd);
	free(bitmap);
}

bitmap_t *bitmap_resize(bitmap_t *bitmap, size_t old_size, size_t new_size)
{
	int fd = bitmap->fd;
	struct stat fileStat;

	fstat(fd, &fileStat);
	size_t size = fileStat.st_size;

	/* grow file if necessary */
	if (size < new_size) {
		if (ftruncate(fd, new_size) < 0) {
			perror("Error increasing file size with ftruncate");
			free_bitmap(bitmap);
			close(fd);
			return NULL;
		}
	}
	lseek(fd, 0, SEEK_SET);

	/* resize if mmap exists and possible on this os, else new mmap */
	if (bitmap->array != NULL) {
		bitmap->array = mremap(bitmap->array, old_size, new_size,
					MREMAP_MAYMOVE);
		if (bitmap->array == MAP_FAILED) {
			perror("Error resizing mmap");
			free_bitmap(bitmap);
			close(fd);
			return NULL;
		}
	}
	if (bitmap->array == NULL) {
		bitmap->array = mmap(0, new_size, PROT_READ | PROT_WRITE,
						MAP_SHARED, fd, 0);
		if (bitmap->array == MAP_FAILED) {
			perror("Error init mmap");
			free_bitmap(bitmap);
			close(fd);
			return NULL;
		}
	}

	bitmap->bytes = new_size;
	return bitmap;
}

/* Create a new bitmap, not full featured, simple to give
 * us a means of interacting with the 4 bit counters */
bitmap_t *new_bitmap(int fd, size_t bytes)
{
	bitmap_t *bitmap;

	if ((bitmap = (bitmap_t *)malloc(sizeof(bitmap_t))) == NULL) {
		return NULL;
	}

	bitmap->bytes = bytes;
	bitmap->fd = fd;
	bitmap->array = NULL;

	if ((bitmap = bitmap_resize(bitmap, 0, bytes)) == NULL) {
		return NULL;
	}

	return bitmap;
}

int bitmap_increment(bitmap_t *bitmap, unsigned int index, long offset)
{
	long access = index / 2 + offset;
	uint8_t temp;
	uint8_t n = bitmap->array[access];
	if (index % 2 != 0) {
		temp = (n & 0x0f);
		n = (n & 0xf0) + ((n & 0x0f) + 0x01);
	} else {
		temp = (n & 0xf0) >> 4;
		n = (n & 0x0f) + ((n & 0xf0) + 0x10);
	}

	if (temp == 0x0f) {
		fprintf(stderr, "Error, 4 bit int Overflow\n");
		return -1;
	}

	bitmap->array[access] = n;
	return 0;
}

/* increments the four bit counter */
int bitmap_decrement(bitmap_t *bitmap, unsigned int index, long offset)
{
	long access = index / 2 + offset;
	uint8_t temp;
	uint8_t n = bitmap->array[access];

	if (index % 2 != 0) {
		temp = (n & 0x0f);
		n = (n & 0xf0) + ((n & 0x0f) - 0x01);
	} else {
		temp = (n & 0xf0) >> 4;
		n = (n & 0x0f) + ((n & 0xf0) - 0x10);
	}

	if (temp == 0x00) {
		fprintf(stderr, "Error, Decrementing zero\n");
		return -1;
	}

	bitmap->array[access] = n;
	return 0;
}

/* decrements the four bit counter */
int bitmap_check(bitmap_t *bitmap, unsigned int index, long offset)
{
	long access = index / 2 + offset;
	if (index % 2 != 0 ) {
		return bitmap->array[access] & 0x0f;
	} else {
		return bitmap->array[access] & 0xf0;
	}
}

int bitmap_flush(bitmap_t *bitmap)
{
	if ((msync(bitmap->array, bitmap->bytes, MS_SYNC) < 0)) {
		perror("Error, flushing bitmap to disk");
		return -1;
	} else {
		return 0;
	}
}

/*
 * Perform the actual hashing for `key`
 *
 * Only call the hash once to get a pair of initial values (h1 and
 * h2). Use these values to generate all hashes in a quick loop.
 *
 * See paper by Kirsch, Mitzenmacher [2006]
 * http://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf
 */
void hash_func(counting_bloom_t *bloom, const char *key, size_t key_len,
		uint32_t *hashes)
{
	int i;
	uint32_t checksum[4];

	MurmurHash3_x64_128(key, key_len, SALT_CONSTANT, checksum);
	uint32_t h1 = checksum[0];
	uint32_t h2 = checksum[1];

	for (i = 0; i < bloom->nfuncs; i++) {
		hashes[i] = (h1 + i * h2) % bloom->counts_per_func;
	}
}

int free_counting_bloom(counting_bloom_t *bloom)
{
	if (bloom != NULL) {
		free(bloom->hashes);
		bloom->hashes = NULL;
		free(bloom->bitmap);
		free(bloom);
		bloom = NULL;
	}
	return 0;
}

counting_bloom_t *counting_bloom_init(unsigned int capacity,
		double error_rate, long offset)
{
	counting_bloom_t *bloom;

	if ((bloom = malloc(sizeof(counting_bloom_t))) == NULL) {
		fprintf(stderr, "Error, could not realloc a new bloom filter\n");
		return NULL;
	}
	bloom->bitmap = NULL;
	bloom->capacity = capacity;
	bloom->error_rate = error_rate;
	bloom->offset = offset + sizeof(counting_bloom_header_t);
	bloom->nfuncs = (int) ceil(log(1 / error_rate) / log(2));
	bloom->counts_per_func = (int) ceil(capacity * fabs(log(error_rate)) /
			(bloom->nfuncs * pow(log(2), 2)));
	bloom->size = bloom->nfuncs * bloom->counts_per_func;
	/* rounding-up integer divide by 2 of bloom->size */
	bloom->num_bytes = ((bloom->size + 1) / 2) +
				sizeof(counting_bloom_header_t);
	bloom->hashes = calloc(bloom->nfuncs, sizeof(uint32_t));

	return bloom;
}

counting_bloom_t *new_counting_bloom(unsigned int capacity, double error_rate,
		const char *filename)
{
	counting_bloom_t *cur_bloom;
	int fd;

	if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC,
					(mode_t)0600)) < 0) {
		perror("Error, Opening File Failed");
		fprintf(stderr, " %s \n", filename);
		return NULL;
	}

	cur_bloom = counting_bloom_init(capacity, error_rate, 0);
	cur_bloom->bitmap = new_bitmap(fd, cur_bloom->num_bytes);
	cur_bloom->header = (counting_bloom_header_t *)
				(cur_bloom->bitmap->array);
	return cur_bloom;
}

int counting_bloom_add(counting_bloom_t *bloom, const char *s, size_t len)
{
	unsigned int index, i, offset;
	unsigned int *hashes = bloom->hashes;

	hash_func(bloom, s, len, hashes);

	for (i = 0; i < bloom->nfuncs; i++) {
		offset = i * bloom->counts_per_func;
		index = hashes[i] + offset;
		bitmap_increment(bloom->bitmap, index, bloom->offset);
	}
	bloom->header->count++;

	return 0;
}

int counting_bloom_remove(counting_bloom_t *bloom, const char *s, size_t len)
{
	unsigned int index, i, offset;
	unsigned int *hashes = bloom->hashes;

	hash_func(bloom, s, len, hashes);
	for (i = 0; i < bloom->nfuncs; i++) {
		offset = i * bloom->counts_per_func;
		index = hashes[i] + offset;
		bitmap_decrement(bloom->bitmap, index, bloom->offset);
	}
	bloom->header->count--;

	return 0;
}

int counting_bloom_check(counting_bloom_t *bloom, const char *s, size_t len)
{
	unsigned int index, i, offset;
	unsigned int *hashes = bloom->hashes;

	hash_func(bloom, s, len, hashes);
	
	for (i = 0; i < bloom->nfuncs; i++) {
		offset = i * bloom->counts_per_func;
		index = hashes[i] + offset;
		if (!(bitmap_check(bloom->bitmap, index, bloom->offset))) {
			return 0;
		}
	}

	return 1;
}

counting_bloom_t *new_counting_bloom_from_file(unsigned int capacity,
		double error_rate, const char *filename)
{
	int fd;
	off_t size;

	counting_bloom_t *bloom;

	if ((fd = open(filename, O_RDWR, (mode_t)0600)) < 0) {
		fprintf(stderr, "Error, Could not open file %s: %s\n",
				filename, strerror(errno));
		return NULL;
	}
	if ((size = lseek(fd, 0, SEEK_END)) < 0) {
		perror("Error, calling lseek() to tell file size");
		close(fd);
		return NULL;
	}
	if (size == 0) {
		fprintf(stderr, "Error, File size zero\n");
	}

	bloom = counting_bloom_init(capacity, error_rate, 0);

	if (size != bloom->num_bytes) {
		free_counting_bloom(bloom);
		fprintf(stderr, "Error, Actual filesize and expected filesize are not equal\n");
		return NULL;
	}
	if ((bloom->bitmap = new_bitmap(fd, size)) == NULL) {
		fprintf(stderr, "Error, Could not create bitmap with file\n");
		free_counting_bloom(bloom);
		return NULL;
	}

	bloom->header = (counting_bloom_header_t *)(bloom->bitmap->array);
	return bloom;
}
