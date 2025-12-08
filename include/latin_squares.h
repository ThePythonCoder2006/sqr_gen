/*
 * stub for compatibility reasons:
 * all the declarations were moved to "pow_m_sqr.h" to prevent include loops and because of strong code similarity
 */

#ifndef __LATIN_SQUARES__
#define __LATIN_SQUARES__

#include <stdint.h>

typedef struct
{
  uint32_t n;
  uint8_t *arr;
} latin_square;

#endif // __LATIN_SQUARES__