#include "base/md_strmap.h"
#include "base/md_error.h"
#include "base/md_hash.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MD_STRMAP_EMPTY 0u
#define MD_STRMAP_USED 1u

static char *md_strdup(const char *text)
{
	char *out;
	size_t len;

	len = strlen(text);
	out = malloc(len + 1u);
	if (!out)
		return NULL;
	memcpy(out, text, len + 1u);
	return out;
}

void md_strmap_init(struct md_strmap *map)
{
	map->entries = NULL;
	map->len = 0;
	map->cap = 0;
}

static void md_strmap_free_entries(struct md_strmap *map)
{
	size_t i;

	for (i = 0; i < map->cap; i++)
		free(map->entries[i].key);
	free(map->entries);
}

void md_strmap_cleanup(struct md_strmap *map)
{
	md_strmap_free_entries(map);
	md_strmap_init(map);
}

static size_t find_slot(const struct md_strmap *map, const char *key, int *found)
{
	size_t mask;
	size_t pos;

	*found = 0;
	if (map->cap == 0)
		return 0;

	mask = map->cap - 1u;
	pos = (size_t)md_hash_cstr(key) & mask;
	while (map->entries[pos].state == MD_STRMAP_USED) {
		if (strcmp(map->entries[pos].key, key) == 0) {
			*found = 1;
			return pos;
		}
		pos = (pos + 1u) & mask;
	}
	return pos;
}

static int md_strmap_rehash(struct md_strmap *map, size_t cap)
{
	struct md_strmap next;
	struct md_strmap_entry *entry;
	size_t i;
	int rc;

	md_strmap_init(&next);
	next.entries = calloc(cap, sizeof(*next.entries));
	if (!next.entries)
		return MD_ERR_NOMEM;
	next.cap = cap;

	for (i = 0; i < map->cap; i++) {
		entry = &map->entries[i];
		if (entry->state != MD_STRMAP_USED)
			continue;
		rc = md_strmap_set(&next, entry->key, entry->value);
		if (rc != MD_OK) {
			md_strmap_cleanup(&next);
			return rc;
		}
	}

	md_strmap_free_entries(map);
	*map = next;
	return MD_OK;
}

static int ensure_capacity(struct md_strmap *map)
{
	size_t next_cap;

	if (map->cap != 0 && (map->len + 1u) * 4u < map->cap * 3u)
		return MD_OK;

	next_cap = map->cap ? map->cap * 2u : 16u;
	return md_strmap_rehash(map, next_cap);
}

int md_strmap_set(struct md_strmap *map, const char *key, void *value)
{
	struct md_strmap_entry *entry;
	size_t pos;
	int found;
	int rc;

	if (!map || !key)
		return MD_ERR_INVALID;

	rc = ensure_capacity(map);
	if (rc != MD_OK)
		return rc;

	pos = find_slot(map, key, &found);
	entry = &map->entries[pos];
	if (!found) {
		entry->key = md_strdup(key);
		if (!entry->key)
			return MD_ERR_NOMEM;
		entry->state = MD_STRMAP_USED;
		map->len++;
	}
	entry->value = value;
	return MD_OK;
}

void *md_strmap_get(const struct md_strmap *map, const char *key)
{
	size_t pos;
	int found;

	if (!map || !key || map->cap == 0)
		return NULL;

	pos = find_slot(map, key, &found);
	return found ? map->entries[pos].value : NULL;
}

int md_strmap_contains(const struct md_strmap *map, const char *key)
{
	size_t pos;
	int found;

	if (!map || !key || map->cap == 0)
		return 0;

	pos = find_slot(map, key, &found);
	(void)pos;
	return found;
}

void md_strmap_clear(struct md_strmap *map)
{
	size_t i;

	for (i = 0; i < map->cap; i++) {
		free(map->entries[i].key);
		map->entries[i].key = NULL;
		map->entries[i].value = NULL;
		map->entries[i].state = MD_STRMAP_EMPTY;
	}
	map->len = 0;
}
