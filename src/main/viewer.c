#include <curses.h>
#include <stdio.h>

#include "pow_m_sqr.h"
#include "taxicab.h"
#include "types.h"
#include "serialize.h"
#include "probas.h"

#include "nob.h"
#include "flag.h"

#include <ncurses.h>
#include <string.h>

typedef enum file_type_e
{
  FILE_POW_M_SQR,
  FILE_TAXICABS,
  FILE_TYPE_COUNT,
} file_type;

void usage(void);

int main(int argc, char** argv)
{
  file_type type = FILE_POW_M_SQR;
  char** file_pow_m_sqr = flag_str("sqr", "sq", "to display a square of powers. takes M_name as value");
  Flag_List* file_taxicabs = flag_list("taxi", "to display 2 taxicabs. Write the files names as -taxi=a_name -taxi=b_name, and pass the base_file_name as path");
  char** latex_outfile = flag_str("l", "", "set to non-null to write the read matrix to a latex file of your chosing");

  flag_parse(argc, argv);

  argc = flag_rest_argc();
  argv = flag_rest_argv();

  if (argc <= 0)
  {
    fprintf(stderr, "No file provided\n");
    usage();
    return 1;
  }

  char* path = nob_shift(argv, argc);

  uint32_t r, s, d;
  FILE* info = fopen(temp_sprintf("%sinfo.txt", path), "r");
  if (info == NULL)
  {
    fprintf(stderr, "[ERROR] Could not open file info.txt: %s\n", strerror(errno));
    exit(1);
  }
  fscanf(info, "r=%u,s=%u,d=%u\n", &r, &s, &d);
  fclose(info);

  if ((*file_pow_m_sqr)[0] != '\0')
    type = FILE_POW_M_SQR;
  if (file_taxicabs->count > 0)
    type = FILE_TAXICABS;

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

  pow_m_sqr M;
  taxicab a, b;
  switch (type)
  {
    case FILE_POW_M_SQR:
      load_pow_m_sqr(path, &M, *file_pow_m_sqr);
#ifndef __NO_GUI__
      mvpow_m_sqr_printw(0, 0, M);
      refresh();
      getch();
      clear();
      printw("is%s a magic square of %u-th powers\n", is_pow_m_sqr(M) ? "" : " not", M.d);
      refresh();
      getch();
#else
      pow_m_sqr_printf(M);
#endif

      if ((*latex_outfile)[0] != '\0')
      {

#ifndef __NO_GUI__
        printw("Saving latex in %s\n", *latex_outfile);
#else
        printf("Saving latex in %s\n", *latex_outfile);
#endif
        latex_pow_m_sqr(*latex_outfile, M, "", r, s);
#ifndef __NO_GUI__
        printw("Successfully saved square as latex in %s\n", *latex_outfile);
        getch();
#else
        printf("Successfully saved square as latex\n");
#endif
      }

      pow_m_sqr_clear(&M);
      break;

    case FILE_TAXICABS:
      if (file_taxicabs->count < 2)
      {
        fprintf(stderr, "[ERROR] Not enough arguments for -taxi\n");
        usage();
        exit(1);
      }
      if (file_taxicabs->count > 2)
        fprintf(stderr, "[WARNING] Extra arguments in -taxi, expected 3 but got: %zu\n", file_taxicabs->count);

      load_taxicabs(path, &a, file_taxicabs->items[0], &b, file_taxicabs->items[1]);

#ifndef __NO_GUI__
      clear();
      int dx = mvtaxicab_print(0, 0, a);
      printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
      mvtaxicab_print(0, dx + 5, b);
      printw("is%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
      refresh();
      getch();
#else
      taxicab_printf(a);
      printf("\nis%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(a) ? "" : " not", a.r, a.s, a.d);
      taxicab_printf(b);
      printf("\nis%s a (%"PRIu32", %"PRIu32", %"PRIu32")-taxicab\n", is_taxicab(b) ? "" : " not", b.r, b.s, b.d);
#endif

      if (a.r != b.s || a.s != b.r || a.d != b.d || !taxicab_cross_products_are_distinct(a, b))
      {
        fprintf(stderr, "[WARNING] Non-compatible taxicabs, skipping magic square of powers info");
        break;
      }

      pow_m_sqr_init(&M, a.r * a.s, a.d);
      pow_semi_m_sqr_from_taxicab(M, a, b, NULL, NULL);

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

      getch();
#else
      pow_m_sqr_printf(M);
      printf("\nis%s a semi magic square of %u-th powers\n", is_pow_semi_m_sqr(M) ? "" : " not", M.d);

      printf("without latin squares: %e\n", p_no_latin);
      printf("with latin squares: %e\n", p_with_latin);
#endif

      if ((*latex_outfile)[0] != '\0')
      {

#ifndef __NO_GUI__
        printw("Saving latex in %s\n", *latex_outfile);
#else
        printf("Saving latex in %s\n", *latex_outfile);
#endif
        FILE* latex = fopen(*latex_outfile, "w");
        flatex_taxicab(latex, a);
        flatex_taxicab(latex, b);
#ifndef __NO_GUI__
        printw("Successfully saved taxicabs as latex in %s\n", *latex_outfile);
        getch();
#else
        printf("Successfully saved taxicabs as latex in %s\n", *latex_outfile);
#endif
      }
      break;

    case FILE_TYPE_COUNT:
      fprintf(stderr, "[UNREACHABLE] wrong file type\n");
      exit(1);
  }

#ifndef __NO_GUI__
  endwin();
#endif

  return 0;
}

void usage(void)
{
  return;
}

#define  NOB_IMPLEMENTATION
#define FLAG_IMPLEMENTATION
#include "nob.h"
#include "flag.h"

#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"
