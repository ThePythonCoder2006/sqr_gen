#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <ncurses.h>
#include "nob.h"

#include "find_latin_squares.h"
#include "pow_m_sqr.h"
#include "serialize.h"

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

/*
 * returns 1 upon early breaking
 */
uint8_t action_on_all_latin_square_arrays(const char*const base_file_name, const char*const name, perf_counter* perf, action func, void* data)
{
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  uint8_t ret = 0;
  size_t count;
  uint32_t r, s;
  fread(&count, sizeof(count), 1, f);
  fread(&r,     sizeof(r),     1, f);
  fread(&s,     sizeof(s),     1, f);

  latin_square *P = calloc(r, sizeof(latin_square));
  latin_square *Q = calloc(s, sizeof(latin_square));
  if (P == NULL || Q == NULL)
  {
    fprintf(stderr, "[OoM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint32_t i = 0; i < r; ++i)
    latin_square_init(P + i, s);
  for (uint32_t j = 0; j < s; ++j)
    latin_square_init(Q + j, r);

  for (size_t idx = 0; idx < count; ++idx)
  {
    for (uint32_t i = 0; i < r; ++i)
      fread_latin_square_array(f, P + i);
    for (uint32_t j = 0; j < s; ++j)
      fread_latin_square_array(f, Q + j);

    if (!(*func)(P, r, Q, s, data))
    {
      ret = 1;
      break;
    }

    if (idx % REFRESH_RATE == 0)
    {
#ifndef __NO_GUI__
      clear();
      printw("progress: %zu / %zu = %.2f%%\n", idx, count, ((float) idx) / count * 100.0);
      perf->counter = idx;
      perf->lcounter = idx;
      print_perfw(perf, "lsquares array");
      refresh();
#else
      printf("progress: %zu / %zu = %.2f%%\n", idx, count, ((float) idx) / count * 100.0);
      perf->counter = idx;
      perf->lcounter = idx;
      printf_perf(perf, "lsquares array");
#endif
    }
  }

  for (uint32_t i = 0; i < r; ++i)
    latin_square_clear(P + i);
  for (uint32_t j = 0; j < s; ++j)
    latin_square_clear(Q + j);

  free(P);
  free(Q);
  fclose(f);
  return ret;
}
