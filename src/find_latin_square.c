#include <stdint.h>
#include <stdlib.h>

#include "find_latin_squares.h"
#include "latin_squares.h"
#include "pow_m_sqr.h"

typedef struct
{
  uint64_t n;
  uint8_t **usedInRow;
  uint8_t **usedInCol;
  // bool inited;
} state;

#define LATIN_SQUARE_UNSET UINT8_MAX

void init_arrays(state *s);
void free_arrays(state *s);
uint8_t iterate_over_all_square_callback_inside(state *s, latin_square *P, uint64_t row, uint64_t col, latin_square_callback callback, void *data);

// state s = {.inited = false};

// void print_latin_square(latin_square P)
// {
//   mvpow_m_sqr_printw(0, 0, P);
// }

/*
 * P is modified in place as well as being transmitted to callback
 * callback should return non-zero for the search to continue
 * returns non-zero if every latin square was exausted and zero if it stopped due to an interupt from callback
 */
uint8_t iterate_over_all_square_callback(latin_square *P, latin_square_callback callback, void *data)
{
  state s = {.n = P->n};
  init_arrays(&s);
  for (uint32_t idx = 0; idx < P->n * P->n; ++idx)
    M_SQR_GET_AS_VEC(*P, idx) = LATIN_SQUARE_UNSET;

  // Fix first row: {0, 1, 2, ..., n - 1}
  for (uint64_t col = 0; col < P->n; col++)
  {
    GET_AS_MAT(P->arr, 0, col, P->n) = col;
    s.usedInRow[0][col] = 1;
    s.usedInCol[col][col] = 1;
  }

  // start from second row, first column
  uint8_t ret = iterate_over_all_square_callback_inside(&s, P, 1, 0, callback, data);
  free_arrays(&s);
  return ret;
}

// Recursive backtracking function to fill the square
uint8_t iterate_over_all_square_callback_inside(state *s, latin_square *P, uint64_t row, uint64_t col, latin_square_callback callback, void *data)
{
  // normally : 0 <= row < n but here we overflowed ie the square is full
  if (row == P->n)
    return (*callback)(P, data);

  if (GET_AS_MAT(P->arr, row, col, P->n) != LATIN_SQUARE_UNSET)
  {
    // Move to next cell
    if (col == P->n - 1)
      return iterate_over_all_square_callback_inside(s, P, row + 1, 0, callback, data);
    else
      return iterate_over_all_square_callback_inside(s, P, row, col + 1, callback, data);
  }

  for (uint64_t num = 0; num < P->n; num++)
  {
    if (!(s->usedInRow[row][num]) && !(s->usedInCol[col][num]))
    {
      GET_AS_MAT(P->arr, row, col, P->n) = num;
      s->usedInRow[row][num] = 1;
      s->usedInCol[col][num] = 1;

      // Move to next cell
      uint8_t cont = (col == P->n - 1) ? iterate_over_all_square_callback_inside(s, P, row + 1, 0, callback, data)
                                       : iterate_over_all_square_callback_inside(s, P, row, col + 1, callback, data);

      // Backtrack
      GET_AS_MAT(P->arr, row, col, P->n) = LATIN_SQUARE_UNSET;
      s->usedInRow[row][num] = 0;
      s->usedInCol[col][num] = 0;

      if (!cont)
        return 0;
    }
  }

  return 1;
}

typedef struct
{
  latin_square *base;
  uint64_t len;
  latin_square_array_callback f;
  void *data;
} square_array_pack;

uint8_t iterate_over_all_square_array_callback_inside(latin_square *P, void *data)
{
  square_array_pack *pack = (square_array_pack *)data;
  latin_square *base = pack->base;
  /*
   * P - base == 0 on the first step
   * hence the counting is done zero based ie we need to go only until P - base == len - 1
   */
  if ((uint64_t)(P - base) == pack->len - 1)
    return (pack->f)(base, pack->len, pack->data);

  // we do not need to handle stop here as the external function, which is not recursive, will just return once the search on the first element ended
  return iterate_over_all_square_callback(P + 1, iterate_over_all_square_array_callback_inside, data);
}

uint8_t iterate_over_all_square_array_callback(latin_square *P, uint64_t len, latin_square_array_callback f, void *data)
{
  square_array_pack pack = {.base = P, .len = len, .f = f, .data = data};
  return iterate_over_all_square_callback(P, iterate_over_all_square_array_callback_inside, &pack);
}

void init_arrays(state *s)
{
  // Allocate usedInRow
  s->usedInRow = malloc(s->n * sizeof(bool *));
  for (uint64_t i = 0; i < s->n; i++)
  {
    s->usedInRow[i] = calloc(s->n, sizeof(bool));
  }

  // Allocate usedInCol
  s->usedInCol = malloc(s->n * sizeof(bool *));
  for (uint64_t i = 0; i < s->n; i++)
  {
    s->usedInCol[i] = calloc(s->n, sizeof(bool));
  }
}

void free_arrays(state *s)
{
  for (uint64_t i = 0; i < s->n; i++)
  {
    free(s->usedInRow[i]);
    free(s->usedInCol[i]);
  }
  free(s->usedInRow);
  free(s->usedInCol);
}

#if 0
int main()
{
  printf("Enter size of Latin square (n): ");
  int size;
  scanf("%d", &size);
  if (size < 1 || size > 12)
  {
    printf("Please use a reasonable size (1-12)\n");
    return 1;
  }

  init_arrays(size);
  latin_square P = {0};
  latin_square_init(&P, size);

  // Fix first row: {1, 2, ..., n}
  for (uint64_t col = 0; col < P.n; col++)
  {
    M_SQR_GET_AS_MAT(P, 0, col) = col + 1;
    usedInRow[0][col + 1] = true;
    usedInCol[col][col + 1] = true;
  }

  fillCell(P, 1, 0, print_latin_square); // Start from second row, first column

  latin_square_clear(&P);
  free_arrays(P.n);
  return 0;
}

#endif
