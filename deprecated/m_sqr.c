#include "gmp.h"
#include "m_sqr.h"

/*
 * sums the entries of the j-th column of M into ret
 */
void m_sqr_sum_col(mpz_t ret, m_sqr M, uint64_t j)
{
  mpz_set_ui(ret, 0);
  for (uint64_t i = 0; i < M.n; ++i)
    mpz_add(ret, ret, M_SQR_GET_AS_MAT(M, i, j));
  return;
}

/*
 * sums the entries of the i-th row of M into ret
 */
void m_sqr_sum_row(mpz_t ret, m_sqr M, uint64_t i)
{
  mpz_set_ui(ret, 0);
  for (uint64_t j = 0; j < M.n; ++j)
    mpz_add(ret, ret, M_SQR_GET_AS_MAT(M, i, j));
  return;
}

void m_sqr_sum_diag1(mpz_t ret, m_sqr M)
{
  mpz_set_ui(ret, 0);
  for (uint64_t k = 0; k < M.n; ++k)
    mpz_add(ret, ret, M_SQR_GET_AS_MAT(M, k, k));
  return;
}

void m_sqr_sum_diag2(mpz_t ret, m_sqr M)
{
  mpz_set_ui(ret, 0);
  for (uint64_t k = 0; k < M.n; ++k)
    mpz_add(ret, ret, M_SQR_GET_AS_MAT(M, k, M.n - k - 1));
  return;
}

void max_m_sqr(mpz_t max, m_sqr M)
{
  mpz_set_ui(max, 0);
  for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
  {
    if (mpz_cmp(M_SQR_GET_AS_VEC(M, idx), max) > 0)
      mpz_set(max, M_SQR_GET_AS_VEC(M, idx));
  }
  return;
}

uint8_t *nb_occurence_m_sqr(m_sqr M, uint64_t N)
{

  uint8_t *occ = calloc(N, sizeof(uint8_t));
  if (occ == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint64_t idx = 0; idx < M.n * M.n; ++idx)
    occ[mpz_get_ui(M_SQR_GET_AS_VEC(M, idx)) - 1] += 1; // -1 to get them 0-indexed

  return occ;
}

uint8_t m_sqr_is_distinct(m_sqr M)
{
  mpz_t N_;
  mpz_init(N_);
  max_m_sqr(N_, M);
  if (!mpz_fits_ulong_p(N_))
  {
    fprintf(stderr, "[TODO] checking if square is distinct is not yet implemented for such high max values\n");
    return 0;
  }
  uint64_t N = mpz_get_ui(N_);
  mpz_clear(N_);

  uint8_t *occ = nb_occurence_m_sqr(M, N);
  for (uint64_t idx = 0; idx < N; ++idx)
    if (occ[idx] > 1)
      return 0;

  free(occ);
  return 1;
}

uint8_t is_m_sqr(m_sqr M)
{
  if (M.n <= 0)
    return 0;

  mpz_t mu, curr;
  mpz_inits(mu, curr, (mpz_ptr)NULL);

  m_sqr_sum_row(mu, M, 0);

  for (uint64_t i = 0; i < M.n; ++i)
  {
    m_sqr_sum_row(curr, M, i);
    if (mpz_cmp(curr, mu) != 0)
      return 0;
  }

  for (uint64_t j = 0; j < M.n; ++j)
  {
    m_sqr_sum_col(curr, M, j);
    if (mpz_cmp(curr, mu) != 0)
      return 0;
  }

  m_sqr_sum_diag1(curr, M);
  if (mpz_cmp(curr, mu) != 0)
    return 0;
  m_sqr_sum_diag2(curr, M);
  if (mpz_cmp(curr, mu) != 0)
    return 0;

  mpz_clears(mu, curr, (mpz_ptr)NULL);
  return m_sqr_is_distinct(M);
}

/*
 * allocates arr and zero initialises it
 */
int m_sqr_init(m_sqr *M, uint64_t n)
{
  M->n = n;
  M->arr = calloc(n * n, sizeof(*(M->arr)));
  if (M->arr == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint64_t idx = 0; idx < M->n * M->n; ++idx)
    mpz_init_set_ui(M_SQR_GET_AS_VEC(*M, idx), 0);
  return 0;
}

void m_sqr_print(m_sqr M)
{
  size_t *max = calloc(M.n, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  for (uint64_t j = 0; j < M.n; ++j)
    for (uint64_t i = 0; i < M.n; ++i)
    {
      // +1 for the potential '-' sign
      size_t l = mpz_sizeinbase(M_SQR_GET_AS_MAT(M, i, j), 10) + 1;
      if (l > max[j])
        max[j] = l;
    }

  size_t width = 1; // start '|'

  for (uint64_t j = 0; j < M.n; ++j)
    width += max[j] + 1; // +1 for '|'

  for (uint64_t _ = 0; _ < width; ++_)
    putchar('-');
  putchar('\n');

  for (uint64_t i = 0; i < M.n; ++i)
  {
    putchar('|');
    for (uint64_t j = 0; j < M.n; ++j)
    {
      gmp_printf("% *Zd|", max[j], M_SQR_GET_AS_MAT(M, i, j));
    }
    putchar('\n');
    for (uint64_t _ = 0; _ < width; ++_)
      putchar('-');
    putchar('\n');
  }

  free(max);

  return;
}

void m_sqr_clear(m_sqr *M)
{
  for (uint64_t idx = 0; idx < M->n * M->n; ++idx)
    mpz_clear(M_SQR_GET_AS_VEC(*M, idx));
  free(M->arr);
  M->arr = NULL;
  return;
}