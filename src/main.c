#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include "m_sqr.h"
#include "pow_m_sqr.h"
#include "taxicab.h"
#include "find_taxicab.h"
#include "find_sets.h"

#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"

#include "GMP.h"
#include "curses.h"

void random_perm(uint64_t *l, size_t n);

int main2(int argc, char **argv)
{
  (void)argc, (void)argv;

  pow_m_sqr test4_2 = {0};
  pow_m_sqr_init(&test4_2, 4, 2);

  M_SQR_GET_AS_MAT(test4_2, 0, 0) = 11;
  M_SQR_GET_AS_MAT(test4_2, 0, 1) = 59;
  M_SQR_GET_AS_MAT(test4_2, 0, 2) = 17;
  M_SQR_GET_AS_MAT(test4_2, 0, 3) = 68;

  M_SQR_GET_AS_MAT(test4_2, 1, 0) = 77;
  M_SQR_GET_AS_MAT(test4_2, 1, 1) = 28;
  M_SQR_GET_AS_MAT(test4_2, 1, 2) = 31;
  M_SQR_GET_AS_MAT(test4_2, 1, 3) = 29;

  M_SQR_GET_AS_MAT(test4_2, 2, 0) = 8;
  M_SQR_GET_AS_MAT(test4_2, 2, 1) = 23;
  M_SQR_GET_AS_MAT(test4_2, 2, 2) = 79;
  M_SQR_GET_AS_MAT(test4_2, 2, 3) = 41;

  M_SQR_GET_AS_MAT(test4_2, 3, 0) = 49;
  M_SQR_GET_AS_MAT(test4_2, 3, 1) = 61;
  M_SQR_GET_AS_MAT(test4_2, 3, 2) = 32;
  M_SQR_GET_AS_MAT(test4_2, 3, 3) = 37;

  initscr();
  mvpow_m_sqr_print(0, 0, test4_2);
  refresh();

  // printw("%s\n", is_pow_m_sqr(test4_2) ? "is a magic square of squares" : "is not a magic square of squares");

  perf_counter perf = {.boards_tested = 0};
  timer_start(&(perf.time));
  search_pow_m_sqr(test4_2, 79, 5, NULL, &perf);
  clear();
  printw("global average speed was: ");
  print_perfw(perf, "grids");
  printw(". Time taken: %lf s", timer_stop(&(perf.time)));

  mvpow_m_sqr_print(1, 0, test4_2);
  printw("%s\n", is_pow_m_sqr(test4_2) ? "is a magic square of squares" : "is not a magic square of squares");
  refresh();

  getch();
  endwin();

  pow_m_sqr_clear(&test4_2);

  return 0;
}

