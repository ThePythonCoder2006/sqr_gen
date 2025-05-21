#ifndef __M_SQR__
#define __M_SQR__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "gmp.h"

typedef struct m_sqr_s
{
  uint64_t n;
  mpz_t *arr;
} m_sqr;

#define GET_AS_MAT(M, i, j) ((M).arr[(i) * ((M).n) + (j)])
#define GET_AS_VEC(M, idx) ((M).arr[(idx)])

void m_sqr_sum_col(mpz_t ret, m_sqr M, uint64_t j);
void m_sqr_sum_row(mpz_t ret, m_sqr M, uint64_t i);
void m_sqr_sum_diag1(mpz_t ret, m_sqr M);
void m_sqr_sum_diag2(mpz_t ret, m_sqr M);
void max_m_sqr(mpz_t max, m_sqr M);
uint8_t *nb_occurence_m_sqr(m_sqr M, uint64_t N);
uint8_t is_m_sqr(m_sqr M);
int m_sqr_init(m_sqr *M, uint64_t n);
void m_sqr_print(m_sqr M);
void m_sqr_clear(m_sqr *M);

#endif // __M_SQR__