#ifndef MORPH_MD_TABLE_LAYOUT_H
#define MORPH_MD_TABLE_LAYOUT_H

#include <stddef.h>

struct md_table_column_constraint {
	unsigned int min_width;
	unsigned int preferred_width;
};

int md_table_size_columns(
	const struct md_table_column_constraint *columns,
	size_t column_count,
	unsigned int available_width,
	unsigned int *widths);

#endif
