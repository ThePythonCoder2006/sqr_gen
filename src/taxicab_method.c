#include "types.h"
#include <curses.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include "taxicab.h"
#include "pow_m_sqr.h"
#include "arithmetic.h"
#include "taxicab_method.h"
#include "permut.h"
#include "serialize.h"

#include "find_sets.h"
#include "find_latin_squares.h"

typedef struct pow_m_sqr_and_da_sets_packed_s
{
  pow_m_sqr *M;
  da_sets *rels;
  size_t requiered_sets;
} pow_m_sqr_and_da_sets_packed;

uint8_t search_pow_m_sqr_from_taxicab_iterate_over_sets_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  if (set_has_magic_sum(selected, *(pack->M)))
  {
    rel_item *set = calloc(n, sizeof(rel_item));
    size_t k = 0;
    for (uint32_t i = 0; i < n; ++i)
      for (uint32_t j = 0; j < n; ++j)
        if (GET_AS_MAT(selected, i, j, n))
          set[k++] = i * n + j;
    printf("%zu\n", pack->rels->count);
    da_append((pack->rels), set);
  }
  return pack->rels->count < pack->requiered_sets;
}

uint8_t search_pow_m_sqr_from_taxicab_find_sets_collision_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  // find_sets_print_selection(selected, n, NULL);

  uint64_t acc = 0;
  rel_item *set = calloc(n, sizeof(rel_item));
  if (set == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  size_t k = 0;
  for (uint32_t i = 0; i < n; ++i)
    for (uint32_t j = 0; j < n; ++j)
      if (GET_AS_MAT(selected, i, j, n))
      {
        set[k++] = i * n + j;
        acc += ui_pow_ui(M_SQR_GET_AS_MAT(*pack->M, i, j), pack->M->d);
      }
#ifndef __NO_GUI__
  printw("  *");
  (void) acc;
#else
  printf("set sum = %"PRIu64"\n", acc);
#endif
  da_append((pack->rels), set);

  // printf("%u\n", pack->rels->count);
  return pack->rels->count < pack->requiered_sets;
}

typedef struct
{
  /*
   * P = array of size r of latin_squares of side s
   * Q = array of size s of latin_squares of side r
   */
  latin_square *P, *Q;
  pow_m_sqr *M;
  uint32_t r, s;
  da_sets rels, mark;
  perf_counter* perf;
  x_y_rel rel1, rel2, inv, sigma;
  uint8_t* rows, *cols;
  uint16_t refresh_frame;
} iterate_over_latin_squares_array_pack;

void print_iterate_over_latin_squares_array_pack(iterate_over_latin_squares_array_pack *pack);
uint8_t compat_callback1(latin_square *_1, uint64_t _2, void *data);
uint8_t compat_callback2(latin_square *_1, uint64_t _2, void *data);
uint8_t check_for_compatibility_in_latin_squares(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark, x_y_rel rel1, x_y_rel rel2, x_y_rel inv, x_y_rel sigma, uint8_t* rows, uint8_t* cols);

// start the search on Q
uint8_t compat_callback1(latin_square *_1, uint64_t _2, void *data)
{
  (void)_1, (void)_2;
  iterate_over_latin_squares_array_pack *pack = (iterate_over_latin_squares_array_pack *)data;
  return iterate_over_all_square_array_callback(pack->Q, pack->s, compat_callback2, data);
}

uint8_t compat_callback2(latin_square *_1, uint64_t _2, void *data)
{
  (void)_1, (void)_2;
  iterate_over_latin_squares_array_pack *pack = (iterate_over_latin_squares_array_pack *)data;

#ifndef __NO_GUI__
  if ((pack->refresh_frame & 0xff) == 0)
  {
    clear();
    move(0, 0);
    char buff[256] = {0};
    gmp_snprintf(buff, 255, "%Zu arrays of latin squares tested so far", pack->perf->counter);
    printw("%s\n", buff);
    print_perfw(pack->perf, "arrays of latin squares");
    addch('\n');
    printw("mark.count: %"PRIu64"\n", pack->mark.count);
    refresh();
  }
  ++pack->refresh_frame;
#endif

  mpz_add_ui(pack->perf->counter, pack->perf->counter, 1);
  mpz_add_ui(pack->perf->lcounter, pack->perf->lcounter, 1);

  return !check_for_compatibility_in_latin_squares(pack->P, pack->Q, pack->M, pack->r, pack->s, pack->rels, pack->mark, pack->rel1, pack->rel2, pack->inv, pack->sigma, pack->rows, pack->cols);
}

