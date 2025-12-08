#ifndef __FIND_SETS__
#define __FIND_SETS__

#include <stdint.h>
#include <stdlib.h>

#include "pow_m_sqr.h"

/*
 * first argument contains array of bool accessible by GET_AS_MAT indicating where the selected entries are.
 * second argument is the width of the square
 * third argument contains data passed on by caller to iterate_over_sets. Is allowed to be NULL, if callback doesn't use it. Will not be modified/read to by iterate_over_sets
 * should return non-zero for the iteration to continue
 */
typedef uint8_t (*set_callback)(uint8_t *, uint32_t, void *);

typedef struct sets_search_data_s
{
  uint8_t *selected;
  uint32_t *sum_col;
  uint32_t *sum_row;
  uint32_t r, s, n;
} sets_search_data;

void iterate_over_sets_callback(uint32_t r, uint32_t s, set_callback f, void *data);
uint8_t find_sets_print_selection(uint8_t *selected, uint32_t n, void *_);
uint8_t set_has_magic_sum(const uint8_t *selected, const pow_m_sqr M);
void find_sets_collision_method(pow_m_sqr M, const uint32_t r, const uint32_t s, set_callback f, void *data);

#endif // __FIND_SETS__