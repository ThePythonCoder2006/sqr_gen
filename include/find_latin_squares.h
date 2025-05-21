#ifndef __FIND_LATIN_SQUARES__
#define __FIND_LATIN_SQUARES__

#include <stdint.h>

#include "latin_squares.h"

typedef void (*latin_square_callback)(latin_square);

bool iterate_over_all_square_callback(latin_square P, latin_square_callback callback);

#endif // __FIND_LATIN_SQUARES__