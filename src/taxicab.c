#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "taxicab.h"
#define __TIMER_IMPLEMENTATION__
#include "timer.h"
#include "perf_counter.h"

#include "gmp.h"
#include "curses.h"

/*
 * return x^n
 */
uint64_t ui_pow_ui(uint64_t x, uint64_t n)
{
  uint64_t acc = 1;
  uint64_t a = x;
  while (n)
  {
    if (n & 0x1)
      acc *= a;
    a *= a;
    n >>= 1;
  }
  return acc;
}

uint64_t taxicab_sum_row(taxicab T, uint64_t i)
{
  assert(i < T.r);

  uint64_t acc = 0;
  for (uint64_t j = 0; j < T.s; ++j)
    acc += ui_pow_ui(TAXI_GET_AS_MAT(T, i, j), T.d);

  return acc;
}

// uint64_t max_pow_m_sqr(pow_m_sqr M)
// {
//   uint64_t max = 0;
//   for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
//   {
//     if (GET_AS_VEC(M, idx) > max)
//       max = GET_AS_VEC(M, idx);
//   }
//   return max;
// }

// uint8_t *nb_occurence_pow_m_sqr(pow_m_sqr M, uint64_t N)
// {
//   uint8_t *occ = calloc(N, sizeof(uint8_t));
//   if (occ == NULL)
//   {
//     fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
//     exit(1);
//   }

//   for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
//     occ[GET_AS_VEC(M, idx) - 1] += 1; // -1 to get them 0-indexed

//   return occ;
// }

// uint8_t pow_m_sqr_is_distinct(pow_m_sqr M)
// {
//   uint64_t N = max_pow_m_sqr(M);

//   uint8_t *occ = nb_occurence_pow_m_sqr(M, N);
//   for (uint64_t idx = 0; idx < N; ++idx)
//     if (occ[idx] > 1)
//       return 0;

//   free(occ);
//   return 1;
// }

uint8_t is_taxicab(taxicab T)
{
  uint64_t mu, curr;

  mu = taxicab_sum_row(T, 0);

  for (uint64_t i = 1; i < T.r; ++i)
  {
    curr = taxicab_sum_row(T, i);
    if (curr != mu)
      return 0;
  }

  return 1;
}

uint8_t taxicab_cross_products_are_distinct(taxicab a, taxicab b)
{
  assert(a.r == b.s && a.s == b.r);

  uint64_t m = 0;
  for (uint32_t i = 0; i < a.r; ++i)
    for (uint32_t j = 0; j < a.s; ++j)
      for (uint32_t u = 0; u < b.r; ++u)
        for (uint32_t v = 0; v < b.s; ++v)
          if (TAXI_GET_AS_MAT(a, i, j) * TAXI_GET_AS_MAT(b, u, v) > m)
            m = TAXI_GET_AS_MAT(a, i, j) * TAXI_GET_AS_MAT(b, u, v);

  uint8_t *counts = calloc(m, sizeof(uint8_t));
  if (counts == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint32_t i = 0; i < a.r; ++i)
    for (uint32_t j = 0; j < a.s; ++j)
      for (uint32_t u = 0; u < b.r; ++u)
        for (uint32_t v = 0; v < b.s; ++v)
        {
          if (counts[TAXI_GET_AS_MAT(a, i, j) * TAXI_GET_AS_MAT(b, u, v)] > 0)
            return 0;
          counts[TAXI_GET_AS_MAT(a, i, j) * TAXI_GET_AS_MAT(b, u, v)] = 1;
        }

  free(counts);
  return 1;
}

// uint8_t is_pow_m_sqr(pow_m_sqr M)
// {
//   return check_sums(M) && pow_m_sqr_is_distinct(M);
// }

/*
 * allocates arr and zero initialises it
 */
