#ifndef MORPH_MD_ARRAY_H
#define MORPH_MD_ARRAY_H

#include <stddef.h>

struct md_array {
	void *data;
	size_t len;
	size_t cap;
	size_t elem_size;
};

void md_array_init(struct md_array *array, size_t elem_size);
void md_array_cleanup(struct md_array *array);
int md_array_reserve(struct md_array *array, size_t needed);
void *md_array_push(struct md_array *array);
void *md_array_get(struct md_array *array, size_t index);
void md_array_clear(struct md_array *array);

#endif
