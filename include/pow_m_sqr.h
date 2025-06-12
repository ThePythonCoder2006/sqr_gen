#ifndef __POW_M_SQR__
#define __POW_M_SQR__

#include <stdlib.h>
#include <stdint.h>

#include "perf_counter.h"
#include "taxicab.h"
#include "latin_squares.h"

#include "gmp.h"

typedef struct pow_m_sqr_s
{
  uint32_t n, d;
  uint32_t *rows, *cols;
  uint64_t *arr;
} pow_m_sqr;

typedef struct
{
  uint32_t i, j;
} position;

/*
 * `.d` field is considered to not be set, its value is undefined
 */
typedef pow_m_sqr latin_square;

// will evalutate `M` multiple times !!
#define M_SQR_GET_AS_MAT(M, i, j) ((M).arr[((M).rows[(i)]) * ((M).n) + ((M).cols[(j)])])
#define M_SQR_GET_AS_VEC(M, idx) ((M).arr[(idx)])

/* ------------ magic squares of powers ---------------- */

// init
int pow_m_sqr_init(pow_m_sqr *M, uint64_t n, uint64_t d);
void pow_m_sqr_clear(pow_m_sqr *M);

// magic square checking
uint64_t pow_m_sqr_sum_col(pow_m_sqr M, uint64_t j);
uint64_t pow_m_sqr_sum_row(pow_m_sqr M, uint64_t i);
uint64_t pow_m_sqr_sum_diag1(pow_m_sqr M);
uint64_t pow_m_sqr_sum_diag2(pow_m_sqr M);
uint64_t max_pow_m_sqr(pow_m_sqr M);
uint8_t *nb_occurence_pow_m_sqr(pow_m_sqr M, uint64_t N);
uint8_t is_pow_m_sqr(pow_m_sqr M);
uint8_t is_pow_semi_m_sqr(pow_m_sqr M);

void permute_into_pow_m_sqr(pow_m_sqr M, position *diag1, position *diag2);

// IO
void mvpow_m_sqr_printw_highlighted(int y0, int x0, pow_m_sqr M, position *items1, position *items2, int BG_COLOR1, int BG_COLOR2);
void mvpow_m_sqr_printw(int y, int x, pow_m_sqr M);
void pow_m_sqr_printf(pow_m_sqr M);

// magic square generation
int search_pow_m_sqr(pow_m_sqr base, uint64_t X, uint64_t progress, uint8_t *heat_map, perf_counter *perf);
void pow_semi_m_sqr_from_taxicab(pow_m_sqr M, taxicab a, taxicab b, latin_square *P, latin_square *Q);
void search_pow_m_sqr_from_pow_semi_m_sqr(pow_m_sqr M);
void generate_siamese(pow_m_sqr M);
void search_pow_m_sqr_from_taxicabs(pow_m_sqr M, taxicab a, taxicab b);
int8_t parity_of_sets(position *rel1, position *rel2, const uint64_t n);

/* ------------ latin squares ---------------- */

void latin_square_init(latin_square *P, uint64_t n);
void latin_square_clear(latin_square *P);
void standart_latin_square(latin_square P);

#endif // __POW_M_SQR__