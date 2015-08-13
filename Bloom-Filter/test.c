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
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include "bloom.h"

#define BILLION  1E9
#define CAPACITY 1100000
#define ERROR_RATE .01

enum {
	TEST_PASS,
	TEST_WARN,
	TEST_FAIL,
};

struct stats {
	int true_positives;
	int true_negatives;
	int false_positives;
	int false_negatives;
};

static void chomp_line(char *word)
{
	char *p;
	if ((p = strchr(word, '\r'))) {
		*p = '\0';
	}
	if ((p = strchr(word, '\n'))) {
		*p = '\0';
	}
}

static int print_results(struct stats *stats)
{
	float false_positive_rate = (float)stats->false_positives /
                        (stats->false_positives + stats->true_negatives);

	printf("True positives:     %7d"   "\n"
		"True negatives:     %7d"   "\n"
		"False positives:    %7d"   "\n"
		"False negatives:    %7d"   "\n"
		"False positive rate: %.4f" "\n",
		stats->true_positives,
		stats->true_negatives,
		stats->false_positives,
		stats->false_negatives,
		false_positive_rate);

	if (stats->false_negatives > 0) {
		printf("TEST FAIL (false negatives exist)\n");
		return TEST_FAIL;
	} else if (false_positive_rate > ERROR_RATE) {
		printf("TEST WARN (false positive rate too high)\n");
		return TEST_WARN;
	} else {
		printf("TEST PASS\n");
		return TEST_PASS;
	}
}

static void bloom_score(int positive, int should_positive, struct stats *stats,
		const char *key)
{
	if (should_positive) {
		if (positive) {
			stats->true_positives++;
		} else {
			stats->false_negatives++;
			fprintf(stderr, "ERROR: False negative: '%s'\n", key);
		}
	} else {
		if (positive)
			stats->false_positives++;
		else
			stats->true_negatives++;
	}
}

int test_counting_remove_reopen(const char *bloom_file, const char *words_file)
{
	FILE *fp;
	char word[256];
	counting_bloom_t *bloom;
	int i, key_removed;
	struct stats results = { 0 };

	printf("\n* test counting remove & reopen\n");

	if ((fp = fopen(bloom_file, "r"))) {
		fclose(fp);
		remove(bloom_file);
	}
	if (!(bloom = new_counting_bloom(CAPACITY, ERROR_RATE, bloom_file))) {
		fprintf(stderr, "ERROR: Could not create bloom filter\n");
		return TEST_FAIL;
	}
	if (!(fp = fopen(words_file, "r"))) {
		fprintf(stderr, "ERROR: Could not open words file\n");
		return TEST_FAIL;
	}

	for (i = 0; fgets(word, sizeof(word), fp) && (i < CAPACITY); i++) {
		chomp_line(word);
		counting_bloom_add(bloom, word, strlen(word));
	}

	fseek(fp, 0, SEEK_SET);
	for (i = 0; fgets(word, sizeof(word), fp) && (i < CAPACITY); i++) {
		if (i % 5 == 0) {
			chomp_line(word);
			counting_bloom_remove(bloom, word, strlen(word));
		}
	}

	free_counting_bloom(bloom);
	bloom = new_counting_bloom_from_file(CAPACITY, ERROR_RATE, bloom_file);

	fseek(fp, 0, SEEK_SET);
	for (i = 0; (fgets(word, sizeof(word), fp)) && (i < CAPACITY); i++) {
		chomp_line(word);
		key_removed = (i % 5 == 0);
		bloom_score(counting_bloom_check(bloom, word, strlen(word)),
				!key_removed, &results, word);
	}
	fclose(fp);

	printf("Elements added:   %6d" "\n"
		"Elements removed: %6d" "\n"
		"Total size: %d KiB"  "\n\n",
		i, i / 5,
		(int) bloom->num_bytes / 1024);

	free_counting_bloom(bloom);
	return print_results(&results);
}

int test_counting_accuracy(const char *bloom_file, const char *words_file)
{
	FILE *fp;
	char word[256];
	counting_bloom_t *bloom;
	int i;
	struct stats results = { 0 };

	printf("\n* test counting accuracy\n");

	if ((fp = fopen(bloom_file, "r"))) {
		fclose(fp);
		remove(bloom_file);
	}

	if (!(bloom = new_counting_bloom(CAPACITY, ERROR_RATE, bloom_file))) {
		fprintf(stderr, "ERROR: Could not create bloom filter\n");
		return TEST_FAIL;
	}
	if (!(fp = fopen(words_file, "r"))) {
		fprintf(stderr, "ERROR: Could not open words file\n");
		return TEST_FAIL;
	}

	for (i = 0; fgets(word, sizeof(word), fp) && (i < CAPACITY * 2); i++) {
		if (i % 2 == 0) {
			chomp_line(word);
			counting_bloom_add(bloom, word, strlen(word));
		}
	}

	fseek(fp, 0, SEEK_SET);
	for (i = 0; fgets(word, sizeof(word), fp) && (i < CAPACITY * 2); i++) {
		if (i % 2 == 1) {
			chomp_line(word);
			bloom_score(counting_bloom_check(bloom, word,
					strlen(word)), 0, &results, word);
		}
	}

	fclose(fp);
	
	printf("Elements added:   %6d" "\n"
		"Elements checked: %6d" "\n"
		"Total size: %d KiB"  "\n\n",
		(i + 1) / 2, i / 2,
		(int) bloom->num_bytes / 1024);

	free_counting_bloom(bloom);

	return print_results(&results);
}

int main(int argc, char *argv[])
{
	int i;
	int failures = 0, warnings = 0;
	struct timespec start, stop;
	unsigned long accum;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <bloom_file> <words_file>\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
		perror( "clock gettime" );
		exit( EXIT_FAILURE );
	}
	int (*tests[])(const char *, const char *) = {
		test_counting_remove_reopen,
		test_counting_accuracy,
		NULL,
	};

	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
		perror( "clock gettime" );
		exit( EXIT_FAILURE );
	}
	for (i = 0; tests[i] != NULL;  i++) {
		int result = (tests[i])(argv[1], argv[2]);
		if (result == TEST_FAIL) {
			failures++;
		} else if (result == TEST_WARN) {
			warnings++;
		}
	}

	accum = stop.tv_nsec - start.tv_nsec;
	if (accum < 0)
		accum = accum + 1000000000;
	printf("\nTime: %lu\n", accum);
	printf("\n** %d failures, %d warnings\n", failures, warnings);
	if (failures) {
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
