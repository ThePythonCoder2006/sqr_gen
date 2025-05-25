#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "pow_m_sqr.h"
#include "timer.h"
#include "perf_counter.h"
#include "taxicab.h"
#include "latin_squares.h"
#include "find_sets.h"

#define NOB_STRIP_PREFIX
#include "nob.h"
#include "gmp.h"
#include "curses.h"

/*
 * sums the entries of the j-th column of M into ret
 */
// void pow_m_sqr_sum_col(mpz_t ret, pow_m_sqr M, uint64_t j)
// {
//   mpz_t curr;
//   mpz_init(curr);
//   mpz_set_ui(ret, 0);
//   for (uint64_t i = 0; i < M.n; ++i)
//   {
//     mpz_ui_pow_ui(curr, M_SQR_GET_AS_MAT(M, i, j), M.d);
//     mpz_add(ret, ret, curr);
//   }
//   mpz_clear(curr);
//   return;
// }

uint64_t ui_pow_ui(uint64_t x, uint64_t n);

uint64_t pow_m_sqr_sum_col(pow_m_sqr M, uint64_t j)
{
  uint64_t acc = 0;
  for (uint64_t i = 0; i < M.n; ++i)
    acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, i, j), M.d);

  return acc;
}

/*
 * sums the entries of the i-th row of M into ret
 */
// void pow_m_sqr_sum_row(mpz_t ret, pow_m_sqr M, uint64_t i)
// {
//   mpz_t curr;
//   mpz_init(curr);
//   mpz_set_ui(ret, 0);
//   for (uint64_t j = 0; j < M.n; ++j)
//   {
//     mpz_ui_pow_ui(curr, M_SQR_GET_AS_MAT(M, i, j), M.d);
//     mpz_add(ret, ret, curr);
//   }
//   mpz_clear(curr);
//   return;
// }

uint64_t pow_m_sqr_sum_row(pow_m_sqr M, uint64_t i)
{
  uint64_t acc = 0;
  for (uint64_t j = 0; j < M.n; ++j)
    acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, i, j), M.d);

  return acc;
}

// void pow_m_sqr_sum_diag1(mpz_t ret, pow_m_sqr M)
// {
//   mpz_t curr;
//   mpz_init(curr);
//   mpz_set_ui(ret, 0);
//   for (uint64_t k = 0; k < M.n; ++k)
//   {
//     mpz_ui_pow_ui(curr, M_SQR_GET_AS_MAT(M, k, k), M.d);
//     mpz_add(ret, ret, curr);
//   }
//   mpz_clear(curr);
//   return;
// }

uint64_t pow_m_sqr_sum_diag1(pow_m_sqr M)
{
  uint64_t acc = 0;
  for (uint64_t k = 0; k < M.n; ++k)
    acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, k, k), M.d);

  return acc;
}

// void pow_m_sqr_sum_diag2(mpz_t ret, pow_m_sqr M)
// {
//   mpz_t curr;
//   mpz_init(curr);
//   mpz_set_ui(ret, 0);
//   for (uint64_t k = 0; k < M.n; ++k)
//   {
//     mpz_ui_pow_ui(curr, M_SQR_GET_AS_MAT(M, k, M.n - k - 1), M.d);
//     mpz_add(ret, ret, curr);
//   }
//   mpz_clear(curr);
//   return;
// }

uint64_t pow_m_sqr_sum_diag2(pow_m_sqr M)
{
  uint64_t acc = 0;
  for (uint64_t k = 0; k < M.n; ++k)
    acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, k, M.n - k - 1), M.d);

  return acc;
}

uint64_t max_pow_m_sqr(pow_m_sqr M)
{
  uint64_t max = 0;
  for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
  {
    if (M_SQR_GET_AS_VEC(M, idx) > max)
      max = M_SQR_GET_AS_VEC(M, idx);
  }
  return max;
}

