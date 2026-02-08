#ifndef __POW_M_SQR__
#define __POW_M_SQR__

#include <stdint.h>

#include "perf_counter.h"
#include "taxicab.h"

#include "types.h"

// will evalutate `M` multiple times !!
#define M_SQR_GET_AS_MAT(M, i, j) ((M).arr[((M).rows[(i)]) * ((M).n) + ((M).cols[(j)])])
#define M_SQR_GET_AS_VEC(M, idx) ((M).arr[(idx)])

#define GET_AS_MAT(arr, i, j, width) ((arr)[(i) * (width) + (j)])

/* ------------ magic squares of powers ---------------- */

// init and conversion
int pow_m_sqr_init(pow_m_sqr *M, uint64_t n, uint64_t d);
void pow_m_sqr_clear(pow_m_sqr *M);
void highlighted_square_init(highlighted_square *ret, const uint32_t n, const uint32_t d);
void highlighted_square_clear(highlighted_square *ret);
void highlighted_square_from_pow_m_sqr(highlighted_square *ret, const pow_m_sqr *M, const rel_item *rel1, const rel_item *rel2);
void pow_m_sqr_from_highlighted_square(pow_m_sqr *ret, rel_item *rel1, rel_item *rel2, const highlighted_square *M);
void rels_from_highlighted_square(rel_item *rel1, rel_item *rel2, const highlighted_square *M);

// magic square checking
uint64_t pow_m_sqr_sum_col(pow_m_sqr M, uint64_t j);
uint64_t pow_m_sqr_sum_row(pow_m_sqr M, uint64_t i);
uint64_t pow_m_sqr_sum_diag1(pow_m_sqr M);
uint64_t pow_m_sqr_sum_diag2(pow_m_sqr M);
uint64_t max_pow_m_sqr(pow_m_sqr M);
uint8_t *nb_occurence_pow_m_sqr(pow_m_sqr M, uint64_t N);
uint8_t is_pow_m_sqr(pow_m_sqr M);
uint8_t is_pow_semi_m_sqr(pow_m_sqr M);

// IO
void mvpow_m_sqr_printw_highlighted(int y0, int x0, pow_m_sqr M, rel_item *items1, rel_item *items2, int BG_COLOR1, int BG_COLOR2);
void mvpow_m_sqr_printw(int y, int x, pow_m_sqr M);
void mvhighlighted_square_printw(int y0, int x0, highlighted_square M, int BG_COLOR1, int BG_COLOR2);
void pow_m_sqr_printf(pow_m_sqr M);

// magic square generation
int search_pow_m_sqr(pow_m_sqr base, uint64_t X, uint64_t progress, uint8_t *heat_map, perf_counter *perf);
void pow_semi_m_sqr_from_taxicab(pow_m_sqr M, taxicab a, taxicab b, latin_square *P, latin_square *Q);
void search_pow_m_sqr_from_pow_semi_m_sqr(pow_m_sqr M);
void generate_siamese(pow_m_sqr M);
int8_t parity_of_sets(uint32_t *rel1, uint32_t *rel2, const uint64_t n);

/* ------------ latin squares ---------------- */

void latin_square_init(latin_square *P, uint64_t n);
void latin_square_clear(latin_square *P);
void standart_latin_square(latin_square P);
void latin_square_printf(latin_square P);

#endif // __POW_M_SQR__

