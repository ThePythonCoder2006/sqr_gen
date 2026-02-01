#include <ncurses.h>

#include <assert.h>
#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include "pow_m_sqr.h"
#include "taxicab.h"
#include "probas.h"
#include "latin_squares.h"
#include "find_taxicab.h"
#include "permut.h"

#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"

// #include "known/16x16x4.h"


uint8_t print_latin_square_array(latin_square *P, uint64_t len, void *_);

int main(int argc, char **argv)
{
  (void)argc, (void)argv;
  // srand(time(NULL));

#ifndef __NO_GUI__
  initscr();
  
  if (!has_colors())
  {
    fprintf(stderr, "[ERROR] Your terminal does not support color!\n");
    endwin();
    exit(1);
  }

  start_color();
#endif

  srand(69);

  taxicab a = {0};
  taxicab_init(&a, 3, 4, 2);
  find_taxicab(a);

#if 0
  TAXI_GET_AS_MAT(a, 0, 0) = 2;
  TAXI_GET_AS_MAT(a, 0, 1) = 21;
  TAXI_GET_AS_MAT(a, 0, 2) = 29;
  TAXI_GET_AS_MAT(a, 0, 3) = 32;

  TAXI_GET_AS_MAT(a, 1, 0) = 7;
  TAXI_GET_AS_MAT(a, 1, 1) = 23;
  TAXI_GET_AS_MAT(a, 1, 2) = 24;
  TAXI_GET_AS_MAT(a, 1, 3) = 34;

  TAXI_GET_AS_MAT(a, 2, 0) = 8;
  TAXI_GET_AS_MAT(a, 2, 1) = 9;
  TAXI_GET_AS_MAT(a, 2, 2) = 16;
  TAXI_GET_AS_MAT(a, 2, 3) = 37;

  TAXI_GET_AS_MAT(a, 3, 0) = 14;
  TAXI_GET_AS_MAT(a, 3, 1) = 26;
  TAXI_GET_AS_MAT(a, 3, 2) = 27;
  TAXI_GET_AS_MAT(a, 3, 3) = 31;
#endif

  /*
  TAXI_GET_AS_MAT(a, 0, 0) = 2;
  TAXI_GET_AS_MAT(a, 0, 1) = 16;
  TAXI_GET_AS_MAT(a, 0, 2) = 21;
  TAXI_GET_AS_MAT(a, 0, 3) = 25;

  TAXI_GET_AS_MAT(a, 1, 0) = 5;
  TAXI_GET_AS_MAT(a, 1, 1) = 11;
  TAXI_GET_AS_MAT(a, 1, 2) = 12;
  TAXI_GET_AS_MAT(a, 1, 3) = 28;

  TAXI_GET_AS_MAT(a, 2, 0) = 13;
  TAXI_GET_AS_MAT(a, 2, 1) = 19;
  TAXI_GET_AS_MAT(a, 2, 2) = 20;
  TAXI_GET_AS_MAT(a, 2, 3) = 24;
  */

  taxicab b = {0};
  taxicab_init(&b, a.s, a.r, a.d);
  do
   find_taxicab(b);
  while (!taxicab_cross_products_are_distinct(a, b));
#if 0

  TAXI_GET_AS_MAT(b, 0, 0) = 3;
  TAXI_GET_AS_MAT(b, 0, 1) = 85;
  TAXI_GET_AS_MAT(b, 0, 2) = 97;
  TAXI_GET_AS_MAT(b, 0, 3) = 116;

  TAXI_GET_AS_MAT(b, 1, 0) = 23;
  TAXI_GET_AS_MAT(b, 1, 1) = 25;
  TAXI_GET_AS_MAT(b, 1, 2) = 98;
  TAXI_GET_AS_MAT(b, 1, 3) = 123;

  TAXI_GET_AS_MAT(b, 2, 0) = 43;
  TAXI_GET_AS_MAT(b, 2, 1) = 81;
  TAXI_GET_AS_MAT(b, 2, 2) = 95;
  TAXI_GET_AS_MAT(b, 2, 3) = 118;

  TAXI_GET_AS_MAT(b, 3, 0) = 45;
  TAXI_GET_AS_MAT(b, 3, 1) = 73;
  TAXI_GET_AS_MAT(b, 3, 2) = 106;
  TAXI_GET_AS_MAT(b, 3, 3) = 113;
#endif

  /*
  TAXI_GET_AS_MAT(b, 0, 0) = 2;
  TAXI_GET_AS_MAT(b, 0, 1) = 71;
  TAXI_GET_AS_MAT(b, 0, 2) = 73;

  TAXI_GET_AS_MAT(b, 1, 0) = 17;
  TAXI_GET_AS_MAT(b, 1, 1) = 62;
  TAXI_GET_AS_MAT(b, 1, 2) = 79;

  TAXI_GET_AS_MAT(b, 2, 0) = 29;
  TAXI_GET_AS_MAT(b, 2, 1) = 53;
  TAXI_GET_AS_MAT(b, 2, 2) = 82;

  TAXI_GET_AS_MAT(b, 3, 0) = 37;
  TAXI_GET_AS_MAT(b, 3, 1) = 46;
  TAXI_GET_AS_MAT(b, 3, 2) = 83;
  */


#ifndef __NO_GUI__
  clear();
  mvtaxicab_print(0, 0, a);
  printw("is%s a (%"PRIu64", %"PRIu64", %"PRIu64")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
  mvtaxicab_print(0, 30, b);
  printw("is%s a (%"PRIu64", %"PRIu64", %"PRIu64")-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
  getch();
#else
  taxicab_printf(a);
  printf("\nis%s a (%"PRIu64", %"PRIu64", %"PRIu64")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
  taxicab_printf(b);
  printf("\nis%s a (%"PRIu64", %"PRIu64", %"PRIu64")-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
#endif

  pow_m_sqr sq = {0};
  pow_m_sqr_init(&sq, a.r * a.s, a.d);

  pow_semi_m_sqr_from_taxicab(sq, a, b, NULL, NULL);

//   uint32_t rel1[6] = {0, 2, 5, 1, 3, 4};
//   uint32_t rel2[6] = {3, 4, 1, 5, 0, 2};
// #ifndef __DEBUG__
//   clear();
//   mvpow_m_sqr_printw_highlighted(0, 0, sq, rel1, rel2, COLOR_YELLOW, COLOR_CYAN);
//   printw("is%s a semi magic square of %u-th powers", is_pow_semi_m_sqr(sq) ? "" : " not", sq.d);

//   getch();
// #else
//   pow_m_sqr_printf(sq);
//   printf("\nis%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(sq) ? "" : " not", sq.d);
// #endif

  const double p_no_latin = proba_without_latin_square(sq);
  const double p_with_latin = proba_with_latin_square(sq, a.r, a.s);

#ifndef __NO_GUI__
  clear();
  mvpow_m_sqr_printw(0, 0, sq);
  printw("is%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(sq) ? "" : " not", sq.d);
  printw("without latin squares: %e\n", p_no_latin);
  printf("------------------\n");
  printw("with latin squares: %e\n", p_with_latin);

  getch();
#else
  pow_m_sqr_printf(sq);
  printf("\nis%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(sq) ? "" : " not", sq.d);

  printf("without latin squares: %e\n", p_no_latin);
  printf("with latin squares: %e\n", p_with_latin);
#endif

  search_pow_m_sqr_from_taxicabs(sq, a, b);

#ifndef __NO_GUI__
  endwin();
#endif

  pow_m_sqr_clear(&sq);

  taxicab_clear(&a);
  taxicab_clear(&b);
  return 0;
}