uint8_t *nb_occurence_pow_m_sqr(pow_m_sqr M, uint64_t N)
{
  uint8_t *occ = calloc(N, sizeof(uint8_t));
  if (occ == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
    occ[M_SQR_GET_AS_VEC(M, idx) - 1] += 1; // -1 to get them 0-indexed

  return occ;
}

uint8_t pow_m_sqr_is_distinct(pow_m_sqr M)
{
  uint64_t N = max_pow_m_sqr(M);

  uint8_t *occ = nb_occurence_pow_m_sqr(M, N);
  for (uint64_t idx = 0; idx < N; ++idx)
    if (occ[idx] > 1)
      return 0;

  free(occ);
  return 1;
}

uint8_t check_sums(pow_m_sqr M)
{
  if (!is_pow_semi_m_sqr(M))
    return 0;

  uint64_t mu, curr;

  mu = pow_m_sqr_sum_row(M, 0);

  curr = pow_m_sqr_sum_diag1(M);
  if (curr != mu)
    return 0;
  curr = pow_m_sqr_sum_diag2(M);
  if (curr != mu)
    return 0;
  return 1;
}

uint8_t is_pow_semi_m_sqr(pow_m_sqr M)
{
  if (M.n <= 0)
    return 0;

  uint64_t mu, curr;

  mu = pow_m_sqr_sum_row(M, 0);
  // printf("%llu\n", mu);

  for (uint64_t i = 1; i < M.n; ++i)
  {
    curr = pow_m_sqr_sum_row(M, i);
    if (curr != mu)
      return 0;
  }

  for (uint64_t j = 0; j < M.n; ++j)
  {
    curr = pow_m_sqr_sum_col(M, j);
    if (curr != mu)
      return 0;
  }

  return pow_m_sqr_is_distinct(M);
}

uint8_t is_pow_m_sqr(pow_m_sqr M)
{
  return check_sums(M) && pow_m_sqr_is_distinct(M);
}

/*
 * allocates arr and zero initialises it
 */
int pow_m_sqr_init(pow_m_sqr *M, uint64_t n, uint64_t d)
{
  M->n = n;
  M->d = d;
  M->arr = calloc(n * n, sizeof(*(M->arr)));
  M->cols = calloc(n, sizeof(*M->cols));
  M->rows = calloc(n, sizeof(*M->rows));
  if (M->arr == NULL || M->cols == NULL || M->rows == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  for (uint64_t i = 0; i < n; ++i)
  {
    M->cols[i] = i;
    M->rows[i] = i;
  }
  return 0;
}

void mvpow_m_sqr_print(int y0, int x0, pow_m_sqr M)
{
  size_t *max = calloc(M.n, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  char buff[32] = {0};

  for (uint64_t j = 0; j < M.n; ++j)
    for (uint64_t i = 0; i < M.n; ++i)
    {
      size_t l = snprintf(buff, 31, "%llu", M_SQR_GET_AS_MAT(M, i, j));
      if (l > max[j])
        max[j] = l;
    }

  size_t dwidth = snprintf(buff, 31, "%u", M.d);

  size_t width = 1; // start '|'

  for (uint64_t j = 0; j < M.n; ++j)
  {
    width += max[j] + dwidth + 1; // +1 for '|'
    // mvprintw(j, 30, "%llu", max[j]);
  }

  // for (uint64_t _ = 0; _ < width; ++_)
  //   putchar('-');
  // putchar('\n');

  int x = x0, y = y0;
  mvaddch(y, x, ACS_ULCORNER);
  mvvline(y + 1, x, 0, 3 * M.n);
  mvhline(y, x + 1, 0, width - 1);

  for (uint64_t i = 0; i < M.n; ++i)
  {
    x = x0 + 1;
    y += 1;
    for (uint64_t j = 0; j < M.n; ++j)
    {
      x += max[j];
      x += mvprintw(y, x, "%u", M.d);
      if (i == 0)
      {
        if (j == M.n - 1)
          mvaddch(y - 1, x, ACS_URCORNER);
        else
          mvaddch(y - 1, x, ACS_TTEE);
      }
      else
      {
        if (j == M.n - 1)
          mvaddch(y - 1, x, ACS_RTEE);
        else
          mvaddch(y - 1, x, ACS_PLUS);
      }

      mvaddch(y, x, ACS_VLINE);
      mvaddch(y + 1, x, ACS_VLINE);

      if (j == 0)
      {
        mvhline(y + 2, x0 + 1, 0, width - 2);
        if (i == M.n - 1)
          mvaddch(y + 2, x0, ACS_LLCORNER);
        else
          mvaddch(y + 2, x0, ACS_LTEE);
      }

      if (i == M.n - 1)
      {
        if (j == M.n - 1)
          mvaddch(y + 2, x, ACS_LRCORNER);
        else
          mvaddch(y + 2, x, ACS_BTEE);
      }

      ++x;
    }

    x = x0 + 1;
    y += 1;

    for (uint64_t j = 0; j < M.n; ++j)
    {
      x += mvprintw(y, x, "%*llu", (int)max[j], M_SQR_GET_AS_MAT(M, i, j));
      x += dwidth + 1; // +1 for vertical seperator
    }

    y += 1;
    x = x0;
  }

  move(y + 1, x0);

  free(max);

  return;
}

void pow_m_sqr_clear(pow_m_sqr *M)
{
  free(M->arr);
  M->arr = NULL;
  return;
}

enum
{
  PARTIAL_M_SQR_VALID,
  PARTIAL_M_SQR_NEXT,
  PARTIAL_M_SQR_BREAK
};

/*
 * checks if the newly placed entries at `progress` allow this square to be a valid candidate to be a magic square of M.d-th powers.
 * previous cols/rows/diags not affected by placement are not checked !!
 */
int is_valid_partial_pow_m_sqr(pow_m_sqr M, uint64_t progress)
{
  uint64_t mu, curr;
  mu = pow_m_sqr_sum_row(M, 0); // might not be valid sum if progress < M.n, but in that case it will not be used by tests.
  uint8_t flags = PARTIAL_M_SQR_VALID;

#define ACT_ON_CMP(cmp)            \
  do                               \
  {                                \
    if (cmp > 0)                   \
    {                              \
      flags = PARTIAL_M_SQR_NEXT;  \
      goto ret;                    \
    }                              \
    if (cmp < 0)                   \
    {                              \
      flags = PARTIAL_M_SQR_BREAK; \
      goto ret;                    \
    }                              \
  } while (0)

  /*
   * We are only checking rows/cols/diags we just filled as all other previous ones must be valid, as they would have been checked before
   */

  // check only the row we filled
  if ((progress + 1) % M.n == 0 && progress >= M.n)
  {
    uint64_t row = (progress + 1) / M.n;
    curr = pow_m_sqr_sum_row(M, row - 1); // 0-indexed
    int cmp = mu - curr;
    ACT_ON_CMP(cmp);
  }

  // check only the column we filled
  if (progress >= M.n * (M.n - 1))
  {
    uint64_t col = progress - (M.n * (M.n - 1) - 1);
    curr = pow_m_sqr_sum_col(M, col - 1); // 0-indexed
    int cmp = mu - curr;
    ACT_ON_CMP(cmp);

    // check only if we filled the diag
    if (col == 1)
    {
      curr = pow_m_sqr_sum_diag2(M);
      // gmp_printf("%Zd, %Zd\n", mu, curr);
      int cmp = mu - curr;
      ACT_ON_CMP(cmp);
    }
  }
#undef ACT_ON_CMP

ret:
  return flags;
}

/*
 * returns the number of boards there exists AFTER THE SQUARE POINTED TO BY PROGRESS WAS FILLED
 */
uint64_t potential_boards_from_progress(uint64_t n, uint64_t X, uint64_t progress)
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
  for (uint64_t i = 0; i < n * n - (progress + 1); ++i)
    acc *= X - (progress + 1) - i;

  return acc;
}

/*
 * searches for magic squares of base.d-th power with entries in the form of x^d, with 1 <= x <= X
 * base is considered to have its first progress entires filled with valid entries
 * returns non-zero if a solution is found
 * solution is set in `base`
 * `*boards_tested` contains the count of boards "tested", ie the index of the current board in lexicographic order
 */
int search_pow_m_sqr(pow_m_sqr base, uint64_t X, uint64_t progress, uint8_t *heat_map, perf_counter *perf)
{
  if (heat_map == NULL)
  {
    heat_map = calloc(X, sizeof(uint8_t));
    for (uint64_t idx = 0; idx < progress; ++idx)
      heat_map[M_SQR_GET_AS_VEC(base, idx)] = 1;
  }

  if (progress == base.n * base.n)
  {
    mpz_add_ui(perf->boards_tested, perf->boards_tested, 1);
    return is_pow_m_sqr(base);
  }

  if ((mpz_get_ui(perf->boards_tested) & 0xffffff) == 0 && mpz_cmp_ui(perf->boards_tested, 0) != 0)
  {
    move(0, 0);
    clear();

    printw("average speed = ");
    print_perfw(*perf, "grids");
    mvpow_m_sqr_print(1, 0, base);
    char buff[256] = {0};
    gmp_snprintf(buff, 255, "%Zu boards have been rejected so far", perf->boards_tested);
    printw("%s.\n Current time: %lfs", buff, timer_stop(&(perf->time)));
    refresh();
  }

  if ((progress + 1) % base.n == 0 && progress >= base.n)
  {
    int64_t mu = pow_m_sqr_sum_row(base, 0);
    int64_t partial_sum = 0;
    uint64_t i = (progress + 1) / base.n - 1; // -1 for 0-index
    for (uint64_t j = 0; j < base.n - 1; ++j)
      partial_sum += ui_pow_ui(M_SQR_GET_AS_MAT(base, i, j), base.d);

    mpz_t diff;
    mpz_init_set_si(diff, mu - partial_sum);
    if (!mpz_root(diff, diff, base.d))
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_boards_from_progress(base.n, X, progress));
      mpz_clear(diff);
      return 0;
    }

    mpz_clear(diff);
  }

  for (M_SQR_GET_AS_VEC(base, progress) = 1; M_SQR_GET_AS_VEC(base, progress) <= X; ++M_SQR_GET_AS_VEC(base, progress))
  {
    // printf("%llu", M_SQR_GET_AS_VEC(base, progress));
    if (heat_map[M_SQR_GET_AS_VEC(base, progress)])
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_boards_from_progress(base.n, X, progress));
      continue;
    }
    heat_map[M_SQR_GET_AS_VEC(base, progress)] = 1;

    uint8_t flags = is_valid_partial_pow_m_sqr(base, progress);
    if (flags == PARTIAL_M_SQR_NEXT)
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_boards_from_progress(base.n, X, progress));
      heat_map[M_SQR_GET_AS_VEC(base, progress)] = 0;
      continue;
    }
    else if (flags == PARTIAL_M_SQR_BREAK)
    {
      mpz_add_ui(perf->boards_tested, perf->boards_tested, potential_boards_from_progress(base.n, X, progress));
      heat_map[M_SQR_GET_AS_VEC(base, progress)] = 0;
      break;
    }

    if (search_pow_m_sqr(base, X, progress + 1, heat_map, perf))
      return 1;

    heat_map[M_SQR_GET_AS_VEC(base, progress)] = 0;
  }

  return 0;
}

