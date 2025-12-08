#ifndef __PROBAS__
#define __PROBAS__

#include <stdint.h>

#include "pow_m_sqr.h"

double proba_without_latin_square(pow_m_sqr M);
double proba_with_latin_square(pow_m_sqr M, const uint32_t r, const uint32_t s);

#endif
