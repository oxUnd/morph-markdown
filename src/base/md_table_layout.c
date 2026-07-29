#include "base/md_table_layout.h"
#include "base/md_error.h"

#include <stdint.h>
#include <stdlib.h>

struct width_share {
	size_t column;
	uint64_t remainder;
};

static int share_before(const void *left, const void *right)
{
	const struct width_share *a = left;
	const struct width_share *b = right;

	if (a->remainder > b->remainder)
		return -1;
	if (a->remainder < b->remainder)
		return 1;
	if (a->column < b->column)
		return -1;
	if (a->column > b->column)
		return 1;
	return 0;
}

static unsigned int column_preferred(
	const struct md_table_column_constraint *column)
{
	return column->preferred_width > column->min_width ?
		column->preferred_width : column->min_width;
}

static uint64_t preferred_sum(
	const struct md_table_column_constraint *columns,
	size_t column_count)
{
	uint64_t total = 0u;
	size_t i;

	for (i = 0u; i < column_count; i++)
		total += column_preferred(&columns[i]);
	return total;
}

static uint64_t minimum_sum(
	const struct md_table_column_constraint *columns,
	size_t column_count)
{
	uint64_t total = 0u;
	size_t i;

	for (i = 0u; i < column_count; i++)
		total += columns[i].min_width;
	return total;
}

static void copy_widths(
	const struct md_table_column_constraint *columns,
	size_t column_count, int preferred, unsigned int *widths)
{
	size_t i;

	for (i = 0u; i < column_count; i++) {
		widths[i] = preferred ? column_preferred(&columns[i]) :
			columns[i].min_width;
	}
}

static int distribute_flexible_width(
	const struct md_table_column_constraint *columns,
	size_t column_count, uint64_t extra, uint64_t flex_sum,
	unsigned int *widths)
{
	struct width_share *shares;
	uint64_t assigned = 0u;
	uint64_t flex;
	uint64_t amount;
	size_t i;

	shares = calloc(column_count ? column_count : 1u, sizeof(*shares));
	if (!shares)
		return MD_ERR_NOMEM;
	for (i = 0u; i < column_count; i++) {
		flex = column_preferred(&columns[i]) - columns[i].min_width;
		amount = flex_sum ? extra * flex / flex_sum : 0u;
		widths[i] = columns[i].min_width + (unsigned int)amount;
		assigned += amount;
		shares[i].column = i;
		shares[i].remainder = flex_sum ?
			(extra * flex) % flex_sum : 0u;
	}
	qsort(shares, column_count, sizeof(*shares), share_before);
	for (i = 0u; i < (size_t)(extra - assigned); i++)
		widths[shares[i].column]++;
	free(shares);
	return MD_OK;
}

int md_table_size_columns(
	const struct md_table_column_constraint *columns,
	size_t column_count,
	unsigned int available_width,
	unsigned int *widths)
{
	uint64_t min_sum;
	uint64_t pref_sum;
	uint64_t flex_sum;

	if (column_count == 0u)
		return MD_OK;
	if (!columns || !widths)
		return MD_ERR_INVALID;
	min_sum = minimum_sum(columns, column_count);
	pref_sum = preferred_sum(columns, column_count);
	if (pref_sum <= available_width) {
		copy_widths(columns, column_count, 1, widths);
		return MD_OK;
	}
	if (min_sum >= available_width) {
		copy_widths(columns, column_count, 0, widths);
		return MD_OK;
	}
	flex_sum = pref_sum - min_sum;
	return distribute_flexible_width(
		columns, column_count, available_width - min_sum,
		flex_sum, widths);
}
