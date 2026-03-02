#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
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

uint8_t is_latin_square(latin_square P)
{
  uint8_t* seen = calloc(P.n, sizeof(uint8_t));

  // test the rows
  for (size_t i = 0; i < P.n; ++i)
  {
    memset(seen, 0, P.n * sizeof(*seen));
    for (size_t j = 0; j < P.n; ++j)
    {
      uint8_t P_ij = GET_AS_MAT(P.arr, i, j, P.n);
      if (P_ij >= P.n) return 0;
      seen[P_ij] = 1;
    }

    // there are exactely P.n spots in the row, for all the spots to be filled means no overlap happened
    for (size_t j = 0; j < P.n; ++j)
      if (!seen[j])
        return 0;
  }

  // test the cols
  for (size_t j = 0; j < P.n; ++j)
  {
    memset(seen, 0, P.n * sizeof(*seen));
    for (size_t i = 0; i < P.n; ++i)
    {
      uint8_t P_ij = GET_AS_MAT(P.arr, i, j, P.n);
      if (P_ij >= P.n) return 0;
      seen[P_ij] = 1;
    }

    // there are exactely P.n spots in the col, for all the spots to be filled means no overlap happened
    for (size_t i = 0; i < P.n; ++i)
      if (!seen[j])
        return 0;
  }

  return 1;
}