#define GET_AS_MAT_IF_NONNULL(l, i, j) (l == NULL ? l##_standart : GET_AS_MAT(l, i, j))
#define GET_AS_VEC_IF_NONNULL(l, idx) (l == NULL ? l##_standart : l[idx])

/*
 * P and Q are array of respectively `a.s` and `a.r` latin squares
 * P[j] should be of size `a.r` x `a.r`
 * Q[i] should be of size `a.s` x `a.s`
 * P and Q are allowed to be NULL, in that case, the standart latin squares will be used everywhere
 * `a.r` must be equal to `b.s` and `a.s` must be equal to `b.r`
 */
void pow_semi_m_sqr_from_taxicab(pow_m_sqr M, taxicab a, taxicab b, latin_square *P, latin_square *Q)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

  latin_square P_standart, Q_standart = {0};
  if (P == NULL)
  {
    latin_square_init(&P_standart, a.r);
    standart_latin_square(P_standart);
  }
  if (Q == NULL)
  {
    latin_square_init(&Q_standart, a.s);
    standart_latin_square(Q_standart);
  }

  /*
   * matrix of block of size:a
   * WIDTH: `a.r` blocks and LENGTH: `a.s` blocks
   */
  for (uint64_t i = 0; i < a.r; ++i)
    for (uint64_t j = 0; j < a.s; ++j)
      /*
       * iterate through blocks of size:
       * WIDTH: `a.s` and LENGTH: `a.r`
       */
      for (uint64_t u = 0; u < a.s; ++u)
        for (uint64_t v = 0; v < a.r; ++v)
        {
          latin_square P_j = GET_AS_VEC_IF_NONNULL(P, j);
          latin_square Q_i = GET_AS_VEC_IF_NONNULL(Q, i);
          uint64_t P_jiv = M_SQR_GET_AS_MAT(P_j, i, v);
          uint64_t Q_iju = M_SQR_GET_AS_MAT(Q_i, j, u);
          uint64_t a_idx = TAXI_GET_AS_MAT(a, j, P_jiv);
          uint64_t b_idx = TAXI_GET_AS_MAT(b, i, Q_iju);
          // if (i == 0 && j == 0 && u == 0 && v == 0)
          // printf("M[%llu, %llu] = %llu, %llu", i * a.s + u, j * a.r + v, a_idx, b_idx);
          M_SQR_GET_AS_MAT(M, i * a.s + u, j * a.r + v) = a_idx * b_idx;
        }

  return;
}

