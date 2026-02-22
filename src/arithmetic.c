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

/* Euclidean GCD of two non-negative integers. */
uint64_t gcd(uint64_t a, uint64_t b)
{
  while (b) { uint64_t t = b; b = a % b; a = t; }
  return a;
}

