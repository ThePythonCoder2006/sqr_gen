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

#include "flag.h"

typedef struct run_data_s
{
  size_t requiered_sets;
  size_t max_threads;
  bool no_taxicab_method;
  bool use_multithreading;
  bool new_taxicabs;
  double min_proba;
  uint64_t max_sum;
  bool regen_latin_square_list;
  bool help;
  uint64_t r, s, d;
  char base_file_name[256];
  method m;
  FILE* info;
  taxicab a, b;
  pow_m_sqr sq;
  perf_counter perf;
} run_data;
#define DEFAUL_MIN_PROBA (.00001)
#define DEFAULT_MAX_SUM UINT64_MAX

void show_starting_stats_on_square(run_data* run);
void exit_fun(void);
int parse_args(int argc, char** argv, run_data* run);
uint8_t print_latin_square_array(latin_square *P, uint64_t len, void *_);

int do_run(run_data* run);

int main(int argc, char **argv)
{
  run_data run = {.requiered_sets = 0, .max_threads = DEFAULT_MAX_THREADS, .no_taxicab_method = false,
    .use_multithreading = false, .new_taxicabs = false, .min_proba = DEFAUL_MIN_PROBA, .max_sum = DEFAULT_MAX_SUM,
    .regen_latin_square_list = false, .r = 3, .s = 4, .d = 2};

  if (!parse_args(argc, argv, &run)) return 1;

  // srand(69);
  srand(time(NULL));

#ifndef __NO_GUI__
  atexit(exit_fun);

  initscr();

  if (!has_colors())
  {
    fprintf(stderr, "[ERROR] Your terminal does not support color!\n");
    endwin();
    exit(1);
  }

  start_color();
#endif


  do_run(&run);

  return 0;
}

void show_starting_stats_on_square(run_data* run)
{
#ifndef __NO_GUI__
  clear();
  mvtaxicab_print(0, 0, run->a);
  printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(run->a) ? "" : " not", run->a.r, run->a.s, run->a.d);
  mvtaxicab_print(0, 40, run->b);
  printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(run->b) ? "" : " not", run->b.r, run->b.s, run->b.d);
  refresh();
  getch();