#if __IN_PLACE_PERMUT__
/*
 * permut should be an array of size `M.n` where `permut[j]` contains the final indice of column `j`
 */
void permute_cols(pow_m_sqr M, uint64_t *permut)
{
  uint64_t *arr_copy = calloc(M.n * M.n, sizeof(*(M.arr)));
  memcpy(arr_copy, M.arr, M.n * M.n * sizeof(*(M.arr)));

  for (uint64_t i = 0; i < M.n; ++i)
    for (uint64_t j = 0; j < M.n; ++j)
      M_SQR_GET_AS_MAT(M, i, permut[j]) = arr_copy[i * M.n + j];

  free(M.arr);
  M.arr = arr_copy;

  return;
}

/*
 * permut should be an array of size `M.n` where `permut[i]` contains the final indice of line `i`
 */
void permute_lines(pow_m_sqr M, uint64_t *permut)
{
  uint64_t *arr_copy = calloc(M.n * M.n, sizeof(*(M.arr)));
  memcpy(arr_copy, M.arr, M.n * M.n * sizeof(*(M.arr)));

  for (uint64_t i = 0; i < M.n; ++i)
    for (uint64_t j = 0; j < M.n; ++j)
      M_SQR_GET_AS_MAT(M, permut[i], j) = arr_copy[i * M.n + j];

  free(arr_copy);

  return;
}
#else
/*
 * permut should be an array of size `M.n` where `permut[j]` contains the final indice of column `j`
 */
