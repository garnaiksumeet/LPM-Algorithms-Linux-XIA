/*
 * Original Code by Armon Dadgar at github.com/armon/statsite. See the LICENSE
 * in the repository for code usage and distribution.
 *
 * 2015, LPM Algorithms for Linux-XIA
 * Garnaik Sumeet, Michel Machado
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "hashmap.h"
#include "murmur.h"

#define MAX_CAPACITY 0.75
#define DEFAULT_CAPACITY 128
#define SALT_CONSTANT 0x97c29b3a

/**
 * Creates a new hashmap and allocates space for it.
 * initial_size The minimim initial size. 0 for default (64).
 * map Output. Set to the address of the map
 * return 0 on success.
 */
int hashmap_init(int initial_size, struct hashmap **map)
{
	int i;
	if (initial_size <= 0) {
		initial_size = DEFAULT_CAPACITY;
	} else {
		int msb = 0;
		for (i = 0; i < sizeof(initial_size) * 8; i++) {
			if ((initial_size >> i) & 0x1)
				msb = i;
		}
		if ((1 << msb) != initial_size)
			msb += 1;
		initial_size = 1 << msb;
	}

	struct hashmap *m = calloc(1, sizeof(struct hashmap));
	m->table_size = initial_size;
	m->max_size = MAX_CAPACITY * initial_size;
	m->table = (struct hashmap_entry *) calloc(initial_size,
			sizeof(struct hashmap_entry));

	*map = m;
	return 0;
}

/*
 * Destroys a map and cleans up all associated memory
 * arg map The hashmap to destroy. Frees memory.
 */
int hashmap_destroy(struct hashmap *map)
{
	struct hashmap_entry *entry, *old;
	int in_table, i;

	for (i = 0; i < map->table_size; i++) {
		entry = map->table + i;
		in_table = 1;
		while (entry && entry->key) {
			old = entry;
			entry = entry->next;
			free(old->key);
			if (!in_table) {
				free(old);
			}
			in_table = 0;
		}
	}
	free(map->table);
	free(map);
	return 0;
}

/*
 * Returns the size of the hashmap in items
 */
int hashmap_size(struct hashmap *map)
{
	return map->count;
}

/*
 * Gets a value.
 * key The key to look for
 * key_len The key length
 * out Lower 64-bits of the hash of key
 * nexthop Output. Set to the value of the key.
 * 0 on success. -1 if not found.
 */
int hashmap_get(struct hashmap *map, const char *key, uint64_t out,
		unsigned int *nexthop)
{
	unsigned int index = out % map->table_size;
	struct hashmap_entry *entry = map->table + index;

	while (entry && entry->key) {
		if (strcmp(entry->key, key) == 0) {
			*nexthop = entry->nexthop;
			return 0;
		}
		entry = entry->next;
	}

	return -1;
}

/*
 * Internal method to insert into a hash table
 * table The table to insert into
 * table_size The size of the table
 * key The key to insert
 * key_len The length of the key
 * value The value to associate
 * should_cmp Should keys be compared to existing ones.
 * should_dup Should duplicate keys
 * nexthop The nexthop address
 * out The lower 64-bit is the hash returned from murmurhash
 * return 1 if the key is new, 0 if updated.
 */
static int hashmap_insert_table(struct hashmap_entry *table, int table_size,
			const char *key, int key_len, unsigned int nexthop,
			int should_cmp, int should_dup, uint64_t out)
{
	// Mod the lower 64bits of the hash function with the table
	// size to get the index
	unsigned int index = out % table_size;

	struct hashmap_entry *entry = table + index;
	struct hashmap_entry *last_entry = NULL;

	while (entry && entry->key) {
		if (should_cmp && (strcmp(entry->key, key) == 0)) {
			entry->nexthop = nexthop;
			return 0;
		}
		last_entry = entry;
		entry = entry->next;
	}

	if (last_entry == NULL) {
		entry->key = (should_dup) ? strdup(key) : (char *) key;
		entry->nexthop = nexthop;
	} else {
		entry = calloc(1, sizeof(struct hashmap_entry));
		entry->key = (should_dup) ? strdup(key) : (char *) key;
		entry->nexthop = nexthop;
		last_entry->next = entry;
	}
	return 1;
}