int main(int argc, char **argv)
{
  (void)argc, (void)argv;
  srand(69);

  taxicab a = {0};
  taxicab_init(&a, 2, 3, 2);
  find_taxicab(a);
  // printf("%u\n", __LINE__);

  // TAXI_GET_AS_MAT(a, 0, 0) = 2;
  // TAXI_GET_AS_MAT(a, 0, 1) = 21;
  // TAXI_GET_AS_MAT(a, 0, 2) = 29;
  // TAXI_GET_AS_MAT(a, 0, 3) = 32;

  // TAXI_GET_AS_MAT(a, 1, 0) = 7;
  // TAXI_GET_AS_MAT(a, 1, 1) = 23;
  // TAXI_GET_AS_MAT(a, 1, 2) = 24;
  // TAXI_GET_AS_MAT(a, 1, 3) = 34;

  // TAXI_GET_AS_MAT(a, 2, 0) = 8;
  // TAXI_GET_AS_MAT(a, 2, 1) = 9;
  // TAXI_GET_AS_MAT(a, 2, 2) = 16;
  // TAXI_GET_AS_MAT(a, 2, 3) = 37;

  // TAXI_GET_AS_MAT(a, 3, 0) = 14;
  // TAXI_GET_AS_MAT(a, 3, 1) = 26;
  // TAXI_GET_AS_MAT(a, 3, 2) = 27;
  // TAXI_GET_AS_MAT(a, 3, 3) = 31;

  taxicab b = {0};
  taxicab_init(&b, 3, 2, 2);
  do
    find_taxicab(b);
  while (!taxicab_cross_products_are_distinct(a, b));

  // TAXI_GET_AS_MAT(b, 0, 0) = 3;
  // TAXI_GET_AS_MAT(b, 0, 1) = 85;
  // TAXI_GET_AS_MAT(b, 0, 2) = 97;
  // TAXI_GET_AS_MAT(b, 0, 3) = 116;

  // TAXI_GET_AS_MAT(b, 1, 0) = 23;
  // TAXI_GET_AS_MAT(b, 1, 1) = 25;
  // TAXI_GET_AS_MAT(b, 1, 2) = 98;
  // TAXI_GET_AS_MAT(b, 1, 3) = 123;

  // TAXI_GET_AS_MAT(b, 2, 0) = 43;
  // TAXI_GET_AS_MAT(b, 2, 1) = 81;
  // TAXI_GET_AS_MAT(b, 2, 2) = 95;
  // TAXI_GET_AS_MAT(b, 2, 3) = 118;

  // TAXI_GET_AS_MAT(b, 3, 0) = 45;
  // TAXI_GET_AS_MAT(b, 3, 1) = 73;
  // TAXI_GET_AS_MAT(b, 3, 2) = 106;
  // TAXI_GET_AS_MAT(b, 3, 3) = 113;

#if 0
  initscr();
  mvtaxicab_print(0, 0, T_4_4_3);
  printw("is%s a (4, 4, 3)-taxicab\n", is_taxicab(T_4_4_3) ? "" : " not");

  perf_counter perf = {.boards_tested = 0};
  timer_start(&(perf.time));
  search_taxicab(T_4_4_3, 28, 0, NULL, &perf);

  clear();
  printw("global average speed was: ");
  print_perfw(perf, "taxicabs");
  printw(". Time taken: %lf s", timer_stop(&(perf.time)));

  mvtaxicab_print(1, 0, T_4_4_3);
  printw("is%s a taxicab\n", is_taxicab(T_4_4_3) ? "" : " not");

  getch();
  clear();
  mvtaxicab_print(0, 30, T_4_3_4);
  printw("is%s a (4, 3, 4)-taxicab\n", is_taxicab(T_4_3_4) ? "" : " not");
  refresh();

  timer_start(&(perf.time));
  search_taxicab(T_4_3_4, 60, 0, NULL, &perf);

  clear();
  printw("global average speed was: ");
  print_perfw(perf, "taxicabs");
  printw(". Time taken: %lf s", timer_stop(&(perf.time)));

  mvtaxicab_print(1, 0, T_4_3_4);
  printw("is%s a taxicab\n", is_taxicab(T_4_3_4) ? "" : " not");

  getch();
  clear();
#endif

  // taxicab a = {0};
  // taxicab_init(&a, 4, 4, 4);

#ifndef __DEBUG__
  initscr();
  // printf("%u\n", __LINE__);
  clear();
  mvtaxicab_print(0, 0, a);
  // printf("%u\n", __LINE__);
  printw("is%s a (%llu, %llu, %llu)-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
  // printf("%u\n", __LINE__);
  mvtaxicab_print(0, 30, b);
  // printf("%u\n", __LINE__);
  printw("is%s a (%llu, %llu, %llu)-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
  // printf("%u\n", __LINE__);
  getch();
  // printw("global average speed was: ");
  // print_perfw(perf, "taxicabs");
  // printw(". Time taken: %lf s", timer_stop(&(perf.time)));
  // getch();

#endif

  // mvtaxicab_print(1, 0, T_2_2_2);
  // printw("is%s a taxicab\n", is_taxicab(T_2_2_2) ? "" : " not");

  // getch();

  pow_m_sqr sq = {0};
  pow_m_sqr_init(&sq, 6, 2);

  pow_semi_m_sqr_from_taxicab(sq, a, b, NULL, NULL);

#ifndef __DEBUG__
  clear();
  mvpow_m_sqr_print(0, 0, sq);
  printw("is%s a semi magic square of %u-th powers", is_pow_semi_m_sqr(sq) ? "" : " not", sq.d);

  getch();
#else
  printf("is%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(sq) ? "" : " not", sq.d);
#endif

  // search_pow_m_sqr_from_pow_semi_m_sqr(sq_9_2);
  // #ifndef __DEBUG__
  //   clear();
  //   mvpow_m_sqr_print(0, 0, sq);
  //   printw("is%s a magic square of %u-th powers", is_pow_m_sqr(sq) ? "" : " not", sq.d);
  //   getch();

  //   endwin();
  // #endif

  search_pow_m_sqr_from_taxicabs(sq, a, b);

  pow_m_sqr_clear(&sq);

  taxicab_clear(&a);
  taxicab_clear(&b);
  return 0;
}
