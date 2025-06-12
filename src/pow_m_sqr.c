#define NOB_STRIP_PREFIX
#include "nob.h"
#undef ERROR

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "pow_m_sqr.h"
#include "timer.h"
#include "perf_counter.h"
#include "taxicab.h"
#include "latin_squares.h"

#include "find_sets.h"
#include "find_latin_squares.h"

#include "gmp.h"
#include "curses.h"

#define REQUIERED_SETS 5

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
#ifndef __DEBUG__
    move(0, 0);
    clear();

    printw("average speed = ");
    print_perfw(*perf, "grids");
    mvpow_m_sqr_printw(1, 0, base);
    char buff[256] = {0};
    gmp_snprintf(buff, 255, "%Zu boards have been rejected so far", perf->boards_tested);
    printw("%s.\n Current time: %lfs", buff, timer_stop(&(perf->time)));
    refresh();
#else
    printf("average speed = ");
    printf_perf(*perf, "grids");
    pow_m_sqr_printf(base);
    gmp_printf("%Zu boards have been rejected so far.\n Current time: %lfs\n", perf->boards_tested, timer_stop(&perf->time));
#endif
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
 * P and Q are array of respectively `a.r` and `a.s` latin squares
 * P[j] should be of size `a.s` x `a.s`
 * Q[i] should be of size `a.r` x `a.r`
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
    latin_square_init(&P_standart, a.s);
    standart_latin_square(P_standart);
  }
  if (Q == NULL)
  {
    latin_square_init(&Q_standart, a.r);
    standart_latin_square(Q_standart);
  }
  /*
   * Contrary to the ressource used for this method : https://wismuth.com/magic/squares-of-nth-powers.html#16x16
   * The indices are a bit reversed:
   * block size: r x s
   *                         r blocks
   *                s cols s cols   ...   s cols
   *               |------|------|- ... -|------|
   *           r   | M_11 | M_12 |       | M_1r |
   *          rows |      |      |       |      |
   *               |------|------|- ... -|------|
   *   s       .   .      .      .       .      .
   * blocks    .   .      .      .       .      .
   *           .   .      .      .       .      .
   *               |------|------|- ... -|------|
   *           r   | M_s1 | M_s2 |       | M_sr |
   *          rows |      |      |       |      |
   *               |------|------|- ... -|------|
   *
   * Hence the formula becomes for (i, j, u, v) \in \N_s x \N_r x \N_r x \N_s
   *  -> [M_ij]_uv = ( a_{j, P[j]_{i, v}} * b_{i, Q[i]_{j, u}} )
   */

  for (uint64_t i = 0; i < a.s; ++i)
    for (uint64_t j = 0; j < a.r; ++j)
      for (uint64_t u = 0; u < a.r; ++u)
        for (uint64_t v = 0; v < a.s; ++v)
        {
          latin_square P_j = GET_AS_VEC_IF_NONNULL(P, j);
          latin_square Q_i = GET_AS_VEC_IF_NONNULL(Q, i);
          uint64_t P_jiv = M_SQR_GET_AS_MAT(P_j, i, v);
          uint64_t Q_iju = M_SQR_GET_AS_MAT(Q_i, j, u);
          uint64_t a_idx = TAXI_GET_AS_MAT(a, j, P_jiv);
          uint64_t b_idx = TAXI_GET_AS_MAT(b, i, Q_iju);
          // if (i == 0 && j == 0 && u == 0 && v == 0)
          // printf("M[%llu, %llu] = %llu, %llu", i * a.s + u, j * a.r + v, a_idx, b_idx);
          M_SQR_GET_AS_MAT(M, i * a.r + u, j * a.s + v) = a_idx * b_idx;
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

void transpose_cols(pow_m_sqr M, uint64_t i, uint64_t j)
{
  uint32_t t = M.cols[i];
  M.cols[i] = M.cols[j];
  M.cols[j] = t;
  return;
}

void transpose_rows(pow_m_sqr M, uint64_t i, uint64_t j)
{
  uint32_t t = M.rows[i];
  M.rows[i] = M.rows[j];
  M.rows[j] = t;
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
    // mvpow_m_sqr_printw(0, 0, M);
    // printw("%i, %i\n", i, j);
    // refresh();
    // getch();
  }

  return;
}

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
  return pack->rels->count < 1;
}

