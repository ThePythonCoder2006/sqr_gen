// #include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include "pow_m_sqr.h"
#include "arithmetic.h"

#include "gmp.h"

const uint32_t list_of_primes[] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167};// , 173};
/*
    179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
    419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
    547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
    811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
    947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021};
*/

const uint64_t A000479[] = {1ULL, 1ULL, 1ULL, 2ULL, 24ULL, 1344ULL, 1128960ULL, 12198297600ULL,
                            2697818265354240ULL};

#define lOCAL_BOUND 1024
#define NUM_SAMPLES 10000

double p_magic(uint64_t m, uint64_t n, double c)
{
  // p_magic = n / (2 pi c^2 m^2)
  return ((double)n) / ((double)2 * M_PI * c * c * (double)m * (double)m);
}

// Computes (base^exp) % mod using modular exponentiation
uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod)
{
  uint64_t result = 1;
  base = base % mod;
  while (exp > 0)
  {
    if (exp % 2 == 1)
      result = (result * base) % mod;
    base = (base * base) % mod;
    exp /= 2;
  }
  return result;
}

double correction_factor(uint64_t p_e, uint64_t *arr, uint64_t d, uint64_t n, uint64_t m, uint64_t num_samples)
{
  const uint64_t modulus = p_e;
  uint64_t count_zero = 0;

  for (uint64_t sample = 0; sample < num_samples; ++sample)
  {
    uint64_t sum = 0;

    for (uint64_t i = 0; i < n; ++i)
    {
      uint64_t x = rand() % n * n;

      sum = (sum + mod_pow(arr[x], d, modulus)) % modulus;
    }

    if (sum == m % modulus)
      count_zero++;
  }

  const double ret = (double)modulus * (double)count_zero / num_samples;

  // printf("m === %"PRIu64" [%"PRIu64"]\tcorr = %lf\n", m % modulus, modulus, ret);

  return ret;
}

void n_perm(mpz_t rop, uint64_t n)
{
  mpz_fac_ui(rop, n);
  mpz_mul(rop, rop, rop);

  mpz_t tmp;
  mpz_init(tmp);

  mpz_fac_ui(tmp, n / 2);
  mpz_div(rop, rop, tmp);

  mpz_clear(tmp);

  mpz_div_2exp(rop, rop, n / 2 + 1);

  return;
}

double coefficient_of_variation(uint64_t *arr, uint64_t n)
{
  double sigma = 0;
  double mu = 0;

  for (uint64_t i = 0; i < n; ++i)
    mu += (double)arr[i];
  mu /= n;

  // printf("mu = %lf = %e\n", mu, mu);

  for (uint64_t i = 0; i < n; ++i)
    sigma += (mu - (double)arr[i]) * (mu - (double)arr[i]);
  sigma = sqrt(1.0 / (double)n * sigma);
  // printf("sigma = %lf = %e\n", sigma, sigma);
  return sigma / mu;
}

double proba_without_latin_square(pow_m_sqr M)
{
  const uint64_t n = M.n;
  const uint64_t m = pow_m_sqr_sum_row(M, 0);
  uint64_t *arr = calloc(n * n, sizeof(uint64_t));
  for (uint64_t i = 0; i < n * n; ++i)
    arr[i] = ui_pow_ui(M_SQR_GET_AS_VEC(M, i), M.d);
  const double c = coefficient_of_variation(arr, n * n);
  free(arr);
  // printf("c = %lf\n", c);

  double f_mod = 1;

  for (uint32_t i = 0; i < sizeof(list_of_primes) / sizeof(*list_of_primes); ++i)
  {
    const uint64_t p = list_of_primes[i];
    uint64_t acc = 1;
    do
    {
      acc *= p;
    } while (acc * p <= lOCAL_BOUND);
    double corr = correction_factor(acc, M.arr, M.d, n, m, NUM_SAMPLES);
    f_mod *= corr;
  }

  f_mod *= f_mod;

  // printf("f_mod = %lf\n", f_mod);

  mpz_t perm;
  mpz_init(perm);
  n_perm(perm, n);
  uint64_t n_perm_ = mpz_get_ui(perm);
  mpz_clear(perm);

  double p_m = p_magic(m, n, c);
  // printf("p_magic = %e\n", p_m);
  // printf("1/m^2 = %e\n", 1.0 / ((double)m * (double)m));

  return p_m * f_mod * n_perm_;
}

double proba_with_latin_square(pow_m_sqr M, const uint32_t r, const uint32_t s)
{
  return proba_without_latin_square(M) * ui_pow_ui(A000479[r], s) * ui_pow_ui(A000479[s], r);
}

