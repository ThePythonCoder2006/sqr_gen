#ifndef __TAXICAB__
#define __TAXICAB__

#include <stdint.h>

#include "perf_counter.h"

/*
 * represents a (r, s, d)-taxicab
 * ie   a_{1, 1}^d + a_{1, 2}^d + ... + a_{1, s}^d
 *    = a_{2, 1}^d + a_{2, 2}^d + ... + a_{2, s}^d
 *    ...
 *    = a_{r, 1}^d + a_{r, 2}^d + ... + a_{r, s}^d
 * where a_{i, j} = arr[i * s + j]
 * r is the HEIGHT ie number of representations
 * s is the WIDTH ie the number of terms in each representation
 */
typedef struct
{
  uint64_t d, r, s;
  uint64_t *arr;
} taxicab;

#define TAXI_GET_AS_MAT(T, i, j) ((T).arr[(i) * ((T).s) + (j)])
#define TAXI_GET_AS_VEC(T, idx) ((T).arr[(idx)])

uint8_t is_taxicab(taxicab T);
uint8_t taxicab_cross_products_are_distinct(taxicab a, taxicab b);
int taxicab_init(taxicab *T, uint64_t r, uint64_t s, uint64_t d);
void mvtaxicab_print(int y0, int x0, taxicab T);
void taxicab_clear(taxicab *T);
int search_taxicab(taxicab T, uint64_t X, uint64_t progress, uint8_t *heat_map, perf_counter *perf, uint8_t setup);

#endif