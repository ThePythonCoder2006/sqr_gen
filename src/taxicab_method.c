#include "curses.h"
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

#include "find_sets.h"
#include "find_latin_squares.h"

typedef struct da_sets_s
{
  rel_item **items;
  size_t count;
  size_t capacity;
  uint32_t n; // size of each set
} da_sets;

typedef struct pow_m_sqr_and_da_sets_packed_s
{
  pow_m_sqr *M;
  da_sets *rels;
} pow_m_sqr_and_da_sets_packed;

uint8_t search_pow_m_sqr_from_taxicab_iterate_over_sets_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  if (set_has_magic_sum(selected, *(pack->M)))
  {
    rel_item *set = calloc(n, sizeof(rel_item));
    for (uint32_t i = 0; i < n; ++i)
      for (uint32_t j = 0; j < n; ++j)
        if (GET_AS_MAT(selected, i, j, n))
          set[i] = j;
    printf("%zu\n", pack->rels->count);
    da_append((pack->rels), set);
  }
  return pack->rels->count < REQUIERED_SETS;
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

  for (uint32_t i = 0; i < n; ++i)
    for (uint32_t j = 0; j < n; ++j)
      if (GET_AS_MAT(selected, i, j, n))
      {
        set[i] = j;
        acc += ui_pow_ui(M_SQR_GET_AS_MAT(*pack->M, i, j), pack->M->d);
      }
  printf("set sum = %"PRIu64"\n", acc);
  da_append((pack->rels), set);

  // printf("%u\n", pack->rels->count);
  return pack->rels->count < REQUIERED_SETS;
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
  perf_counter perf;
} iterate_over_latin_squares_array_pack;

void print_iterate_over_latin_squares_array_pack(iterate_over_latin_squares_array_pack *pack);
uint8_t compat_callback1(latin_square *_1, uint64_t _2, void *data);
uint8_t compat_callback2(latin_square *_1, uint64_t _2, void *data);
uint8_t check_for_compatibility_in_latin_squares(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark);

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
#ifndef __DEBUG__
  clear();
  move(0, 0);
  char buff[256] = {0};
  gmp_snprintf(buff, 255, "%Zu arrays of latin squares tested so far", pack->perf.counter);
  printw("%s\n", buff);
  print_perfw(pack->perf, "arrays of latin squares");
  refresh();
#endif
  mpz_add_ui(pack->perf.counter, pack->perf.counter, 1);
  return !check_for_compatibility_in_latin_squares(pack->P, pack->Q, pack->M, pack->r, pack->s, pack->rels, pack->mark);
}

/*
 * returns non-zero iff two compatible sets exist within the latin_square provided
 */
uint8_t check_for_compatibility_in_latin_squares(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark)
{
  const uint64_t n = r * s;

  da_foreach(rel_item *, rel, &rels)
  {
    if (!fall_on_different_line_after_latin_squares(*rel, P, Q, r, s))
      continue;

    da_foreach(rel_item *, prev_rel, &mark)
    {
      if (rels_are_compatible(*rel, *prev_rel, n))
      {
        // we found two non-colliding correct sets: success !!
        printf("YAAAAAAAAAAAAAAAAAAAAAAY!!!!!!!!!!!!!\n");
        printf_rel(*rel, n);
        putchar('\n');
        printf_rel(*prev_rel, n);
        putchar('\n');
#ifndef __DEBUG__
        clear();
        mvpow_m_sqr_printw_highlighted(0, 0, *M, *rel, *prev_rel, COLOR_YELLOW, COLOR_CYAN);
        refresh();
        getch();
#endif

        permute_into_pow_m_sqr(M, *rel, *prev_rel);

#ifndef __DEBUG__

        rel_item main_diag[6] = {0, 1, 2, 3, 4, 5};
        rel_item anti_diag[6] = {5, 4, 3, 2, 1, 0};

        clear();
        mvpow_m_sqr_printw_highlighted(0, 0, *M, main_diag, anti_diag, COLOR_YELLOW, COLOR_CYAN);
        printw("is%s a magic square of %u-th powers", is_pow_m_sqr(*M) ? "" : " not", M->d);
        getch();

        endwin();
#endif
        return 1; // quit the search
      }
    }
    da_append(&mark, *rel);
  }

  return 0;
}

uint8_t find_set_compatible_latin_squares_array(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark)
{
  iterate_over_latin_squares_array_pack pack = {.P = P, .Q = Q, M, .r = r, .s = s, .rels = rels, .mark = mark};
  mpz_init_set_ui(pack.perf.counter, 0);
  timer_start(&pack.perf.time);
  return iterate_over_all_square_array_callback(P, r, compat_callback1, &pack);
}

typedef struct
{
  pow_m_sqr M;
  uint64_t r, s;
  uint8_t (*f)(uint8_t *selected, uint32_t n, void *data);
  pow_m_sqr_and_da_sets_packed* data;
} find_sets_collision_method_pack;

void search_pow_m_sqr_from_taxicabs(pow_m_sqr M, taxicab a, taxicab b)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

  // fill M with a semi magic square from standart latin squares
  pow_semi_m_sqr_from_taxicab(M, a, b, NULL, NULL);

  printf("mu = %"PRIu64"\n", pow_m_sqr_sum_row(M, 0));

  da_sets rels = {.n = M.n};
  pow_m_sqr_and_da_sets_packed pack = {.M = &M, .rels = &rels};
#if 1
  find_sets_collision_method(M, a.r, a.s, search_pow_m_sqr_from_taxicab_find_sets_collision_callback, &pack);
#else
  iterate_over_sets_callback(a.r, a.s, search_pow_m_sqr_from_taxicab_iterate_over_sets_callback, &pack);
#endif

#ifndef __DEBUG__
  clear();
  printw("found: %"PRIu64" sets\n", rels.count);
  // for (uint32_t k = 0; k < M.n; ++k)
  //   printf("(%u, %u), ", rels.items[0][k].i, rels.items[0][k].j);
  // putchar('\n');
  // mvpow_m_sqr_printw_highlighted(1, 0, M, rels.items[0]);
  refresh();
  getch();
#else
  printf("%"PRIu64"\n", rels.count);
#endif

  if (rels.count < 2)
  {
    fprintf(stderr, "[ABORT] Found %"PRIu64" < 2 sets.\n", rels.count);
    return;
  }

  da_free(rels);

  getchar();

  return;

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

  da_sets mark = {.n = M.n};
  int res = !find_set_compatible_latin_squares_array(P, Q, &M, a.r, a.s, rels, mark);
#ifndef __DEBUG__
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
