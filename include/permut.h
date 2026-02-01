#ifndef __PERMUT__
#define __PERMUT__

#include <stdint.h>

#include "latin_squares.h"
#include "pow_m_sqr.h"

void position_after_latin_square_permutation(uint32_t *ret_row, uint32_t *ret_col, uint32_t row, uint32_t col, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
uint8_t fall_on_different_line_after_latin_squares(rel_item *poses, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
void permute_into_pow_m_sqr(pow_m_sqr *M, rel_item *diag1, rel_item *diag2);
uint8_t rels_are_disjoint(rel_item *rel1, rel_item *rel2, const size_t n);
uint8_t rels_are_compatible(rel_item *rel1, rel_item *rel2, const size_t n);
void printf_rel(rel_item *rel, const size_t n);
uint8_t rels_are_diagonizable(rel_item* rel1, rel_item* rel2, rel_item* rel1_inv, rel_item* sigma, size_t n);

#endif // __PERMUT__
