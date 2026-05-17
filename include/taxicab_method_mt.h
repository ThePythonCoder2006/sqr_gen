#ifndef __TAXICAB_METHOD_MT__
#define __TAXICAB_METHOD_MT__

#include "types.h"
#include "perf_counter.h"

#ifndef REQUIERED_SETS
  #define REQUIERED_SETS (1ULL<<5)
#endif

#ifndef DEFAULT_MAX_THREADS
  #define DEFAULT_MAX_THREADS 4
#endif

void search_pow_m_sqr_from_taxicabs_mt(perf_counter* perf, const char* const base_file_name, pow_m_sqr M, taxicab a, taxicab b, size_t requiered_sets, size_t thread_count);

#endif // __TAXICAB_METHOD_MT__