void permute_cols(pow_m_sqr M, uint64_t *permut)
{
  uint32_t *t = calloc(M.n, sizeof(*M.cols));
  memcpy(t, M.cols, M.n * sizeof(*M.cols));
  for (uint32_t j = 0; j < M.n; ++j)
    M.cols[permut[j]] = t[j];

  free(t);
  return;
}

/*
 * permut should be an array of size `M.n` where `permut[i]` contains the final indice of line `i`
 */
void permute_lines(pow_m_sqr M, uint64_t *permut)
{
  uint32_t *t = calloc(M.n, sizeof(*M.rows));
  memcpy(t, M.cols, M.n * sizeof(*M.rows));
  for (uint32_t i = 0; i < M.n; ++i)
    M.rows[permut[i]] = t[i];

  free(t);
  return;
}

#endif // __IN_PLACE_PERMUT__

/*
 * Fisher-Yates shuffle
 */
void random_perm(uint64_t *l, size_t n)
{
  for (size_t i = n - 1; i > 0; --i)
  {
    size_t j = rand() % (i + 1);

    uint64_t t = l[j];
    l[j] = l[i];
    l[i] = t;
  }

  return;
}

/*
 * applies the Fisher-Yates shuffle to the columns of M
 */
void shuffle_cols(pow_m_sqr M)
{
  uint64_t *l = calloc(M.n, sizeof(uint64_t));
  for (uint64_t i = 0; i < M.n; ++i)
    l[i] = i;

  random_perm(l, M.n);
  permute_cols(M, l);

  free(l);

  return;
}

/*
 * applies the Fisher-Yates shuffle to the lines of M
 */
void shuffle_lines(pow_m_sqr M)
{
  uint64_t *l = calloc(M.n, sizeof(uint64_t));
  for (uint64_t i = 0; i < M.n; ++i)
    l[i] = i;

  random_perm(l, M.n);
  permute_lines(M, l);

  free(l);

  return;
}