#else
  taxicab_printf(a);
  printf("\nis%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(a) ? "" : " not", run->a.r, run->a.s, run->a.d);
  taxicab_printf(b);
  printf("\nis%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(b) ? "" : " not", run->b.r, run->b.s, run->b.d);
#endif

  const double p_no_latin = proba_without_latin_square(run->sq);
  const double p_with_latin = proba_with_latin_square(run->sq, run->a.r, run->a.s);

  const uint64_t mu = taxicab_sum_row(run->a, 0) * taxicab_sum_row(run->b, 0);

#ifndef __NO_GUI__
  clear();
  if (run->sq.n <= 16)
    mvpow_m_sqr_printw(0, 0, run->sq);
  printw("is%s a semi magic square of %u-th powers with magic constant mu = %"PRIu64"\n", is_pow_semi_m_sqr(run->sq) ? "" : " not", run->sq.d, mu);
  printw("without latin squares: %e\n", p_no_latin);
  printf("------------------\n");
  printw("with latin squares: %e\n", p_with_latin);

  if (run->use_multithreading)
    printw("\nUsing run->sqULTITHREADED search with %zu threads\n", run->max_threads);
  else
    printw("\nUsing single-threaded search\n");

  getch();
#else
  pow_m_sqr_printf(run->sq);
  printf("\nis%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(run->sq) ? "" : " not", run->sq.d);

  printf("without latin squares: %e\n", p_no_latin);
  printf("with latin squares: %e\n", p_with_latin);
#endif

  if (run->use_multithreading)
    printf("\nUsing run->sqULTITHREADED search with %zu threads\n", run->max_threads);
  else
    printf("\nUsing single-threaded search\n");

  return;
}

void usage(FILE* stream)
{
  fprintf(stream, "USAGE:\n %s [OPTIONS] [REQUIRED_SETS]\n", flag_program_name());
 fprintf(stream, "Options:\n");
 flag_print_options(stream);
 fprintf(stream, "\nRequired sets: Number of compatible sets to find (default: %lu)\n", REQUIERED_SETS);
  return;
}

/*
 * returns 1 upon success
 */
int parse_args(int argc, char** argv, run_data* run)
{
  flag_bool_var  (&run->use_multithreading,      "mt",             false,               "use multithreaded search for the latin square enumeration");
  flag_uint64_var(&run->max_threads,             "threads",        DEFAULT_MAX_THREADS, "the number of threads to use for the latin square enumeration");
  flag_bool_var  (&run->new_taxicabs,            "new-taxi",       false,               "find new taxicabs satifiying the condition");
  flag_double_var(&run->min_proba,               "p",              DEFAUL_MIN_PROBA,    "the minimal number of expected solutions from the taxicabs");
  flag_uint64_var(&run->max_sum,                 "sum",            DEFAULT_MAX_SUM,     "the maximal magic sum of the pair of taixcabs");
  flag_bool_var  (&run->regen_latin_square_list, "regen",          false,               "regenerate the list of all latin squares");
  flag_bool_var  (&run->no_taxicab_method,       "no-taxi-method", false,               "wether to use the taxicab method or not");
  flag_bool_var  (&run->help,                    "help",           false,               "show this help message");
  flag_uint64_var(&run->r,                       "r",              3,                   "value of r");
  flag_uint64_var(&run->s,                       "s",              4,                   "value of s");
  flag_uint64_var(&run->d,                       "d",              2,                   "value of d");

  if (!flag_parse(argc, argv))
  {
    usage(stderr);
    flag_print_error(stderr);
    exit(1);
  }

  if (run->help)
  {
    usage(stdout);
    return 0;
  }

  argc = flag_rest_argc();
  argv = flag_rest_argv();
  while (argc > 0)
  {
    const char *arg = nob_shift(argv, argc);

    // Try to parse as required sets number
    run->requiered_sets = atoi(arg);
  }

  return 1;
}

void exit_fun(void)
{
#ifndef __NO_GUI__
  endwin();
#endif
  return;
}

int do_run(run_data* run)
{
  get_file_name_identifier(run->base_file_name, 255,
      temp_sprintf("output/%s-out-", !run->no_taxicab_method ? "taxi" : "method"),
      "/");
  if (!mkdir_if_not_exists(run->base_file_name)) exit(1);

  run->info = fopen(temp_sprintf("%sinfo.txt", run->base_file_name), "w");
  if (run->info == NULL)
  {
    fprintf(stderr, "[ERORR] Could not open file %sinfo.txt: %s", run->base_file_name, strerror(errno));
    exit(1);
  }
  fprintf(run->info, "r=%"PRIu64",s=%"PRIu64",d=%"PRIu64"\n", run->r, run->s, run->d);

  perf_counter_init(&run->perf, 5.0);

  run->new_taxicabs = run->new_taxicabs || run->r != 3 || run->s != 4;
  if (run->new_taxicabs)
  {
    taxicab_init(&run->a, run->r, run->s, run->d);
    taxicab_init(&run->b, run->s, run->r, run->d);

    find_taxicabs_condition(run->a, run->b, run->min_proba, run->max_sum);
  }
  else
    load_taxicabs("./know/12x12-2/", &run->a, "a", &run->b, "b");

  const double taxicab_time = timer_stop(&run->perf.time);
  fprintf(run->info, "%s the taxicabs took %fs\n", run->new_taxicabs ? "finding" : "loading", taxicab_time);

  pow_m_sqr_init(&run->sq, run->a.r * run->a.s, run->a.d);

  pow_semi_m_sqr_from_taxicab(run->sq, run->a, run->b, NULL, NULL);

  show_starting_stats_on_square(run);

  save_taxicabs(run->base_file_name, run->a, "a", run->b, "b");

  if (!run->no_taxicab_method)
  {
    if (run->regen_latin_square_list || !file_exists("./squares.latin_square"))
    {
      latin_square* P = calloc(run->r, sizeof(latin_square));
      latin_square* Q = calloc(run->s, sizeof(latin_square));
      for (uint32_t i = 0; i < run->r; ++i)
        latin_square_init(P + i, run->s);
      for (uint32_t j = 0; j < run->s; ++j)
        latin_square_init(Q + j, run->r);

      save_all_latin_square_arrays("./", P, Q, run->r, run->s, "squares");

      for (uint32_t i = 0; i < run->r; ++i)
        latin_square_clear(P + i);
      for (uint32_t j = 0; j < run->s; ++j)
        latin_square_clear(Q + j);
      free(P);
      free(Q);
    }

    if (run->use_multithreading)
      search_pow_m_sqr_from_taxicabs_mt(&run->perf, run->base_file_name, run->sq, run->a, run->b, run->requiered_sets, run->max_threads);
    else
      search_pow_m_sqr_from_taxicabs(&run->perf, run->base_file_name, run->sq, run->a, run->b, run->requiered_sets);
  }
  else
  {
    run->m = choose_method(run->r * run->s, pow_m_sqr_sum_row(run->sq, 0));
    switch (run->m)
    {
    case METHOD_MU_SQUARED:
      semi_to_full_naive(&run->perf, run->sq);
      break;
    case METHOD_MU:
      semi_to_full_simultanious_perm(&run->perf, run->sq);
      break;
    case METHOD_SQRT_MU:
      TODO("sqrt(mu)");
      break;
    case METHOD_NONE:
    case METHOD_COUNT:
    default:
      fprintf(stderr, "[ERROR] No possible method\n");
      exit(1);
    }

#ifndef __NO_GUI__
      printw("is%s a magic square of %u-th powers\n", is_pow_m_sqr(run->sq) ? "" : " not", run->sq.d);
      getch();
#else
      printf("is%s a magic square of %u-th powers\n", is_pow_m_sqr(run->sq) ? "" : " not", run->sq.d);
#endif
  }

  save_pow_m_sqr(run->base_file_name, run->sq, "sq");
  fprintf(run->info, "Completing the square took %fs\n", timer_stop(&run->perf.time) - taxicab_time);
  fprintf(run->info, "Total time: %fs\n", timer_stop(&run->perf.time));

  fclose(run->info);

  perf_counter_clear(&run->perf);
  pow_m_sqr_clear(&run->sq);

  taxicab_clear(&run->a);
  taxicab_clear(&run->b);

  return 1;
}


#define NOB_IMPLEMENTATION
#include "nob.h"
#define FLAG_IMPLEMENTATION
#include "flag.h"
#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"
