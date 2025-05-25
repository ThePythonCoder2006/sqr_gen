/*
 * size : n x n = (r*s) x (r*s)
 * block size: r x s
 *                         r blocks
 *                s cols s cols   ...   s cols
 *               |------|------|- ... -|------|
 *           r   |      |      |       |      |
 *          rows |      |      |       |      | sum = r
 *               |------|------|- ... -|------|
 *   s       .   .      .      .       .      .    .
 * blocks    .   .      .      .       .      .    .
 *           .   .      .      .       .      .    .
 *               |------|------|- ... -|------|
 *           r   |      |      |       |      | sum = r
 *          rows |      |      |       |      |
 *               |------|------|- ... -|------|
 *                sum= s sum= s   ...   sum= s
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "find_sets.h"

uint8_t find_sets_print_selection(uint8_t *selected, uint32_t n, void *_);
uint8_t iterate_over_sets_callback_inside(sets_search_data *d, uint32_t current_row, uint32_t current_col, uint32_t count, set_callback f, void *data);

int test_find_sets(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <r> <s>\n", argv[0]);
    return 1;
  }
  uint32_t r = atoi(argv[1]);
  uint32_t s = atoi(argv[2]);

  iterate_over_sets_callback(r, s, find_sets_print_selection, NULL);

  return 0;
}

uint8_t find_sets_print_selection(uint8_t *selected, uint32_t n, void *_)
{
  (void)_;

  for (uint32_t i = 0; i < n; ++i)
  {
    for (uint32_t j = 0; j < n; ++j)
      printf(GET_AS_MAT(selected, i, j, n) ? "X " : ". ");
    printf("\n");
  }
  printf("---\n");
  return 1;
}

void iterate_over_sets_callback(uint32_t r, uint32_t s, set_callback f, void *data)
{
  uint32_t n = r * s;
  sets_search_data d = {.r = r, .s = s, .n = n};

  /*
   * There are r block columns of width s
   * for all j in {0; ...; r}
   * sum_col[j] = the numbers of entries selected in the block column j
   */
  d.sum_col = calloc(r, sizeof(uint32_t));
  /*
   * There are s block rows of height r
   * for all i in {0; ...; s}
   * sum_row[i] = the numbers of entries selected in the block row i
   */
  d.sum_row = calloc(r, sizeof(uint32_t));

  d.selected = calloc(n * n, sizeof(uint8_t));

  iterate_over_sets_callback_inside(&d, 0, 0, 0, f, data);

  free(d.sum_col);
  free(d.sum_row);
  free(d.selected);

  return;
}

uint8_t iterate_over_sets_callback_inside(sets_search_data *d, uint32_t current_row, uint32_t current_col, uint32_t count, set_callback f, void *data)
{
  // base case
  if (count == d->n)
  {
    return (*f)(d->selected, d->n, data);
  }

  if (current_row == d->n && current_row == d->n)
    return 1;
  if (current_col == d->n)
    return iterate_over_sets_callback_inside(d, current_row + 1, 0, count, f, data);

  // find current block: there are s block rows and r block rows
  uint32_t block_row = current_row / d->r; // 0 <= current_row <= r*s - 1 then 0 <= block_row <= s - 1
  uint32_t block_col = current_col / d->s; // 0 <= current_col <= r*s - 1 then 0 <= block_col <= r - 1

  /*
   * 2 different cases:
   * - 1st if allowed select the current cell and continue
   * - 2nd do not select the current cell and continue
   */

  if (!GET_AS_MAT(d->selected, current_row, current_col, d->n) && d->sum_row[block_row] < d->r && d->sum_col[block_col] < d->s)
  {
    // add the current cell to the list and update counts
    GET_AS_MAT(d->selected, current_row, current_col, d->n) = 1;
    ++(d->sum_row[block_row]);
    ++(d->sum_col[block_col]);

    if (!iterate_over_sets_callback_inside(d, current_row, current_col + 1, count + 1, f, data))
      return 1;

    // roll back the changes to remove the cell
    GET_AS_MAT(d->selected, current_row, current_col, d->n) = 0;
    --(d->sum_row[block_row]);
    --(d->sum_col[block_col]);
  }

  return iterate_over_sets_callback_inside(d, current_row, current_col + 1, count, f, data);
}