/*
 * returns non-zero iff two compatible sets exist within the latin_square provided
 * mark stores rels which have already fallen on different lines before
 */
uint8_t check_for_compatibility_in_latin_squares(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark, x_y_rel rel1, x_y_rel rel2, x_y_rel inv, x_y_rel sigma, uint8_t* rows, uint8_t* cols)
{
  const uint64_t n = r * s;

  da_foreach(rel_item *, rel, &rels)
  {
    memset(rows, 0, sizeof(*rows) * n);
    memset(cols, 0, sizeof(*cols) * n);
    if (!fall_on_different_line_after_latin_squares(rows, cols,*rel, P, Q, r, s))
      continue;

    x_y_rel_after_latin_squares(rel1, *rel, P, Q, r, s);

    da_foreach(rel_item *, prev_rel, &mark)
    {
      x_y_rel_after_latin_squares(rel2, *prev_rel, P, Q, r, s);

      // Check if the two transformed rels still fall on different rows/cols
      memset(rows, 0, sizeof(*rows) * n);
      memset(cols, 0, sizeof(*cols) * n);

      // Mark rows/cols used by rel1
      for (uint64_t i = 0; i < n; ++i)
      {
        rows[i] = 1;
        cols[rel1[i]] = 1;
      }

      // Check if rel2 collides
      uint8_t collision = 0;
      for (uint64_t i = 0; i < n; ++i)
      {
        if (rows[i] || cols[rel2[i]])
        {
          collision = 1;
          break;
        }
      }

      if (collision)
        continue; // These two sets collide after transformation, skip

      if (!rels_are_diagonizable(rel1, rel2, inv, sigma, n))
        continue;

      // we found two non-colliding correct sets: success !!
      printf("YAAAAAAAAAAAAAAAAAAAAAAY!!!!!!!!!!!!!\n");
      printf_rel(*rel, n);
      putchar('\n');
      printf_rel(*prev_rel, n);
      putchar('\n');
#ifndef __NO_GUI__
      clear();
      mvpow_m_sqr_printw_highlighted(0, 0, *M, *rel, *prev_rel, COLOR_YELLOW, COLOR_CYAN);

      refresh();
      getch();
#endif

      permute_into_pow_m_sqr(M, *rel, *prev_rel);

#ifndef __NO_GUI__

      /*
      rel_item main_diag[6] = {0, 1, 2, 3, 4, 5};
      rel_item anti_diag[6] = {5, 4, 3, 2, 1, 0};
      */

      clear();
      mvpow_m_sqr_printw_highlighted(0, 0, *M, *rel, *prev_rel, COLOR_YELLOW, COLOR_CYAN);
      printw("is%s a magic square of %u-th powers", is_pow_m_sqr(*M) ? "" : " not", M->d);
      getch();

      endwin();
#endif
      return 1; // quit the search
    }

    da_append(&mark, *rel);
  }

  return 0;
}

