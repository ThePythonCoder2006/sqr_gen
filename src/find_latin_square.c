#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "find_latin_squares.h"
#include "latin_squares.h"
#include "pow_m_sqr.h"

typedef struct
{
  uint64_t n;
  bool **usedInRow;
  bool **usedInCol;
  // bool inited;
} state;

void init_arrays(state *s);
void free_arrays(state *s);
bool iterate_over_all_square_callback_inside(state *s, latin_square P, uint64_t row, uint64_t col, latin_square_callback callback);

// state s = {.inited = false};

// void print_latin_square(latin_square P)
// {
//   mvpow_m_sqr_print(0, 0, P);
// }

bool iterate_over_all_square_callback(latin_square P, latin_square_callback callback)
{
  state s = {.n = P.n};
  init_arrays(&s);
  bool ret = iterate_over_all_square_callback_inside(&s, P, 0, 0, callback);
  free_arrays(&s);
  return ret;
}

// Recursive backtracking function to fill the square
bool iterate_over_all_square_callback_inside(state *s, latin_square P, uint64_t row, uint64_t col, latin_square_callback callback)
{
  if (row == P.n)
  {
    (*callback)(P);
    return false; // continue searching all solutions
  }

  if (M_SQR_GET_AS_MAT(P, row, col) != 0)
  {
    // Move to next cell
    if (col == P.n - 1)
      return iterate_over_all_square_callback_inside(s, P, row + 1, 0, callback);
    else
      return iterate_over_all_square_callback_inside(s, P, row, col + 1, callback);
  }

  for (uint64_t num = 1; num <= P.n; num++)
  {
    if (!(s->usedInRow[row][num]) && !(s->usedInCol[col][num]))
    {
      M_SQR_GET_AS_MAT(P, row, col) = num;
      s->usedInRow[row][num] = true;
      s->usedInCol[col][num] = true;

      // Move to next cell
      bool stop = (col == P.n - 1) ? iterate_over_all_square_callback_inside(s, P, row + 1, 0, callback)
                                   : iterate_over_all_square_callback_inside(s, P, row, col + 1, callback);

      // Backtrack
      M_SQR_GET_AS_MAT(P, row, col) = 0;
      s->usedInRow[row][num] = false;
      s->usedInCol[col][num] = false;

      if (stop)
        return true;
    }
  }

  return false;
}

void init_arrays(state *s)
{
  // Allocate usedInRow
  s->usedInRow = malloc(s->n * sizeof(bool *));
  for (uint64_t i = 0; i < s->n; i++)
  {
    s->usedInRow[i] = calloc(s->n + 1, sizeof(bool));
  }

  // Allocate usedInCol
  s->usedInCol = malloc(s->n * sizeof(bool *));
  for (uint64_t i = 0; i < s->n; i++)
  {
    s->usedInCol[i] = calloc(s->n + 1, sizeof(bool));
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
