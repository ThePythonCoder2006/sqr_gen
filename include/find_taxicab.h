#ifndef __FIND_TAXICAB__
#define __FIND_TAXICAB__

#include "types.h"

void find_taxicab(taxicab T);
int find_taxicabs_condition(taxicab a, taxicab b, double p, uint64_t sum);
int taxicab_reduce(taxicab T);

#endif // __FIND_TAXICAB__
