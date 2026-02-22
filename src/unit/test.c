#include <assert.h>
#include <stdio.h>

#include <ncurses.h>

#include "permut.h"
#include "pow_m_sqr.h"
#include "taxicab.h"

#include "know/16x16x4.h"

int main(int argc, char** argv)
{
  (void) argc, (void) argv;

  taxicab a, b;
  taxicab_init(&a, r, s, d);
  taxicab_init(&b, s, r, d);

#if (!defined __NO_GUI__) && 0
  initscr();
  
  if (!has_colors())
  {
    fprintf(stderr, "[ERROR] Your terminal does not support color!\n");
    endwin();
    exit(1);
  }

  start_color();
#endif

  load_a(&a);
  load_b(&b);

  rel_item inv[16], sigma[16];
  assert(rels_are_diagonizable(rel1, rel2, inv, sigma, 16));
  printf("[OK] rels_are_diagonizable\n");

  pow_m_sqr M;
  pow_m_sqr_init(&M, 16, 4);
  pow_semi_m_sqr_from_taxicab(M, a, b, P, Q);

#if !(defined __NO_GUI__) && 0
  mvpow_m_sqr_printw_highlighted(0, 0, M, rel1, rel2, COLOR_YELLOW, COLOR_BLUE);
  printw("is%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(M) ? "" : " not", M.d);
  getch();
#endif

  permute_into_pow_m_sqr(&M, rel1, rel2);

  assert(is_pow_m_sqr(M));
  printf("[OK] permute_into_pow_m_sqr\n");

#if !(defined __NO_GUI__) && 0
  rel_item main_diag[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  rel_item anti_diag[16] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  mvpow_m_sqr_printw_highlighted(0, 0, M, main_diag, anti_diag, COLOR_YELLOW, COLOR_BLUE);
  printw("is%s a magic square of %u-th powers", is_pow_m_sqr(M) ? "" : " not", M.d);
  getch();
#endif

#if !(defined __NO_GUI__) && 0
  endwin();
#endif

  return 0;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"
