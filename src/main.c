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
#include "find_taxicab.h"
#include "probas.h"
#include "serialize.h"
#include "taxicab_method.h"
#include "taxicab_method_mt.h"

#include "perf_counter.h"

size_t requiered_sets = 0;
size_t max_threads = DEFAULT_MAX_THREADS;
uint8_t use_multithreading = 0;
uint8_t new_taxicabs = 0;
double min_proba = 0;
#define DEFAUL_MIN_PROBA (.01)

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

  if (use_multithreading)
    printw("\nUsing MULTITHREADED search with %zu threads\n", max_threads);
  else
    printw("\nUsing single-threaded search\n");

  getch();
#else
  pow_m_sqr_printf(M);
  printf("\nis%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(M) ? "" : " not", M.d);

  printf("without latin squares: %e\n", p_no_latin);
  printf("with latin squares: %e\n", p_with_latin);
#endif

  if (use_multithreading)
    printf("\nUsing MULTITHREADED search with %zu threads\n", max_threads);
  else
    printf("\nUsing single-threaded search\n");

  return;
}

/*
 * returns 1 upon success
 */
int parse_args(int argc, char** argv)
{
  // shift out program name
  nob_shift(argv, argc);

  // Parse arguments
  while (argc > 0) {
    const char *arg = nob_shift(argv, argc);
    
    if (strcmp(arg, "-mt") == 0 || strcmp(arg, "--multithreaded") == 0)
      use_multithreading = true;
    else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--threads") == 0)
     {
      if (argc > 0)
      {
        max_threads = atoi(nob_shift(argv, argc));
        if (max_threads <= 0)
          max_threads = DEFAULT_MAX_THREADS;
      }
    }
    else if (strcmp(arg, "-n") == 0 || strcmp(arg, "--new-taxicabs") == 0)
    {
      new_taxicabs = true;  
      if (argc > 0)
      {
      min_proba = atof(nob_shift(argv, argc));
      if (min_proba <= 0)
        min_proba = DEFAUL_MIN_PROBA;
      }
    }
    else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
    {
      printf("Usage: ./main [OPTIONS] [REQUIRED_SETS]\n");
      printf("Options:\n");
      printf("  -mt, --multithreaded  Use multithreaded Latin square search\n");
      printf("  -t, --threads <N>     Set number of threads (default: %d)\n", DEFAULT_MAX_THREADS);
      printf("-n, --new-taxicabs <p>  Find new taxicabs with expected number of diagonals at least p");
      printf("  -h, --help        Show this help message\n");
      printf("\nRequired sets: Number of compatible sets to find (default: %llu)\n", REQUIERED_SETS);
      return 0;
    }
    else 
      // Try to parse as required sets number
      requiered_sets = atoi(arg);
  }

  return 1;
}

uint8_t print_latin_square_array(latin_square *P, uint64_t len, void *_);

int main(int argc, char **argv)
{
  if (!parse_args(argc, argv)) return 0;

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
  if (new_taxicabs)
  {
    taxicab_init(&a, 3, 4, 2);
    taxicab_init(&b, a.s, a.r, a.d);

    find_taxicabs_proba(a, b, min_proba);
  }
  else
    load_taxicabs("./know/12x12-2/", &a, "a", &b, "b");

  pow_m_sqr sq = {0};
  pow_m_sqr_init(&sq, a.r * a.s, a.d);

  pow_semi_m_sqr_from_taxicab(sq, a, b, NULL, NULL);

  show_starting_stats_on_square(a, b, sq);
 
  if (use_multithreading)
    search_pow_m_sqr_from_taxicabs_mt(sq, a, b, requiered_sets, max_threads);
  else
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

