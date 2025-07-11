#include <stdio.h>
#include <stdlib.h>

#include "latin_squares.h"
#include "pow_m_sqr.h"

void latin_square_init(latin_square *P, uint64_t n)
{
  P->n = n;
  P->arr = calloc(n * n, sizeof(*(P->arr)));
  if (P->arr == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  return;
}

void latin_square_clear(latin_square *P)
{
  free(P->arr);
  P->arr = NULL;
  return;
}

/*
 * 0 | 1 | 2 | 3
 * 1 | 2 | 3 | 0
 * 2 | 3 | 0 | 1
 * 3 | 0 | 1 | 2
 */
void standart_latin_square(latin_square P)
{
  for (uint64_t i = 0; i < P.n; ++i)
    for (uint64_t j = 0; j < P.n; ++j)
      GET_AS_MAT(P.arr, i, j, P.n) = (j + i) % (P.n);
  return;
}