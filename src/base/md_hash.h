#ifndef MORPH_MD_HASH_H
#define MORPH_MD_HASH_H

#include <stdint.h>
#include <stddef.h>

uint64_t md_hash_fnv1a(const void *data, size_t len);
uint64_t md_hash_cstr(const char *text);

#endif
