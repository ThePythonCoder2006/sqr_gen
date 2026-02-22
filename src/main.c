#include <ncurses.h>

#include <assert.h>
#define NOB_STRIP_PREFIX
#include "nob.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include "pow_m_sqr.h"
#include "taxicab.h"
#include "probas.h"
#include "serialize.h"
#include "taxicab_method.h"

#include "perf_counter.h"

// #include "know/16x16x4.h"

void show_starting_stats_on_square(const taxicab a, const taxicab b, const pow_m_sqr M)
{

#ifndef __NO_GUI__
  clear();
  mvtaxicab_print(0, 0, a);
  printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
  mvtaxicab_print(0, 30, b);
  printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
  refresh();
  getch();
#else
  taxicab_printf(a);
  printf("\nis%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
  taxicab_printf(b);
  printf("\nis%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
#endif

  const double p_no_latin = proba_without_latin_square(M);
  const double p_with_latin = proba_with_latin_square(M, a.r, a.s);

#ifndef __NO_GUI__
  clear();
  mvpow_m_sqr_printw(0, 0, M);
  printw("is%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(M) ? "" : " not", M.d);
  printw("without latin squares: %e\n", p_no_latin);
  printf("------------------\n");
  printw("with latin squares: %e\n", p_with_latin);

  getch();
#else
  pow_m_sqr_printf(M);
  printf("\nis%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(M) ? "" : " not", M.d);

  printf("without latin squares: %e\n", p_no_latin);
  printf("with latin squares: %e\n", p_with_latin);
#endif

  return;
}

uint8_t print_latin_square_array(latin_square *P, uint64_t len, void *_);

int main(int argc, char **argv)
{
  size_t requiered_sets = 0;

  // shift out program name
  nob_shift(argv, argc);

  if (argc > 0)
    requiered_sets = atoi(nob_shift(argv, argc));

  srand(time(NULL));

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

  taxicab a = {0};
  taxicab b = {0};
#if __FIND_NEW_TAXICABS__
  taxicab_init(&a, 3, 4, 2);
  taxicab_init(&b, a.s, a.r, a.d);

  find_taxicabs_proba(a, b, .01);
#else
  load_taxicabs("./know/12x12-2/", &a, "a", &b, "b");
#endif

  pow_m_sqr sq = {0};
  pow_m_sqr_init(&sq, a.r * a.s, a.d);

  pow_semi_m_sqr_from_taxicab(sq, a, b, NULL, NULL);

  show_starting_stats_on_square(a, b, sq);

  search_pow_m_sqr_from_taxicabs(sq, a, b, requiered_sets);

#ifndef __NO_GUI__
  endwin();
#endif

  pow_m_sqr_clear(&sq);

  taxicab_clear(&a);
  taxicab_clear(&b);
  return 0;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"

