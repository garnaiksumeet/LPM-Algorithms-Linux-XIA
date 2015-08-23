/*
 * Original Code by Armon Dadgar at github.com/armon/statsite. See the LICENSE
 * in the repository for code usage and distribution.
 *
 * 2015, LPM Algorithms for Linux-XIA
 * Garnaik Sumeet, Michel Machado
 */
#ifndef HASHMAP_H
#define HASHMAP_H


// Basic hash entry.
struct hashmap_entry {
	char *key;
	unsigned int nexthop;
	struct hashmap_entry *next;
};

struct hashmap {
	int count;      // Number of entries
	int table_size; // Size of table in nodes
	int max_size;   // Max size before we resize
	struct hashmap_entry *table; // Pointer to an arry of hashmap_entry
};

/*
 * Creates a new hashmap and allocates space for it.
 * initial_size The minimim initial size.
 * map Output. Set to the address of the map
 * return 0 on success.
 */
int hashmap_init(int initial_size, struct hashmap **map);

/*
 * Destroys a map and cleans up all associated memory
 * map The hashmap to destroy. Frees memory.
 */
int hashmap_destroy(struct hashmap *map);

/*
 * Returns the size of the hashmap in items
 */
int hashmap_size(struct hashmap *map);

/*
 * Gets a value.
 * key The key to look for. Must be null terminated.
 * nexthop Output. Set to the value of the key.
 * 0 on success. -1 if not found.
 */
int hashmap_get(struct hashmap *map, const char *key, uint64_t out,
		unsigned int *nexthop);

/*
 * Puts a key/value pair.
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
		uint64_t out);

/*
 * Deletes a key/value pair.
 *
 * This method is not thread safe.
 *
 * key The key to delete
 * out The lower 64-bits of hash
 * 0 on success. -1 if not found.
 */
int hashmap_delete(struct hashmap *map, const char *key, uint64_t out);

/*
 * Clears all the key/value pairs.
 *
 * This method is not thread safe.
 *
 * map The hashmap
 * 0 on success. -1 if not found.
 */
int hashmap_clear(struct hashmap *map);

#endif
