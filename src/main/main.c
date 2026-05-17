#include "types.h"
#include <ncurses.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

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
uint8_t use_taxicab_method = 1;
uint8_t use_multithreading = 0;
uint8_t new_taxicabs = 0;
double min_proba = 0;
uint64_t max_sum = UINT64_MAX;
uint8_t regen_latin_square_list = 0;
#define DEFAUL_MIN_PROBA (.00001)

void show_starting_stats_on_square(const taxicab a, const taxicab b, const pow_m_sqr M);
int parse_args(int argc, char** argv);

uint8_t print_latin_square_array(latin_square *P, uint64_t len, void *_);

int main(int argc, char **argv)
{
  if (!parse_args(argc, argv)) return 0;

  // srand(69);
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

  const uint32_t r = 4, s = 4, d = 2;

  char base_file_name[256] = {0};
  get_file_name_identifier(base_file_name, 255,
      temp_sprintf("output/%s-out-", use_taxicab_method ? "taxi" : "method"),
      "/");
  if (!mkdir_if_not_exists(base_file_name)) exit(1);

  FILE* info = fopen(temp_sprintf("%sinfo.txt", base_file_name), "w");
  if (info == NULL)
  {
    fprintf(stderr, "[ERORR] Could not open file %sinfo.txt: %s", base_file_name, strerror(errno));
    exit(1);
  }
  fprintf(info, "r=%u,s=%u,d=%u\n", r, s, d);

  perf_counter perf = {0};
  perf_counter_init(&perf, 5.0);

  if (regen_latin_square_list || !file_exists("./squares.latin_square"))
  {
    latin_square* P = calloc(r, sizeof(latin_square));
    latin_square* Q = calloc(s, sizeof(latin_square));
    for (uint32_t i = 0; i < r; ++i)
      latin_square_init(P + i, s);
    for (uint32_t j = 0; j < s; ++j)
      latin_square_init(Q + j, r);

    save_all_latin_square_arrays("./", P, Q, r, s, "squares");

    for (uint32_t i = 0; i < r; ++i)
      latin_square_clear(P + i);
    for (uint32_t j = 0; j < s; ++j)
      latin_square_clear(Q + j);
    free(P);
    free(Q);
  }

  taxicab a = {0};
  taxicab b = {0};
  new_taxicabs = new_taxicabs || r != 3 || s != 4;
  if (new_taxicabs)
  {
    taxicab_init(&a, r, s, d);
    taxicab_init(&b, s, r, d);

    find_taxicabs_condition(a, b, min_proba, max_sum);
  }
  else
    load_taxicabs("./know/12x12-2/", &a, "a", &b, "b");

  const double taxicab_time = timer_stop(&perf.time);
  fprintf(info, "%s the taxicabs took %fs\n", new_taxicabs ? "finding" : "loading", taxicab_time);

  pow_m_sqr sq = {0};
  pow_m_sqr_init(&sq, a.r * a.s, a.d);

  pow_semi_m_sqr_from_taxicab(sq, a, b, NULL, NULL);

  show_starting_stats_on_square(a, b, sq);

  save_taxicabs(base_file_name, a, "a", b, "b");

  if (use_taxicab_method)
  {
    if (use_multithreading)
      search_pow_m_sqr_from_taxicabs_mt(&perf, base_file_name, sq, a, b, requiered_sets, max_threads);
    else
      search_pow_m_sqr_from_taxicabs(&perf, base_file_name, sq, a, b, requiered_sets);
  }
  else
  {
    method m = choose_method(r * s, pow_m_sqr_sum_row(sq, 0));
    switch (m)
    {
    case METHOD_MU_SQUARED:
      semi_to_full_naive(&perf, sq);
      break;
    case METHOD_MU:
      semi_to_full_simultanious_perm(&perf, sq);
      break;
    case METHOD_SQRT_MU:
      TODO("sqrt(mu)");
      break;
    case METHOD_NONE:
    case METHOD_COUNT:
      fprintf(stderr, "[ERROR] No possible method\n");
      exit(1);
    }

#ifndef __NO_GUI__
      printw("is%s a magic square of %u-th powers\n", is_pow_m_sqr(sq) ? "" : " not", sq.d);
      getch();
#else
      printf("is%s a magic square of %u-th powers\n", is_pow_m_sqr(sq) ? "" : " not", sq.d);
#endif
  }

  save_pow_m_sqr(base_file_name, sq, "sq");
  fprintf(info, "Completing the square took %fs\n", timer_stop(&perf.time) - taxicab_time);

#ifndef __NO_GUI__
  endwin();
#endif

  fclose(info);
  perf_counter_clear(&perf);
  pow_m_sqr_clear(&sq);

  taxicab_clear(&a);
  taxicab_clear(&b);
  return 0;
}

void show_starting_stats_on_square(const taxicab a, const taxicab b, const pow_m_sqr M)
{
#ifndef __NO_GUI__
  clear();
  mvtaxicab_print(0, 0, a);
  printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
  mvtaxicab_print(0, 40, b);
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

  const uint64_t mu = taxicab_sum_row(a, 0) * taxicab_sum_row(b, 0);

#ifndef __NO_GUI__
  clear();
  if (M.n <= 16)
    mvpow_m_sqr_printw(0, 0, M);
  printw("is%s a semi magic square of %u-th powers with magic constant mu = %"PRIu64"\n", is_pow_semi_m_sqr(M) ? "" : " not", M.d, mu);
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
  while (argc > 0)
  {
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
        max_sum = atoi(nob_shift(argv, argc));
      }
    }
    else if (strcmp(arg, "--regen") == 0)
      regen_latin_square_list = true;
    else if (strcmp(arg, "--no-taxicab-method") == 0)
      use_taxicab_method = 0;
    else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
    {
      printf("Usage: ./main [OPTIONS] [REQUIRED_SETS]\n");
      printf("Options:\n");
      printf("  -mt, --multithreaded  Use multithreaded Latin square search\n");
      printf("  -t, --threads <N>     Set number of threads (default: %d)\n", DEFAULT_MAX_THREADS);
      printf("-n, --new-taxicabs <p>  Find new taxicabs with expected number of diagonals at least p");
      printf("-regen                  regenerates the file holding the latin square arrays list\n");
      printf("  -h, --help        Show this help message\n");
      printf("\nRequired sets: Number of compatible sets to find (default: %lu)\n", REQUIERED_SETS);
      return 0;
    }
    else
      // Try to parse as required sets number
      requiered_sets = atoi(arg);
  }

  return 1;
}


#define NOB_IMPLEMENTATION
#include "nob.h"
#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"

