#include <curses.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include "types.h"
#include "taxicab.h"
#include "pow_m_sqr.h"
#include "taxicab_method.h"
#include "permut.h"
#include "serialize.h"
#include "find_sets.h"
#include "taxicab_method_common.h"
#include "find_latin_squares_mt.h"

void print_iterate_over_latin_squares_array_pack(iterate_over_latin_squares_array_pack *pack);

/*
 * returns non-zero to indicate to continue
 * mark stores rels which have already fallen on different lines before
 */
uint8_t check_for_compatibility_in_latin_squares_mt(latin_square *P, const uint32_t r, latin_square *Q, const uint32_t s, void* data)
{
  const uint64_t n = r * s;

  iterate_over_latin_squares_array_pack* pack = data;
  da_sets rels = pack->rels;
  da_sets mark = pack->mark;
  // pow_m_sqr* M = pack->M;
  uint8_t* rows = pack->rows;
  uint8_t* cols = pack->cols;
  x_y_rel rel1 = pack->rel1;
  x_y_rel rel2 = pack->rel2;
  x_y_rel inv = pack->inv;
  x_y_rel sigma = pack->sigma;

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

      memset(inv, 0, n * sizeof(*inv));
      memset(sigma, 0, n * sizeof(*inv));

      if (!rels_are_diagonizable(rel1, rel2, inv, sigma, n))
        continue;

      return 0; // quit the search
    }

    da_append(&mark, *rel);
  }

  return 1;
}

typedef struct init_pack_data_s
{
  pow_m_sqr* M;
  da_sets rels;
  perf_counter* perf;
} init_pack_data;

void* init_pack(void* arg)
{
  init_pack_data* data = arg;
  pow_m_sqr* M       = data->M;
  da_sets rels       = data->rels;
  perf_counter* perf = data->perf;

  iterate_over_latin_squares_array_pack* pack = malloc(sizeof(iterate_over_latin_squares_array_pack));

  size_t n = M->n;

  pack->M     = M;
  pack->rels  = rels; // rels is read only
  pack->mark  = (da_sets){0};
  pack->perf  = calloc(1, sizeof(perf_counter)); // zero initialize
  perf_counter_init(pack->perf, perf->lspeed_window);
  perf->counter = 69420ULL;
  pack->rel1  = calloc(n, sizeof(rel_item));
  pack->rel2  = calloc(n, sizeof(rel_item));
  pack->inv   = calloc(n, sizeof(rel_item));
  pack->sigma = calloc(n, sizeof(rel_item));
  if (pack->rel1 == NULL || pack->rel2 == NULL || pack->inv == NULL || pack->sigma == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  pack->rows = calloc(n, sizeof(uint8_t));
  pack->cols = calloc(n, sizeof(uint8_t));
  if (pack->rows == NULL || pack->cols == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  return pack;
}

void clear_pack(void* data)
{
  iterate_over_latin_squares_array_pack* pack = data;

  free(pack->rel1);
  free(pack->rel2);
  free(pack->inv);
  free(pack->sigma);
  free(pack->rows);
  free(pack->cols);
  free(pack->perf);

  return;
}

uint8_t find_set_compatible_latin_squares_array_mt(const char* const base_file_name, const char* const name, pow_m_sqr *M, da_sets rels, da_sets mark, perf_counter* perf, size_t thread_count)
{
  UNUSED(mark);

  init_pack_data pack = (init_pack_data){
    .M=M,
    .perf = perf,
    .rels = rels,
  };

  uint8_t ret = action_on_all_latin_square_arrays_mt(base_file_name, name, thread_count, perf, check_for_compatibility_in_latin_squares_mt, init_pack, clear_pack, &pack);

  return ret;
}

typedef struct
{
  pow_m_sqr M;
  uint64_t r, s;
  uint8_t (*f)(uint8_t *selected, uint32_t n, void *data);
  pow_m_sqr_and_da_sets_packed* data;
} find_sets_collision_method_pack;

void search_pow_m_sqr_from_taxicabs_mt(perf_counter* perf, const char* const base_file_name, pow_m_sqr M, taxicab a, taxicab b, size_t requiered_sets, size_t thread_count)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

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

  da_sets rels = {.n = M.n};
  pow_m_sqr_and_da_sets_packed pack = {.M = &M, .rels = &rels, .requiered_sets=requiered_sets};
#if 1
  find_sets_collision_method(M, a.r, a.s, requiered_sets, perf, search_pow_m_sqr_from_taxicab_find_sets_collision_callback, &pack);
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
  printw("\n[OK] Moving onto latin square array search\n");
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

  perf_counter_clear(perf);
  perf_counter_init(perf, 1);

  da_sets mark = {.n = M.n};
  int res = find_set_compatible_latin_squares_array_mt("./", "squares", &M, rels, mark, perf, thread_count);

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
