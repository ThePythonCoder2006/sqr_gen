#ifndef __FIND_LATIN_SQUARES__
#define __FIND_LATIN_SQUARES__

#include <stdint.h>

#include "latin_squares.h"

typedef uint8_t (*latin_square_callback)(latin_square *, void *);
typedef uint8_t (*latin_square_array_callback)(latin_square *, uint64_t, void *);

uint8_t iterate_over_all_square_callback(latin_square *P, latin_square_callback callback, void *data);
uint8_t iterate_over_all_square_array_callback(latin_square *P, uint64_t len, latin_square_array_callback f, void *data);

#endif // __FIND_LATIN_SQUARES__