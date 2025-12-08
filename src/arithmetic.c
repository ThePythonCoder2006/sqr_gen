#include <stdint.h>

/*
 * return x^n
 */
uint64_t ui_pow_ui(uint64_t x, uint64_t n)
{
  uint64_t acc = 1;
  uint64_t a = x;
  while (n)
  {
    if (n & 0x1)
      acc *= a;
    a *= a;
    n >>= 1;
  }
  return acc;
}