int taxicab_init(taxicab *T, uint64_t r, uint64_t s, uint64_t d)
{
  T->d = d;
  T->r = r;
  T->s = s;
  T->arr = calloc(r * s, sizeof(*(T->arr)));
  if (T->arr == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  return 0;
}

void mvtaxicab_print(int y0, int x0, taxicab T)
{
  size_t *max = calloc(T.s, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  char buff[32] = {0};

  for (uint64_t j = 0; j < T.s; ++j)
    for (uint64_t i = 0; i < T.r; ++i)
    {
      size_t l = snprintf(buff, 31, "%llu", TAXI_GET_AS_MAT(T, i, j));
      if (l > max[j])
        max[j] = l;
    }

  size_t dwidth = snprintf(buff, 31, "%llu", T.d);

  size_t width = 1; // start '|'

  for (uint64_t j = 0; j < T.s; ++j)
  {
    width += max[j] + dwidth + 1; // +1 for '|'
    // mvprintw(j, 30, "%llu", max[j]);
  }

  // for (uint64_t _ = 0; _ < width; ++_)
  //   putchar('-');
  // putchar('\n');

  int x = x0, y = y0;
  mvaddch(y, x, ACS_ULCORNER);
  mvvline(y + 1, x, 0, 3 * T.r);
  mvhline(y, x + 1, 0, width - 1);

  for (uint64_t i = 0; i < T.r; ++i)
  {
    x = x0 + 1;
    y += 1;
    for (uint64_t j = 0; j < T.s; ++j)
    {
      x += max[j];
      x += mvprintw(y, x, "%llu", T.d);
      if (i == 0)
      {
        if (j == T.s - 1)
          mvaddch(y - 1, x, ACS_URCORNER);
        else
          mvaddch(y - 1, x, ACS_TTEE);
      }
      else
      {
        if (j == T.s - 1)
          mvaddch(y - 1, x, ACS_RTEE);
        else
          mvaddch(y - 1, x, ACS_PLUS);
      }

      mvaddch(y, x, ACS_VLINE);
      mvaddch(y + 1, x, ACS_VLINE);

      if (j == 0)
      {
        mvhline(y + 2, x0 + 1, 0, width - 2);
        if (i == T.r - 1)
          mvaddch(y + 2, x0, ACS_LLCORNER);
        else
          mvaddch(y + 2, x0, ACS_LTEE);
      }

      if (i == T.r - 1)
      {
        if (j == T.s - 1)
          mvaddch(y + 2, x, ACS_LRCORNER);
        else
          mvaddch(y + 2, x, ACS_BTEE);
      }

      ++x;
    }

    x = x0 + 1;
    y += 1;

    for (uint64_t j = 0; j < T.s; ++j)
    {
      x += mvprintw(y, x, "%*llu", (int)max[j], TAXI_GET_AS_MAT(T, i, j));
      x += dwidth + 1; // +1 for vertical seperator
    }

    y += 1;
    x = x0;
  }

  move(y + 1, x0);

  free(max);

  return;
}

void taxicab_clear(taxicab *T)
{
  free(T->arr);
  T->arr = NULL;
  return;
}

enum
{
  PARTIAL_TAXICAB_VALID,
  PARTIAL_TAXICAB_NEXT,
  PARTIAL_TAXICAB_BREAK
};

/*
 * checks if the newly placed entries at `progress` allow this square to be a valid candidate to be a magic square of M.d-th powers.
 * previous cols/rows/diags not affected by placement are not checked !!
 */
int is_valid_partial_taxicab(taxicab T, uint64_t progress)
{
  uint64_t mu, curr;
  mu = taxicab_sum_row(T, 0); // might not be valid sum if progress < M.n, but in that case it will not be used by tests.
  uint8_t flags = PARTIAL_TAXICAB_VALID;

#define ACT_ON_CMP(cmp)              \
  do                                 \
  {                                  \
    if (cmp > 0)                     \
    {                                \
      flags = PARTIAL_TAXICAB_NEXT;  \
      goto ret;                      \
    }                                \
    if (cmp < 0)                     \
    {                                \
      flags = PARTIAL_TAXICAB_BREAK; \
      goto ret;                      \
    }                                \
  } while (0)

  /*
   * We are only checking rows/cols/diags we just filled as all other previous ones must be valid, as they would have been checked before
   */

  // check only the row we filled
  if ((progress + 1) % T.s == 0 && progress >= T.s)
  {
    uint64_t row = (progress + 1) / T.s;
    curr = taxicab_sum_row(T, row - 1); // 0-indexed
    int cmp = mu - curr;
    ACT_ON_CMP(cmp);
  }

#undef ACT_ON_CMP

ret:
  return flags;
}

/*
 * returns the number of taxicabs there exists AFTER THE SQUARE POINTED TO BY PROGRESS WAS FILLED
 */
uint64_t potential_taxicabs_from_progress(uint64_t r, uint64_t s, uint64_t X, uint64_t progress)
{
  uint64_t acc = 1;
  /*
   * X choices per square
   * BUT every square has to be distinct
   * hence since `progress + 1` choices have been made in the square before progress, there are:
   *   * `X - (progress + 1)    ` choices for the first square
   *   * `X - (progress + 1) - 1` choices for the second
   *   ...
   */
  for (uint64_t i = 0; i < r * s - (progress + 1); ++i)
    acc *= X - (progress + 1) - i;

  return acc;
}

/*
 * searches for (r, s, d)-taxicabs with entries in the form of x^d, with 1 <= x <= X
 * `T` is considered to have its first `progress` entries filled with valid entries
 * returns non-zero if a solution is found
 * solution is set in `T`
 * `*boards_tested` contains the count of boards "tested", ie the index of the current board in lexicographic order
 */
int search_taxicab(taxicab T, uint64_t X, uint64_t progress, uint8_t *heat_map, perf_counter *perf, uint8_t setup)
{
  uint64_t start_val = 1; // start searching from 1 by default
  if (setup)
  {
    if (progress < T.s - 1)
    {
      search_taxicab(T, X, progress + 1, heat_map, perf, 1);
    }
    else if (progress == T.s - 1)
    {
      start_val = TAXI_GET_AS_VEC(T, progress) + 1;
      if (start_val > X)
        start_val = 1;
    }
    else
    {
      fprintf(stderr, "Unreachable !!!\n");
      exit(1);
    }
  }

  if (heat_map == NULL)
  {
    heat_map = calloc(X, sizeof(uint8_t));
    for (uint64_t idx = 0; idx < progress; ++idx)
      heat_map[TAXI_GET_AS_VEC(T, idx)] = 1;
  }

  if (progress == T.r * T.s)
  {
    mpz_add_ui(perf->boards_tested, perf->boards_tested, 1);
    return is_taxicab(T);
  }

  if ((mpz_get_ui(perf->boards_tested) & 0xffffff) == 0 && mpz_cmp_ui(perf->boards_tested, 0) != 0)
  {
#ifndef __DEBUG__
    move(0, 0);
    clear();

    printw("average speed = ");
    print_perfw(*perf, "taxicabs");
    mvtaxicab_print(1, 0, T);
    char buff[256] = {0};
    gmp_snprintf(buff, 255, "%Zu taxicabs have been rejected so far", perf->boards_tested);
    printw("%s.\n Current time: %lfs", buff, timer_stop(&(perf->time)));
    refresh();
#endif
  }

  if ((progress + 1) % T.s == 0 && progress >= T.s)
  {
    int64_t mu = taxicab_sum_row(T, 0);
    int64_t partial_sum = 0;
    uint64_t i = (progress + 1) / T.s - 1; // -1 for 0-index
    for (uint64_t j = 0; j < T.s - 1; ++j)
      partial_sum += ui_pow_ui(TAXI_GET_AS_MAT(T, i, j), T.d);

    mpz_t diff;
    mpz_init_set_si(diff, mu - partial_sum);
    if (mpz_cmp_ui(diff, 0) < 0)
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_taxicabs_from_progress(T.r, T.s, X, progress));
      mpz_clear(diff);
      return 0;
    }

    if (!mpz_root(diff, diff, T.d))
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_taxicabs_from_progress(T.r, T.s, X, progress));
      mpz_clear(diff);
      return 0;
    }

    mpz_clear(diff);
  }

  for (TAXI_GET_AS_VEC(T, progress) = 1; TAXI_GET_AS_VEC(T, progress) <= X; ++TAXI_GET_AS_VEC(T, progress))
  {
    // printf("%llu", GET_AS_VEC(T, progress));
    if (heat_map[TAXI_GET_AS_VEC(T, progress)])
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_taxicabs_from_progress(T.r, T.s, X, progress));
      continue;
    }
    heat_map[TAXI_GET_AS_VEC(T, progress)] = 1;

    uint8_t flags = is_valid_partial_taxicab(T, progress);
    if (flags == PARTIAL_TAXICAB_NEXT)
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_taxicabs_from_progress(T.r, T.s, X, progress));
      heat_map[TAXI_GET_AS_VEC(T, progress)] = 0;
      continue;
    }
    else if (flags == PARTIAL_TAXICAB_BREAK)
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_taxicabs_from_progress(T.r, T.s, X, progress));
      heat_map[TAXI_GET_AS_VEC(T, progress)] = 0;
      break;
    }

    if (search_taxicab(T, X, progress + 1, heat_map, perf, 0))
      return 1;

    heat_map[TAXI_GET_AS_VEC(T, progress)] = 0;
  }

  return 0;
}

void taxicab_transpose(taxicab dst, taxicab src)
{
  assert(dst.r == src.s && dst.s == src.r);

  for (uint64_t i = 0; i < dst.r; ++i)
    for (uint64_t j = 0; j < dst.s; ++j)
      TAXI_GET_AS_MAT(dst, i, j) = TAXI_GET_AS_MAT(src, j, i);

  return;
}