// all solution already have magic sum
uint8_t search_pow_m_sqr_from_taxicab_find_sets_collision_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  // find_sets_print_selection(selected, n, NULL);

  position *set = calloc(n, sizeof(position));
  uint32_t idx = 0;
  for (uint32_t i = 0; i < n; ++i)
    for (uint32_t j = 0; j < n; ++j)
      if (GET_AS_MAT(selected, i, j, n))
        set[idx++] = (position){.i = i, .j = j};
  da_append((pack->rels), set);

  // printf("%u\n", pack->rels->count);
  return pack->rels->count < REQUIERED_SETS;
}

typedef struct
{
  /*
   * P = array of size r of latin_squares of side s
   * Q = array of size s of latin_squares of side r
   */
  latin_square *P, *Q;
  uint32_t r, s;
  da_sets rels, mark;
} iterate_over_latin_squares_array_pack;

uint8_t fall_on_different_line_after_latin_squares(position *poses, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
void print_iterate_over_latin_squares_array_pack(iterate_over_latin_squares_array_pack *pack);
void printf_rel(position *rel, const size_t n);
uint8_t rels_are_disjoint(position *rel1, position *rel2, const size_t n);
uint8_t rels_are_compatible(position *rel1, position *rel2, const size_t n);

uint8_t compat_callback1(latin_square *_1, uint64_t _2, void *data);
uint8_t compat_callback2(latin_square *_1, uint64_t _2, void *data);
uint8_t check_for_compatibility_in_latin_squares(latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark);

// start the search on Q
uint8_t compat_callback1(latin_square *_1, uint64_t _2, void *data)
{
  (void)_1, (void)_2;
  iterate_over_latin_squares_array_pack *pack = (iterate_over_latin_squares_array_pack *)data;
  return iterate_over_all_square_array_callback(pack->Q, pack->s, compat_callback2, data);
}

uint8_t compat_callback2(latin_square *_1, uint64_t _2, void *data)
{
  (void)_1, (void)_2;
  iterate_over_latin_squares_array_pack *pack = (iterate_over_latin_squares_array_pack *)data;
  return !check_for_compatibility_in_latin_squares(pack->P, pack->Q, pack->r, pack->s, pack->rels, pack->mark);
}

/*
 * returns non-zero iff two compatible sets exist within the latin_square provided
 */
uint8_t check_for_compatibility_in_latin_squares(latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark)
{
  const uint64_t n = r * s;

  da_foreach(position *, rel, &rels)
  {
    if (!fall_on_different_line_after_latin_squares(*rel, P, Q, r, s))
      continue;

    da_foreach(position *, prev_rel, &mark)
    {
      if (rels_are_compatible(*rel, *prev_rel, n))
      {
        // we found two non-colliding correct sets: success !!
        printf("YAAAAAAAAAAAAAAAAAAAAAAY!!!!!!!!!!!!!\n");
        printf_rel(*rel, n);
        putchar('\n');
        printf_rel(*prev_rel, n);
        putchar('\n');
        return 1; // quit the search
      }
    }
    da_append(&mark, *rel);
  }

  return 0;
}

uint8_t find_set_compatible_latin_squares_array(latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark)
{
  iterate_over_latin_squares_array_pack pack = {.P = P, .Q = Q, .r = r, .s = s, .rels = rels, .mark = mark};
  return iterate_over_all_square_array_callback(P, r, compat_callback1, &pack);
}

void search_pow_m_sqr_from_taxicabs(pow_m_sqr M, taxicab a, taxicab b)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

  // fill M with a semi magic square from standart latin squares
  pow_semi_m_sqr_from_taxicab(M, a, b, NULL, NULL);

  /*
   * TODO: we are currently leaking rels
   */
  da_sets rels = {.n = M.n};
  pow_m_sqr_and_da_sets_packed pack = {.M = &M, .rels = &rels};
  find_sets_collision_method(M, a.r, a.s, search_pow_m_sqr_from_taxicab_find_sets_collision_callback, &pack);

  // FILE *f = fopen("output.txt", "w");
  // da_foreach(position *, rel, &rels)
  // {
  //   for (uint32_t idx = 0; idx < rels.n; ++idx)
  //     fprintf(f, "(%u, %u), ", ((*rel)[idx]).i, ((*rel)[idx]).j);
  //   fprintf(f, "\n");
  // }
  // fclose(f);

#ifndef __DEBUG__
  clear();
  printw("%llu\n", rels.count);
  // for (uint32_t k = 0; k < M.n; ++k)
  //   printf("(%u, %u), ", rels.items[0][k].i, rels.items[0][k].j);
  // putchar('\n');
  // mvpow_m_sqr_printw_highlighted(1, 0, M, rels.items[0]);
  refresh();
  getch();
#else
  printf("%llu\n", rels.count);
#endif

  // arrays holding the latin squares
  latin_square *P = calloc(a.r, sizeof(latin_square));
  latin_square *Q = calloc(a.s, sizeof(latin_square));
  if (P == NULL || Q == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint32_t i = 0; i < a.r; ++i)
    latin_square_init(&(P[i]), a.s);
  for (uint32_t i = 0; i < a.s; ++i)
    latin_square_init(&(Q[i]), a.r);

  da_sets mark = {.n = M.n};
  printf("was%s able to find compatible latin square from the found sets\n", !find_set_compatible_latin_squares_array(P, Q, a.r, a.s, rels, mark) ? "" : " not");

  return;
}

