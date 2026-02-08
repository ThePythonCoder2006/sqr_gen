#ifndef __TAXICAB__
#define __TAXICAB__

#include <stdint.h>

#include "perf_counter.h"
#include "types.h"

#define TAXI_GET_AS_MAT(T, i, j) ((T).arr[(i) * ((T).s) + (j)])
#define TAXI_GET_AS_VEC(T, idx) ((T).arr[(idx)])

uint64_t taxicab_sum_row(taxicab T, uint64_t i);
uint8_t is_taxicab(taxicab T);
uint8_t taxicab_cross_products_are_distinct(taxicab a, taxicab b);
int taxicab_init(taxicab *T, uint64_t r, uint64_t s, uint64_t d);
void mvtaxicab_print(int y0, int x0, taxicab T);
void taxicab_printf(taxicab T);
void taxicab_clear(taxicab *T);
int search_taxicab(taxicab T, uint64_t X, uint64_t progress, uint8_t *heat_map, perf_counter *perf, uint8_t setup);

#endif // __TAXICAB__

