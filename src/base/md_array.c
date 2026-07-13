#include "base/md_array.h"
#include "base/md_error.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void md_array_init(struct md_array *array, size_t elem_size)
{
	array->data = NULL;
	array->len = 0;
	array->cap = 0;
	array->elem_size = elem_size;
}

void md_array_cleanup(struct md_array *array)
{
	free(array->data);
	md_array_init(array, array->elem_size);
}

int md_array_reserve(struct md_array *array, size_t needed)
{
	void *next;
	size_t cap;

	if (needed <= array->cap)
		return MD_OK;
	if (array->elem_size == 0 || needed > SIZE_MAX / array->elem_size)
		return MD_ERR_NOMEM;

	cap = array->cap ? array->cap : 8u;
	while (cap < needed) {
		if (cap > SIZE_MAX / 2u)
			return MD_ERR_NOMEM;
		cap *= 2u;
	}

	next = realloc(array->data, cap * array->elem_size);
	if (!next)
		return MD_ERR_NOMEM;

	array->data = next;
	array->cap = cap;
	return MD_OK;
}

void *md_array_push(struct md_array *array)
{
	char *slot;

	if (md_array_reserve(array, array->len + 1u) != MD_OK)
		return NULL;

	slot = (char *)array->data + array->len * array->elem_size;
	memset(slot, 0, array->elem_size);
	array->len++;
	return slot;
}

void *md_array_get(struct md_array *array, size_t index)
{
	if (index >= array->len)
		return NULL;
	return (char *)array->data + index * array->elem_size;
}

void md_array_clear(struct md_array *array)
{
	array->len = 0;
}