/*
 * P must be of length r and Q of length s
 * P must hold square of size s and Q must hold squares of size r
 */
position position_after_latin_square_permutation(position standart_latin_square_pos, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s)
{
  uint32_t row = standart_latin_square_pos.i;
  uint32_t col = standart_latin_square_pos.j;
  uint32_t i = row / r; // 0 <= i < s
  uint32_t j = col / s; // 0 <= j < r
  uint32_t u = row % r; // 0 <= u < r
  uint32_t v = col % s; // 0 <= v < s

  /*
   * a and b denote here (like in the rest of the codebase) (r, s, d)- and (s, r, d)-taxicabs respectively
   * M_{row, col} = [M_{i, j}]_{u, v} = ( a_{j, [P_standart]_{i, v} * b_{i, [Q_standart]_{j, u}} )^d
   *     in real cases (when P != P_standart)            ^---- j                     ^----- i
   * We want to know where the unique coefficient with that value ended up
   * ie we want to find the unique quatuor (i', j', u', v') such that:
   *  -> a_{j', P[j']_{i', v'}} = a_{j, [P_standart]_{i, v}}
   *  -> b_{i', Q[i']_{j', u'}} = b_{i, [Q_standart]_{j, u}}
   * and since a and b are taxicabs with distinct entries we obtain:
   *  -> j' = j
   *  -> P[j']_{i', v'} = [P_standart]_{i, v}
   *  -> i' = i
   *  -> Q[i']_{j', u'} = [Q_standart]_{j, u}
   * simplifying gives:
   *  -> (i', j') = (i, j)
   *  -> P[j]_{i, v'} = [P_standart]_{i, v}
   *  -> Q[i]_{j, u'} = [Q_standart]_{j, u}
   * We hence need to compare the i-th line from P[j] with the i-th line from the standart P latin square
   * and do the same with the j-th line of Q[i] and the j-th line from the standart Q latin square
   * In fact [P_standart]_{i, v} = (i + v) % s        (remember we have P_standart.n = s)
   * So we just need to find the v' for which this value is hit in P[j]_{i, v'}
   */

  const uint32_t P_standart_iv = ((i + v) % s);

  uint32_t v_prime;
  for (v_prime = 0; v_prime < s; ++v_prime)
  {
    if (M_SQR_GET_AS_MAT(P[j], i, v_prime) == P_standart_iv)
      break;
    if (v_prime == s - 1)
    // last iteration and we did not hit it: absurd there must be a cell with value (i + v) % r in P[j], by construction of a latin square
    {
      fprintf(stderr, "[UNREACHABLE] P: %ux%u latin square with no %u in row %u\n", P[j].n, P[j].n, P_standart_iv, i);
      exit(1);
    }
  }

  const uint32_t Q_standart_ju = ((j + u) % r);

  uint32_t u_prime;
  for (u_prime = 0; u_prime < r; ++u_prime)
  {
    if (M_SQR_GET_AS_MAT(Q[i], j, u_prime) == Q_standart_ju)
      break;
    if (u_prime == r - 1)
    // last iteration and we did not hit it: absurd there must be a cell with value (j + u) % s in Q[i], by construction of a latin square
    {
      fprintf(stderr, "[UNREACHABLE] Q: %ux%u latin square with no %u in row %u\n", Q[i].n, Q[i].n, Q_standart_ju, j);
#ifndef __DEBUG__
      clear();
      mvpow_m_sqr_printw(0, 0, Q[i]);
      getch();
#else
      pow_m_sqr_printf(Q[i]);
      putchar('\n');
#endif
      exit(1);
    }
  }

  return (position){.i = i * r + u_prime, .j = j * s + v_prime};
}