uint8_t find_set_compatible_latin_squares_array(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark, perf_counter* perf)
{
  const size_t n = r * s;

  iterate_over_latin_squares_array_pack pack = {.P = P, .Q = Q, M, .r = r, .s = s, .rels = rels, .mark = mark, .perf = perf};

  pack.rel1 = calloc(n, sizeof(rel_item));
  pack.rel2 = calloc(n, sizeof(rel_item));
  pack.inv = calloc(n, sizeof(rel_item));
  pack.sigma = calloc(n, sizeof(rel_item));
  if (pack.rel1 == NULL || pack.rel2 == NULL || pack.inv == NULL || pack.sigma == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  pack.rows = calloc(n, sizeof(uint8_t));
  pack.cols = calloc(n, sizeof(uint8_t));
  if (pack.rows == NULL || pack.cols == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  uint8_t ret = iterate_over_all_square_array_callback(P, r, compat_callback1, &pack);

  free(pack.rel1);
  free(pack.rel2);
  free(pack.inv);
  free(pack.sigma);

  free(pack.rows);
  free(pack.cols);
  return ret;
}

typedef struct
{
  pow_m_sqr M;
  uint64_t r, s;
  uint8_t (*f)(uint8_t *selected, uint32_t n, void *data);
  pow_m_sqr_and_da_sets_packed* data;
} find_sets_collision_method_pack;

void search_pow_m_sqr_from_taxicabs(pow_m_sqr M, taxicab a, taxicab b, size_t requiered_sets)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

  char base_file_name[256] = {0};

  get_file_name_identifier(base_file_name, 255, "output/taxi-out-", "/");

  if (!mkdir_if_not_exists(base_file_name)) exit(1);

  save_taxicabs(base_file_name, a, "a", b, "b");

  // fill M with a semi magic square from standart latin squares
  pow_semi_m_sqr_from_taxicab(M, a, b, NULL, NULL);

  printf("mu = %"PRIu64"\n", pow_m_sqr_sum_row(M, 0));

  if (requiered_sets <= 0)
  {
#ifndef __NO_GUI__
#else
    fprintf(stderr, "[INFO] found: requiered_sets = %"PRIu64", using default value of %"PRIu64"\n", requiered_sets, REQUIERED_SETS);
#endif
    requiered_sets = REQUIERED_SETS;
  }

  perf_counter perf;
  perf_counter_init(&perf, 1);

  da_sets rels = {.n = M.n};
  pow_m_sqr_and_da_sets_packed pack = {.M = &M, .rels = &rels, .requiered_sets=requiered_sets};
#if 1
  find_sets_collision_method(M, a.r, a.s, requiered_sets, &perf, search_pow_m_sqr_from_taxicab_find_sets_collision_callback, &pack);
#else
  iterate_over_sets_callback(a.r, a.s, search_pow_m_sqr_from_taxicab_iterate_over_sets_callback, &pack);
#endif

#ifndef __NO_GUI__
  clear();
  printw("found: %"PRIu64" sets\n", rels.count);
  // for (uint32_t k = 0; k < M.n; ++k)
  //   printf("(%u, %u), ", rels.items[0][k].i, rels.items[0][k].j);
  // putchar('\n');
  // mvpow_m_sqr_printw_highlighted(1, 0, M, rels.items[0]);
  refresh();
  timeout(5 * 1000); // in ms
  getch();
  timeout(-1);
#else
  printf("%"PRIu64"\n", rels.count);
#endif

  if (rels.count < 2)
  {
    fprintf(stderr, "[ABORT] Found %"PRIu64" < 2 sets.\n", rels.count);
    return;
  }

  save_rels(base_file_name, rels, "rels");

  // arrays holding the latin squares
  latin_square *P = calloc(a.r, sizeof(latin_square));
  latin_square *Q = calloc(a.s, sizeof(latin_square));
  if (P == NULL || Q == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint32_t i = 0; i < a.r; ++i)
    latin_square_init(&(P[i]), a.s);
  for (uint32_t i = 0; i < a.s; ++i)
    latin_square_init(&(Q[i]), a.r);

  perf_counter_clear(&perf);
  perf_counter_init(&perf, 1);

  da_sets mark = {.n = M.n};
  int res = !find_set_compatible_latin_squares_array(P, Q, &M, a.r, a.s, rels, mark, &perf);

  save_latin_squares(base_file_name, P, a.r, Q, a.s, "arrays");

#ifndef __NO_GUI__
  getch();
  clear();
  move(0, 0);
  printw("was%s able to find compatible latin square from the found sets\n", res ? "" : " not");
  refresh();
  getch();
#else
  printf("was%s able to find compatible latin square from the found sets\n", res ? "" : " not");
#endif

  da_free(rels);

  return;
}
