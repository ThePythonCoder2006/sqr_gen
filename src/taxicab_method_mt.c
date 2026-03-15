#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <pthread.h>
#include <ncurses.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include "types.h"
#include "taxicab.h"
#include "pow_m_sqr.h"
#include "taxicab_method.h"
#include "permut.h"
#include "serialize.h"
#include "find_sets.h"
#include "find_latin_squares.h"
#include "find_latin_squares_mt.h"
#include "taxicab_method_common.h"

__thread uint64_t g_current_thread_id = 0;

typedef struct {
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
  mt_context *mt_ctx;  // Multithreading context
} iterate_over_latin_squares_array_pack_mt;

// Thread-local wrapper to avoid race conditions on P and Q pointers
typedef struct {
  iterate_over_latin_squares_array_pack_mt *shared_pack;
  latin_square *P;  // Thread-local P array
  latin_square *Q;  // Thread-local Q array (allocated per iteration)
} thread_local_pack;

void print_mt_stats(mt_context *ctx, perf_counter *perf);
uint8_t compat_callback1_mt(latin_square *_1, uint64_t _2, void *data);
uint8_t compat_callback2_mt(latin_square *_1, uint64_t _2, void *data);
uint8_t check_for_compatibility_in_latin_squares_mt(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark, x_y_rel rel1, x_y_rel rel2, x_y_rel inv, x_y_rel sigma, uint8_t* rows, uint8_t* cols, mt_context *mt_ctx);

void print_mt_stats(mt_context *ctx, perf_counter *perf)
{
  mpz_set_ui(perf->counter, ctx->total_iterations);
  mpz_set_ui(perf->lcounter, ctx->total_iterations);
#ifndef __NO_GUI__
  // Print header
  printw("\rTotal iterations: %lu\n", (unsigned long)ctx->total_iterations);

  // Print overall stats
  print_perfw(perf, "arrays of latin squares");
  printw("\n");

  // Print thread grid header
  printw("Thread Stats Grid:\n");
  printw("%-8s %-12s %-15s %-15s %-10s\n", "Thread", "Iterations", "Speed (it/s)", "Peak (it/s)", "Status");
  printw("--------------------------------------------------------------------------------\n");

  // Print stats for each thread
  for (size_t i = 0; i < ctx->max_threads; ++i)
  {
  thread_stats *ts = &ctx->threads[i];
  const char *status = ts->active ? "ACTIVE" : "idle";

  printw("%-8lu %-12lu %-15.2f %-15.2f %-10s\n",
     (unsigned long)ts->thread_id,
     (unsigned long)ts->iterations,
     ts->speed,
     ts->peak_speed,
     status);
  }

  printw("\n");
#else
  printf("\rTotal iterations: %lu", (unsigned long)ctx->total_iterations);

  printf("\nThread Stats:\n");
  for (size_t i = 0; i < ctx->max_threads; ++i)
  {
  thread_stats *ts = &ctx->threads[i];
  if (ts->iterations > 0 || ts->active)
  {
    printf("  Thread %lu: %lu iterations, %.2f it/s (peak: %.2f), %s\n",
       (unsigned long)ts->thread_id,
       (unsigned long)ts->iterations,
       ts->speed,
       ts->peak_speed,
       ts->active ? "ACTIVE" : "idle");
  }
  }
#endif
}

#ifndef __NO_GUI__
// Display thread function - only this thread touches ncurses
static void *display_thread_func(void *arg)
{
  mt_context *ctx = (mt_context *)arg;

  while (1)
  {
    pthread_mutex_lock(&ctx->mutex);

    // Check if we should stop
    if (ctx->display_stop)
    {
      pthread_mutex_unlock(&ctx->mutex);
      break;
    }

    // Clear and redraw screen
    clear();
    move(0, 0);

    // Print stats (this is the ONLY thread calling ncurses functions)
    if (ctx->display_perf != NULL)
      print_mt_stats(ctx, ctx->display_perf);

    refresh();

    pthread_mutex_unlock(&ctx->mutex);

    // Sleep for 100ms between updates
    struct timespec sleep_time = {0, 100000000}; // 100ms
    nanosleep(&sleep_time, NULL);
  }

  return NULL;
}

// Start the display thread
static void start_display_thread(mt_context *ctx, perf_counter *perf)
{
  pthread_mutex_lock(&ctx->mutex);
  ctx->display_perf = perf;
  ctx->display_stop = 0;
  ctx->display_active = 1;
  pthread_mutex_unlock(&ctx->mutex);

  pthread_create(&ctx->display_thread, NULL, display_thread_func, ctx);
}

// Stop the display thread
static void stop_display_thread(mt_context *ctx)
{
  pthread_mutex_lock(&ctx->mutex);
  ctx->display_stop = 1;
  pthread_mutex_unlock(&ctx->mutex);

  if (ctx->display_active)
  {
    pthread_join(ctx->display_thread, NULL);
    ctx->display_active = 0;
  }
}
#endif