/*
 * Internal method to double the size of a hashmap
 */
static void hashmap_double_size(struct hashmap *map)
{
	uint64_t out[2];
	int new_size = map->table_size * 2;
	int new_max_size = map->max_size * 2;

	struct hashmap_entry *new_table = (struct hashmap_entry *)
				calloc(new_size, sizeof(struct hashmap_entry));
	struct hashmap_entry *entry, *old;
	int in_table;
	int i;
	for (i=0; i < map->table_size; i++) {
		entry = map->table + i;
		in_table = 1;
		while (entry && entry->key) {
			old = entry;
			entry = entry->next;
			// Insert the value in the new map
			// Do not compare keys or duplicate since we are just doubling our
			// size, and we have unique keys and duplicates already.
			MurmurHash3_x64_128(old->key, strlen(old->key),
					SALT_CONSTANT, out);
			hashmap_insert_table(new_table, new_size, old->key, strlen(old->key),
					old->nexthop, 0, 0, out[1]);
			if (!in_table) {
				free(old);
			}
			in_table = 0;
		}
	}
	free(map->table);
	map->table = new_table;
	map->table_size = new_size;
	map->max_size = new_max_size;
}

/*
 * Puts a key/value pair. Replaces existing values.
 * key The key to set. This is copied, and a seperate
 * version is owned by the hashmap. The caller the key at will.
 * 
 * This method is not thread safe.
 *
 * out The lower 64-bits of hash
 * nexthop The value to set.
 * 0 if updated, 1 if added.
 */
int hashmap_put(struct hashmap *map, const char *key, unsigned int nexthop,
		uint64_t out)
{
	if (map->count + 1 > map->max_size) {
		hashmap_double_size(map);
		return hashmap_put(map, key, nexthop, out);
	}

	int new = hashmap_insert_table(map->table, map->table_size, key,
			strlen(key), nexthop, 1, 1, out);
	if (new)
		map->count += 1;

	return new;
}

/*
 * Deletes a key/value pair.
 *
 * This method is not thread safe.
 *
 * key The key to delete
 * out The lower 64-bits of hash
 * 0 on success. -1 if not found.
 */
int hashmap_delete(struct hashmap *map, const char *key, uint64_t out)
{
	unsigned int index = out % map->table_size;
	struct hashmap_entry *entry = map->table + index;
	struct hashmap_entry *last_entry = NULL;

	while (entry && entry->key) {
		if (strcmp(entry->key, key) == 0) {
			free(entry->key);
			map->count -= 1;
			if (last_entry == NULL) {
				if (entry->next) {
					struct hashmap_entry *n = entry->next;
					entry->key = n->key;
					entry->nexthop = n->nexthop;
					entry->next = n->next;
					free(n);
				} else {
					entry->key = NULL;
					entry->nexthop = 0;
				}
			} else {
				last_entry->next = entry->next;
				free(entry);
			}
			return 0;
		}
		entry = entry->next;
	}

	return -1;
}

/*
 * Clears all the key/value pairs.
 *
 * This method is not thread safe.
 *
 * 0 on success. -1 if not found.
 */
int hashmap_clear(struct hashmap *map)
{
	struct hashmap_entry *entry, *old;
	int in_table;
	int i;
	for (i=0; i < map->table_size; i++) {
		entry = map->table+i;
		in_table = 1;
		while (entry && entry->key) {
			old = entry;
			entry = entry->next;
			free(old->key);
			if (!in_table) {
				free(old);
			} else {
				old->key = NULL;
			}
			in_table = 0;
		}
	}
	map->count = 0;

	return 0;
}
