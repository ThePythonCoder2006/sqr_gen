#include <stdio.h>
#include <stdlib.h>

#include "latin_squares.h"

void latin_square_init(latin_square *P, uint64_t n)
{
  P->n = n;
  P->arr = calloc(n * n, sizeof(*(P->arr)));
  P->cols = calloc(n, sizeof(*P->cols));
  P->rows = calloc(n, sizeof(*P->rows));
  if (P->arr == NULL || P->cols == NULL || P->rows == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  for (uint64_t i = 0; i < n; ++i)
  {
    P->cols[i] = i;
    P->rows[i] = i;
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
      GET_AS_MAT(P, i, j) = (j + i) % (P.n);
  return;
}