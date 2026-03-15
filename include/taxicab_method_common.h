#ifndef __TAXICAB_METHOD_COMMON__
#define __TAXICAB_METHOD_COMMON__

#include <stdio.h>

#include "types.h"
#include "perf_counter.h"

typedef struct pow_m_sqr_and_da_sets_packed_s
{
  pow_m_sqr *M;
  da_sets *rels;
  size_t requiered_sets;
} pow_m_sqr_and_da_sets_packed;

typedef struct
{
  /*
   * P = array of size r of latin_squares of side s
   * Q = array of size s of latin_squares of side r
   */
  latin_square *P, *Q;
  pow_m_sqr *M;
  uint32_t r, s;
  da_sets rels, mark;
  perf_counter* perf;
  x_y_rel rel1, rel2, inv, sigma;
  uint8_t* rows, *cols;
  uint16_t refresh_frame;

  FILE* f;
} iterate_over_latin_squares_array_pack;

uint8_t search_pow_m_sqr_from_taxicab_iterate_over_sets_callback(uint8_t *selected, uint32_t n, void *data);
uint8_t search_pow_m_sqr_from_taxicab_find_sets_collision_callback(uint8_t *selected, uint32_t n, void *data);

#endif // __TAXICAB_METHOD_COMMON__