// Start the search on Q (multithreaded)
uint8_t compat_callback1_mt(latin_square *P_start, uint64_t _2, void *data)
{
  (void)_2;
  iterate_over_latin_squares_array_pack_mt *shared_pack = (iterate_over_latin_squares_array_pack_mt *)data;

  // Create thread-local pack to hold P and Q without race conditions
  thread_local_pack local_pack = {
    .shared_pack = shared_pack,
    .P = P_start,  // Thread-local P array (filled by this thread)
    .Q = NULL
  };

  // Allocate thread-local Q array for this iteration
  latin_square *Q_local = shared_pack->mt_ctx->Q_arrays[g_current_thread_id];

  // Iterate over Q using the thread-local Q array
  return iterate_over_all_square_array_callback(Q_local, shared_pack->s, compat_callback2_mt, &local_pack);
}

uint8_t compat_callback2_mt(latin_square *Q_array, uint64_t _2, void *data)
{
  (void)_2;
  thread_local_pack *local_pack = (thread_local_pack *)data;
  iterate_over_latin_squares_array_pack_mt *pack = local_pack->shared_pack;

  // Use the thread-local P and Q arrays (no race condition)
  latin_square *P = local_pack->P;
  latin_square *Q = Q_array;

  return !check_for_compatibility_in_latin_squares_mt(P, Q, pack->M, pack->r, pack->s, pack->rels, pack->mark, pack->rel1, pack->rel2, pack->inv, pack->sigma, pack->rows, pack->cols, pack->mt_ctx);
}

uint8_t check_for_compatibility_in_latin_squares_mt(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark, x_y_rel rel1, x_y_rel rel2, x_y_rel inv, x_y_rel sigma, uint8_t* rows, uint8_t* cols, mt_context *mt_ctx)
{
  // This is the same as the non-MT version, just passes mt_ctx around
  UNUSED(mt_ctx);
  const uint64_t n = r * s;

  da_foreach(rel_item *, rel, &rels)
  {
  memset(rows, 0, sizeof(*rows) * n);
  memset(cols, 0, sizeof(*rows) * n);
  if (!fall_on_different_line_after_latin_squares(rows, cols, *rel, P, Q, r, s))
    continue;

  da_foreach(rel_item *, prev_rel, &mark)
  {
    x_y_rel_after_latin_squares(rel1, *rel, P, Q, r, s);
    x_y_rel_after_latin_squares(rel2, *prev_rel, P, Q, r, s);

    if (!rels_are_diagonizable(rel1, rel2, inv, sigma, n))
      continue;

    // We found two non-colliding correct sets: success!
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
    clear();
    mvpow_m_sqr_printw_highlighted(0, 0, *M, *rel, *prev_rel, COLOR_YELLOW, COLOR_CYAN);
    printw("is%s a magic square of %u-th powers", is_pow_m_sqr(*M) ? "" : " not", M->d);
    getch();
    endwin();
#endif
    return 1; // Quit the search
  }

  da_append(&mark, *rel);
  }

  return 0;
}

uint8_t find_set_compatible_latin_squares_array_mt(latin_square *P, latin_square *Q, pow_m_sqr *M, const uint32_t r, const uint32_t s, da_sets rels, da_sets mark, perf_counter* perf, size_t max_threads)
{
  const size_t n = r * s;

  mt_context mt_ctx;
  mt_context_init_with_Q(&mt_ctx, max_threads, s, r);

  iterate_over_latin_squares_array_pack_mt pack = {
    .P = P,
    .Q = Q,
    .M = M,
    .r = r,
    .s = s,
    .rels = rels,
    .mark = mark,
    .perf = perf,
    .mt_ctx = &mt_ctx
  };

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

#ifndef __NO_GUI__
  // Start the display thread (only for GUI mode)
  start_display_thread(&mt_ctx, perf);
#endif

  uint8_t ret = iterate_over_all_square_array_multithreaded(P, r, compat_callback1_mt, &pack, &mt_ctx);

#ifndef __NO_GUI__
  // Stop the display thread
  stop_display_thread(&mt_ctx);
#endif

  free(pack.rel1);
  free(pack.rel2);
  free(pack.inv);
  free(pack.sigma);
  free(pack.rows);
  free(pack.cols);

  mt_context_free(&mt_ctx);

  return ret;
}

void search_pow_m_sqr_from_taxicabs_mt(pow_m_sqr M, taxicab a, taxicab b, size_t requiered_sets, size_t max_threads)
{
  assert(a.r == b.s && a.s == b.r);
  assert(a.d == b.d);
  assert(M.n == a.r * a.s);

  M.d = a.d;

  char base_file_name[256] = {0};
  get_file_name_identifier(base_file_name, 255, "output/taxi-out-mt-", "/");

  if (!mkdir_if_not_exists(base_file_name)) exit(1);

  save_taxicabs(base_file_name, a, "a", b, "b");

  // Fill M with a semi magic square from standard latin squares
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
  pow_m_sqr_and_da_sets_packed pack = {.M = &M, .rels = &rels, .requiered_sets = requiered_sets};

  find_sets_collision_method(M, a.r, a.s, requiered_sets, &perf, search_pow_m_sqr_from_taxicab_find_sets_collision_callback, &pack);

#ifndef __NO_GUI__
  clear();
  printw("found: %"PRIu64" sets\n", rels.count);
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

  // Arrays holding the latin squares
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

  // Use multithreaded version
  int res = !find_set_compatible_latin_squares_array_mt(P, Q, &M, a.r, a.s, rels, mark, &perf, max_threads);

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
}
