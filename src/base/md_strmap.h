#ifndef MORPH_MD_STRMAP_H
#define MORPH_MD_STRMAP_H

#include <stddef.h>

struct md_strmap_entry {
	char *key;
	void *value;
	unsigned char state;
};

struct md_strmap {
	struct md_strmap_entry *entries;
	size_t len;
	size_t cap;
};

void md_strmap_init(struct md_strmap *map);
void md_strmap_cleanup(struct md_strmap *map);
int md_strmap_set(struct md_strmap *map, const char *key, void *value);
void *md_strmap_get(const struct md_strmap *map, const char *key);
int md_strmap_contains(const struct md_strmap *map, const char *key);
void md_strmap_clear(struct md_strmap *map);

#endif
