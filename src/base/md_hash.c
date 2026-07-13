#include "base/md_hash.h"

#include <string.h>

#define FNV_OFFSET 1469598103934665603ULL
#define FNV_PRIME 1099511628211ULL

uint64_t md_hash_fnv1a(const void *data, size_t len)
{
	const unsigned char *bytes;
	uint64_t hash;
	size_t i;

	bytes = data;
	hash = FNV_OFFSET;
	for (i = 0; i < len; i++) {
		hash ^= bytes[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

uint64_t md_hash_cstr(const char *text)
{
	if (!text)
		return FNV_OFFSET;
	return md_hash_fnv1a(text, strlen(text));
}
