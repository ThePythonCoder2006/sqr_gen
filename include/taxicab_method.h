#ifndef __TAXICAB_METHOD__
#define __TAXICAB_METHOD__

#include "types.h"

#ifndef REQUIERED_SETS
  #define REQUIERED_SETS (1ULL<<5)
#endif

void search_pow_m_sqr_from_taxicabs(pow_m_sqr M, taxicab a, taxicab b, size_t requiered_sets);

#endif // __TAXICAB_METHOD__
