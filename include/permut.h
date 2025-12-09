#ifndef __PERMUT__
#define __PERMUT__

#include <stdint.h>

#include "latin_squares.h"
#include "pow_m_sqr.h"

void position_after_latin_square_permutation(uint32_t *ret_row, uint32_t *ret_col, uint32_t row, uint32_t col, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
uint8_t fall_on_different_line_after_latin_squares(uint32_t *poses, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s);
void permute_into_pow_m_sqr(pow_m_sqr *M, uint32_t *diag1, uint32_t *diag2);

#endif // __PERMUT__
