#include <stdint.h>

#include "taxicab.h"
#include "latin_squares.h"

const uint64_t r = 4, s = 4, d = 4;

uint64_t a_arr[] = {  2, 21, 29, 32,
                      7, 23, 24, 34,
                      8,  9, 16, 37,
                    14, 26, 27, 31
};

taxicab a = {.d = d, .r = r, .s = s, .arr = a_arr};

uint64_t b_arr[] = { 3, 85,  97, 116,
                    23, 25,  98, 123,
                    43, 81,  95, 118,
                    45, 73, 106, 113
   };

// b has transposed dimentions from a
taxicab b = {.d = d, .r = s, .s = r, .arr = b_arr};

uint8_t P_0_arr[] = {0, 1, 2, 3,
                     3, 2, 1, 0,
                     1, 0, 3, 2,
                     2, 3, 0, 1
};

uint8_t P_1_arr[] = {0, 1, 2, 3,
                     3, 0, 1, 2,
                     1, 2, 3, 0,
                     2, 3, 0, 1
};

uint8_t P_2_arr[] = {0, 1, 2, 3,
                     2, 0, 3, 1,
                     1, 3, 0, 2,
                     3, 2, 1, 0
};

uint8_t P_3_arr[] = {0, 1, 2, 3,
                     3, 2, 0, 1,
                     1, 0, 3, 2,
                     2, 3, 1, 0
};

latin_square P[] = {
  (latin_square){.n = s, .arr = P_0_arr},
  (latin_square){.n = s, .arr = P_1_arr},
  (latin_square){.n = s, .arr = P_2_arr},
  (latin_square){.n = s, .arr = P_3_arr}
};

uint8_t Q_0_arr[] = {0, 1, 2, 3,
                     2, 0, 3, 1,
                     3, 2, 1, 0,
                     1, 3, 0, 2
};

uint8_t Q_1_arr[] = {0, 1, 2, 3,
                     3, 2, 0, 1,
                     1, 0, 3, 2,
                     2, 3, 1, 0
};

uint8_t Q_2_arr[] = {0, 1, 2, 3,
                     2, 0, 3, 1,
                     1, 3, 0, 2,
                     3, 2, 1, 0
};

uint8_t Q_3_arr[] = {0, 1, 2, 3,
                     1, 0, 3, 2,
                     3, 2, 1, 0,
                     2, 3, 0, 1
};


latin_square Q[] = {
  (latin_square){.n = r, .arr = Q_0_arr},
  (latin_square){.n = r, .arr = Q_1_arr},
  (latin_square){.n = r, .arr = Q_2_arr},
  (latin_square){.n = r, .arr = Q_3_arr}
};