uint8_t fall_on_different_line_after_latin_squares(position *poses, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s)
{
  const uint64_t n = r * s;
  uint8_t *rows = calloc(n, sizeof(uint8_t));
  uint8_t *cols = calloc(n, sizeof(uint8_t));
  for (uint64_t i = 0; i < n; ++i)
  {
    // printf("[2] %u, %u\n", poses[i].i, poses[i].j);
    position new_pos = position_after_latin_square_permutation(poses[i], P, Q, r, s);
    if (rows[new_pos.i] || cols[new_pos.j])
      return 0;
    rows[new_pos.i] = 1;
    cols[new_pos.j] = 1;
  }

  free(rows);
  free(cols);
  return 1;
}

void print_iterate_over_latin_squares_array_pack(iterate_over_latin_squares_array_pack *pack)
{
#ifndef __DEBUG__
  clear();
  for (uint32_t i = 0; i < pack->s; ++i)
    mvpow_m_sqr_printw(0, i * 5 * pack->r, pack->P[i]);

  for (uint32_t i = 0; i < pack->r; ++i)
    mvpow_m_sqr_printw(5 * pack->r + 2, i * 5 * pack->s, pack->Q[i]);
#else
  (void)pack;
#endif

  return;
}

void printf_rel(position *rel, const size_t n)
{
  for (size_t k = 0; k < n; ++k)
    printf("(%u, %u), ", rel[k].i, rel[k].j);

  return;
}

uint8_t rels_are_disjoint(position *rel1, position *rel2, const size_t n)
{
  for (size_t k = 0; k < n; ++k)
    if (rel1[k].i == rel2[k].i || rel1[k].j == rel2[k].j)
      return 0;

  return 1;
}

uint8_t rels_are_compatible(position *rel1, position *rel2, const size_t n)
{
  if (!rels_are_disjoint(rel1, rel2, n))
    return 0;

  /*
   * The compatibility condition is as follow:
   * initiate two counter, i = 0 and j = 0
   * for each column:
   *  - add one to i if the element of rel2 in this column is above the element of rel1 in the column
   *    if the elements from rel1 and rel2 collide: n is odd and this is the middle: ignore
   * for each row:
   *  - add one to the j if the element of rel2 in this row is to the left of the element of rel1 in the row
   *    if the elements from rel1 and rel2 collide: n is odd and this is the middle: ignore
   * FOR N EVEN:
   * rel1 and rel2 are compatible iff i = j = n/2 if n is even
   * FOR N ODD:
   * rel1 and rel2 are compatible iff i = j = (n - 1)/2 if n is odd
   */

  uint64_t top_count = 0;
  uint64_t left_count = 0;

  (void)top_count, (void)left_count;

  fprintf(stderr, "[TODO] Not (yet) implemented");

  return 1;
}

