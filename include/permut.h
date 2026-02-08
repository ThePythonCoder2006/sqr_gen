#ifndef __PERMUT__
#define __PERMUT__

#include <stdint.h>

#include "types.h"

void position_after_latin_square_permutation(uint32_t *ret_row, uint32_t *ret_col, uint32_t row, uint32_t col, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
uint8_t fall_on_different_line_after_latin_squares(uint8_t* rows, uint8_t* cols, rel_item *poses, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
uint8_t rels_are_diagonizable(rel_item* rel1, rel_item* rel2, rel_item* rel1_inv, rel_item* sigma, size_t n);
void permute_into_pow_m_sqr(pow_m_sqr *M, rel_item *diag1, rel_item *diag2);
uint8_t rels_are_disjoint(rel_item *rel1, rel_item *rel2, const size_t n);
uint8_t rels_are_compatible(rel_item *rel1, rel_item *rel2, const size_t n);
void printf_rel(rel_item *rel, const size_t n);
void pos_rel_to_x_y_rel(x_y_rel ret, pos_rel op, const size_t n);
void x_y_rel_after_latin_squares(x_y_rel ret, pos_rel op, latin_square* P, latin_square* Q, const size_t r, const size_t s);

#endif // __PERMUT__
