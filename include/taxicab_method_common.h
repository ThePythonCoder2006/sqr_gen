#ifndef __TAXICAB_METHOD_COMMON__
#define __TAXICAB_METHOD_COMMON__

#include "types.h"

typedef struct pow_m_sqr_and_da_sets_packed_s
{
  pow_m_sqr *M;
  da_sets *rels;
  size_t requiered_sets;
} pow_m_sqr_and_da_sets_packed;

uint8_t search_pow_m_sqr_from_taxicab_iterate_over_sets_callback(uint8_t *selected, uint32_t n, void *data);
uint8_t search_pow_m_sqr_from_taxicab_find_sets_collision_callback(uint8_t *selected, uint32_t n, void *data);

#endif // __TAXICAB_METHOD_COMMON__