void permute_into_pow_m_sqr(pow_m_sqr M, position *diag1, position *diag2)
{
  const uint64_t n = M.n;
  // transpose the main diagonal into place:
  for (uint64_t j = 0; j < n; ++j)
  {
    uint32_t col = diag1[j].j;
    transpose_cols(M, col, j);

    // position of the j-th column in the array diag1
    uint64_t idx1 = 0;
    for (; idx1 < n; ++idx1)
      if (diag1[idx1].j == j)
        break;

    // fix diag1
    uint64_t k = 0;
    for (; k < n; ++k)
      if (diag1[k].j == col)
        break;
    diag1[idx1].j = diag1[k].j;
    diag1[k].j = j;

    // position of the j-th column in the array diag2
    uint64_t idx2 = 0;
    for (; idx2 < n; ++idx2)
      if (diag2[idx2].j == j)
        break;

    // fix diag2
    k = 0;
    for (; k < n; ++k)
      if (diag2[k].j == col)
        break;
    uint32_t t = diag2[idx2].j;
    diag2[idx2].j = diag2[k].j;
    diag2[k].j = t;

    // printf("%hhd\n", parity_of_sets(diag1, diag2, M.n));

#ifndef __DEBUG__
    // clear();
    // mvpow_m_sqr_printw_highlighted(0, 0, M, diag1, diag2, COLOR_YELLOW, COLOR_CYAN);
    // printw("%hhd\n", parity_of_sets(diag1, diag2, M.n));
    // refresh();
    // getch();
#else
    // pow_m_sqr_printf(M);
    // putchar('\n');
#endif
  }

  // printf("----------------------\n");

#ifndef __DEBUG__
  // clear();
  // mvpow_m_sqr_printw_highlighted(0, 0, M, diag1, diag2, COLOR_YELLOW, COLOR_CYAN);
  // refresh();
  // getch();
#else
  // pow_m_sqr_printf(M);
  // putchar('\n');
#endif

  // transpose the anti-diagonal into place:
  for (uint64_t j = 0; j < n; ++j)
  {
    uint32_t col = diag2[j].j;
    transpose_cols(M, col, j);

    // position of the j-th column in the array diag1
    uint64_t idx1 = 0;
    for (; idx1 < n; ++idx1)
      if (diag1[idx1].j == j)
        break;

    // fix diag1
    uint64_t k = 0;
    for (; k < n; ++k)
      if (diag1[k].j == col)
        break;
    diag1[idx1].j = diag1[k].j;
    diag1[k].j = j;

    // position of the j-th column in the array diag2
    uint64_t idx2 = 0;
    for (; idx2 < n; ++idx2)
      if (diag2[idx2].j == j)
        break;

    // fix diag2
    k = 0;
    for (; k < n; ++k)
      if (diag2[k].j == col)
        break;
    uint32_t t = diag2[idx2].j;
    diag2[idx2].j = diag2[k].j;
    diag2[k].j = t;

    // printf("%hhd, ", parity_of_sets(diag1, diag2, M.n));

#ifndef __DEBUG__
    // clear();
    // mvpow_m_sqr_printw_highlighted(0, 0, M, diag1, diag2, COLOR_YELLOW, COLOR_CYAN);
    // printw("%hhd\n", parity_of_sets(diag1, diag2, M.n));
    // refresh();
    // getch();
#else
    // pow_m_sqr_printf(M);
    // putchar('\n');
#endif

    transpose_rows(M, col, j);

    // position of the j-th row in the array diag1
    idx1 = 0;
    for (; idx1 < n; ++idx1)
      if (diag1[idx1].i == j)
        break;

    // fix diag1
    k = 0;
    for (; k < n; ++k)
      if (diag1[k].i == col)
        break;
    diag1[idx1].i = diag1[k].i;
    diag1[k].i = j;

    // position of the j-th column in the array diag2
    idx2 = 0;
    for (; idx2 < n; ++idx2)
      if (diag2[idx2].i == j)
        break;

    // fix diag2
    k = 0;
    for (; k < n; ++k)
      if (diag2[k].i == col)
        break;
    t = diag2[idx2].i;
    diag2[idx2].i = diag2[k].i;
    diag2[k].i = t;

    // printf("%hhd\n", parity_of_sets(diag1, diag2, M.n));

#ifndef __DEBUG__
    // clear();
    // mvpow_m_sqr_printw_highlighted(0, 0, M, diag1, diag2, COLOR_YELLOW, COLOR_CYAN);
    // printw("%hhd\n", parity_of_sets(diag1, diag2, M.n));
    // refresh();
    // getch();
#else
    // pow_m_sqr_printf(M);
    // putchar('\n');
#endif
  }

  return;
}

/*
 * returns the parity of the permutation changing the two sets, rel1 and rel2, into the two main diags
 */
int8_t parity_of_sets(position *rel1, position *rel2, const uint64_t n)
{
  int8_t ret = 1;

  for (uint64_t idx = 0; idx < n; ++idx)
  {
    if (rel1[idx].i != rel1[idx].j || rel1[idx].i != (n - 1) - rel1[idx].j)
    {
      // will need to be transposed
      ret *= -1;
      // printf("(%u %u)", rel1[idx].i, rel1[idx].j);
    }

    // indices go from 0 to (n - 1)
    if (rel2[idx].i != rel2[idx].j || rel2[idx].i != (n - 1) - rel2[idx].j)
    {
      // will need to be transposed
      ret *= -1;
      // printf("(%u %u)", rel2[idx].i, (uint32_t)(n - 1) - rel2[idx].j);
    }
  }

  // putchar('\n');

  return ret;
}