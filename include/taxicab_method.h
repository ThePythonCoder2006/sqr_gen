#ifndef __TAXICAB_METHOD__
#define __TAXICAB_METHOD__

#include "types.h"
#include "perf_counter.h"

#ifndef REQUIERED_SETS
  #define REQUIERED_SETS (1UL<<5)
#endif

void search_pow_m_sqr_from_taxicabs(perf_counter* perf, const char* const base_file_name, pow_m_sqr M, taxicab a, taxicab b, size_t requiered_sets);

#endif // __TAXICAB_METHOD__