void shuffle_lines_and_cols_with_same_perm(pow_m_sqr M)
{
  uint64_t *l = calloc(M.n, sizeof(uint64_t));
  for (uint64_t i = 0; i < M.n; ++i)
    l[i] = i;

  random_perm(l, M.n);
  permute_lines(M, l);
  permute_cols(M, l);

  free(l);

  return;
}

/*
 * `M` is expected to contain a semi-magic square of powers
 */
void search_pow_m_sqr_from_pow_semi_m_sqr(pow_m_sqr M)
{
  uint64_t mu = pow_m_sqr_sum_row(M, 0);

  uint64_t count = 0;
  uint64_t curr;
  while ((curr = pow_m_sqr_sum_diag1(M)) != mu)
  {
    shuffle_lines(M);
    ++count;
    if ((count & 0xffff) == 0)
      printf("1: %llu, %llu != %llu\n", count, curr, mu);
  }

  uint64_t count2 = 0;
  while ((curr = pow_m_sqr_sum_diag2(M)) != mu)
  {
    shuffle_lines_and_cols_with_same_perm(M);
    ++count2;
    if ((count2 & 0xffff) == 0)
      printf("2: %llu, %llu != %llu\n", count2, curr, mu);
  }

  return;
}

void generate_siamese(pow_m_sqr M)
{
  for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
    M_SQR_GET_AS_VEC(M, idx) = 0;

  int64_t i = 0, j = (M.n - 1) / 2;
  for (uint64_t idx = 1; idx <= M.n * M.n; ++idx)
  {
    M_SQR_GET_AS_MAT(M, i, j) = idx;
    if (M_SQR_GET_AS_MAT(M, (i + M.n - 1) % M.n, (j + 1) % M.n) != 0)
      // spot is full: go down
      i = (i + 1) % M.n;
    else // move up right
    {
      i = (i + M.n - 1) % M.n;
      j = (j + 1) % M.n;
    }
    // clear();
    // mvpow_m_sqr_print(0, 0, M);
    // printw("%i, %i\n", i, j);
    // refresh();
    // getch();
  }

  return;
}

uint8_t set_has_magic_sum(uint8_t *selected, pow_m_sqr M)
{
  uint64_t mu = pow_m_sqr_sum_row(M, 0);
  uint64_t acc = 0;
  for (uint32_t i = 0; i < M.n; ++i)
    for (uint32_t j = 0; j < M.n; ++j)
      if (GET_AS_MAT(selected, i, j, M.n))
        acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, i, j), M.d);

  return mu == acc;
}

typedef struct
{
  uint32_t i, j;
} position;

typedef struct da_sets_s
{
  position **items;
  size_t count;
  size_t capacity;
  uint32_t n; // size of each set
} da_sets;

typedef struct pow_m_sqr_and_da_sets_packed_s
{
  pow_m_sqr *M;
  da_sets *rels;
} pow_m_sqr_and_da_sets_packed;

uint8_t search_pow_m_sqr_from_taxicab_iterate_over_sets_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  if (set_has_magic_sum(selected, *(pack->M)))
  {
    position *set = calloc(n, sizeof(position));
    uint32_t idx = 0;
    for (uint32_t i = 0; i < n; ++i)
      for (uint32_t j = 0; j < n; ++j)
        if (GET_AS_MAT(selected, i, j, n))
          set[idx++] = (position){.i = i, .j = j};
    da_append((pack->rels), set);
  }
  // printf("%u\n", pack->rels->count);
  return 1; // pack->rels->count < 1;
}

void search_pow_m_sqr_from_taxicabs(pow_m_sqr M, taxicab a, taxicab b)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

  // fill M with a semi magic square from standart latin squares
  pow_semi_m_sqr_from_taxicab(M, a, b, NULL, NULL);

  da_sets rels = {.n = M.n};
  pow_m_sqr_and_da_sets_packed pack = {.M = &M, .rels = &rels};
  iterate_over_sets_callback(a.r, a.s, search_pow_m_sqr_from_taxicab_iterate_over_sets_callback, (void *)&pack);

  da_foreach(position *, rel, &rels)
  {
    for (uint32_t k = 0; k < rels.n; ++k)
      printf("(%u, %u) ", (*rel)[k].i, (*rel)[k].j);
    putchar('\n');
  }

  return;
}