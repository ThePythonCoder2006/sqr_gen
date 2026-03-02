#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "find_latin_squares_mt.h"
#include "find_latin_squares.h"
#include "pow_m_sqr.h"
#include "perf_counter.h"

#ifndef __NO_GUI__
#include <ncurses.h>
#endif

#define LATIN_SQUARE_UNSET UINT8_MAX

typedef struct {
  latin_square *P;
  uint64_t len;
  latin_square_array_callback f;
  void *data;
  mt_context *ctx;
  uint64_t start_index;
  uint64_t thread_id;
} thread_work_data;

void mt_context_init(mt_context *ctx, size_t max_threads)
{
  ctx->max_threads = max_threads;
  ctx->threads = calloc(max_threads, sizeof(thread_stats));
  if (ctx->threads == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  pthread_mutex_init(&ctx->mutex, NULL);
  ctx->total_iterations = 0;
  ctx->stop_flag = 0;

  for (size_t i = 0; i < max_threads; ++i)
  {
    ctx->threads[i].thread_id = i;
    ctx->threads[i].iterations = 0;
    ctx->threads[i].speed = 0.0;
    ctx->threads[i].peak_speed = 0.0;
    ctx->threads[i].active = 0;
  }
}

void mt_context_free(mt_context *ctx)
{
  pthread_mutex_destroy(&ctx->mutex);
  free(ctx->threads);
  ctx->threads = NULL;
}

// State for filling a single square from a partial configuration
typedef struct {
  uint64_t n;
  uint8_t **usedInRow;
  uint8_t **usedInCol;
} fill_state;

// Data passed through the recursion for statistics tracking
typedef struct {
  thread_work_data *work_data;
  uint64_t local_iterations;
  double start_time;
} mt_iteration_data;

// Forward declarations
static uint8_t fill_latin_square_remainder(fill_state *s, latin_square *P,
                       uint64_t row, uint64_t col,
                       latin_square_callback callback,
                       void *data);
static uint8_t mt_iterate_next_square(latin_square *P, void *data);
static uint8_t mt_iterate_first_square_remainder(latin_square *P, void *data);

// Fill the remainder of a partially-filled Latin square
// This is analogous to iterate_over_all_square_callback_inside
static uint8_t fill_latin_square_remainder(fill_state *s, latin_square *P,
                       uint64_t row, uint64_t col,
                       latin_square_callback callback,
                       void *data)
{
  // If we've filled the entire square, call the callback
  if (row == P->n)
    return (*callback)(P, data);

  // If this cell is already filled, skip to next
  if (GET_AS_MAT(P->arr, row, col, P->n) != LATIN_SQUARE_UNSET)
  {
    // Move to next cell
    if (col == P->n - 1)
      return fill_latin_square_remainder(s, P, row + 1, 0, callback, data);
    else
      return fill_latin_square_remainder(s, P, row, col + 1, callback, data);
  }

  // Try each possible value for this cell
  for (uint64_t num = 0; num < P->n; num++)
  {
    if (!(s->usedInRow[row][num]) && !(s->usedInCol[col][num]))
    {
      // Place the number
      GET_AS_MAT(P->arr, row, col, P->n) = num;
      s->usedInRow[row][num] = 1;
      s->usedInCol[col][num] = 1;

      // Recurse to next cell
      uint8_t cont = (col == P->n - 1)
        ? fill_latin_square_remainder(s, P, row + 1, 0, callback, data)
        : fill_latin_square_remainder(s, P, row, col + 1, callback, data);

      // Backtrack
      GET_AS_MAT(P->arr, row, col, P->n) = LATIN_SQUARE_UNSET;
      s->usedInRow[row][num] = 0;
      s->usedInCol[col][num] = 0;

      if (!cont)
        return 0;
    }
  }

  return 1;
}

// Callback when the first square is complete - now iterate the rest
static uint8_t mt_iterate_next_square(latin_square *P, void *data)
{
  mt_iteration_data *mt_data = (mt_iteration_data *)data;
  thread_work_data *work = mt_data->work_data;

  // Check if we should stop
  // Reading the flag does not require a muter, worst case scenario we miss it by one cycle: no big deal
  uint8_t should_stop = work->ctx->stop_flag;

  if (should_stop) return 0;

  // If this is the last square in the array, call the user's callback
  if ((uint64_t)(P - work->P) == work->len - 1)
  {
    uint8_t result = work->f(work->P, work->len, work->data);

    // Update iteration count
    mt_data->local_iterations++;

    // Update stats every 1000 iterations
    if (mt_data->local_iterations % 1000 == 0)
    {
      double current_time = (double)clock() / CLOCKS_PER_SEC;
      double elapsed = current_time - mt_data->start_time;

      // all of the speeds are local to a thread so no mutex needed
      work->ctx->threads[work->thread_id].iterations = mt_data->local_iterations;
      if (elapsed > 0)
      {
        double speed = (double)mt_data->local_iterations / elapsed;
        work->ctx->threads[work->thread_id].speed = speed;
        if (speed > work->ctx->threads[work->thread_id].peak_speed)
          work->ctx->threads[work->thread_id].peak_speed = speed;
      }

      pthread_mutex_lock(&work->ctx->mutex);
      work->ctx->total_iterations += 1000;
      pthread_mutex_unlock(&work->ctx->mutex);
    }

    return result;
  }

  if ((P+1)->n != P->n)
  {
    fprintf(stderr, "[ERROR] Corrupt latin square size: %u != %u\n", (P+1)->n, P->n);
    exit(1);
  }

  // Otherwise, iterate over all possibilities for the next square
  return iterate_over_all_square_callback(P + 1, mt_iterate_next_square, data);
}

// Fill the remainder of the first square and continue to the rest
static uint8_t mt_iterate_first_square_remainder(latin_square *P, void *data)
{
  mt_iteration_data *mt_data = (mt_iteration_data *)data;
  thread_work_data *work = mt_data->work_data;
  (void) work;

  const uint64_t n = P->n;

  // Initialize the fill state based on what's already filled in the square
  fill_state s;
  s.n = n;
  s.usedInRow = malloc(n * sizeof(uint8_t *));
  s.usedInCol = malloc(n * sizeof(uint8_t *));

  if (s.usedInRow == NULL || s.usedInCol == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint64_t i = 0; i < n; i++)
  {
    s.usedInRow[i] = calloc(n, sizeof(uint8_t));
    s.usedInCol[i] = calloc(n, sizeof(uint8_t));
  }

  // Mark already-filled cells as used
  for (uint64_t row = 0; row < n; row++)
  {
    for (uint64_t col = 0; col < n; col++)
    {
      uint8_t val = GET_AS_MAT(P->arr, row, col, n);
      if (val != LATIN_SQUARE_UNSET)
      {
        s.usedInRow[row][val] = 1;
        s.usedInCol[col][val] = 1;
      }
    }
  }

  // Fill the remainder of this square, starting from position (1,1)
  // (since row 0 is complete and position (1,0) is filled)
  uint8_t result = fill_latin_square_remainder(&s, P, 1, 1, mt_iterate_next_square, data);

  // Free the state
  for (uint64_t i = 0; i < n; i++)
  {
    free(s.usedInRow[i]);
    free(s.usedInCol[i]);
  }
  free(s.usedInRow);
  free(s.usedInCol);

  return result;
}

static void *thread_worker(void *arg)
{
  thread_work_data *work = (thread_work_data *)arg;

  // no mutex needed, only this thread shuold ever write to his .active
  // others may read but this wont cause any actual issue
  work->ctx->threads[work->thread_id].active = 1;

  // Set up iteration data for statistics tracking
  mt_iteration_data mt_data = {
    .work_data = work,
    .local_iterations = 0,
    .start_time = (double)clock() / CLOCKS_PER_SEC
  };

  // Fill the remainder of the first square (which has row 0 and position (1,0) already set)
  // When complete, continue to the rest of the squares in the array

  uint8_t result = mt_iterate_first_square_remainder(work->P, &mt_data);

  work->ctx->threads[work->thread_id].active = 0;
  if (!result)
  {
    pthread_mutex_lock(&work->ctx->mutex);
    work->ctx->stop_flag = 1;
    pthread_mutex_unlock(&work->ctx->mutex);
  }

  return NULL;
}

uint8_t iterate_over_all_square_array_multithreaded(
  latin_square *P,
  uint64_t len,
  latin_square_array_callback f,
  void *data,
  mt_context *ctx)
{
  if (len == 0) return 1;
  if (ctx->max_threads == 0) ctx->max_threads = 1;

  uint8_t result = 1;

  const uint64_t n = P[0].n;

  // We'll distribute work by partitioning based on position (1,0) of the first square

  thread_work_data *work_items = calloc(n, sizeof(thread_work_data));
  pthread_t *threads = calloc(n, sizeof(pthread_t));

  if (work_items == NULL || threads == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  size_t active_threads = 0;

  // For each possible value in position (1,0) of the first square
  // first_val cannot be 0 as the first line is (0, 1, ...) so 0 is already used in the first column
  for (uint64_t first_val = 1; first_val < n && !ctx->stop_flag; ++first_val)
  {
    // Wait if we've reached max threads
    while (active_threads >= ctx->max_threads && !ctx->stop_flag)
    {
      // Wait for all current threads to finish
      for (size_t i = 0; i < active_threads; ++i)
        pthread_join(threads[i], NULL);
      active_threads = 0;
    }

    if (ctx->stop_flag) break;

    // Create a copy of P for this thread
    latin_square *P_copy = calloc(len, sizeof(latin_square));
    if (P_copy == NULL)
    {
      fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
      exit(1);
    }

    for (uint64_t i = 0; i < len; ++i)
    {
      latin_square_init(&P_copy[i], P[i].n);
      // Initialize all cells as UNSET
      for (uint64_t j = 0; j < P[i].n * P[i].n; ++j)
        P_copy[i].arr[j] = LATIN_SQUARE_UNSET;
      assert(P_copy[i].n == P[i].n);
    }

    // Fix the first row of the first square (standard form: 0, 1, 2, ..., n-1)
    for (uint64_t col = 0; col < n; ++col)
      GET_AS_MAT(P_copy[0].arr, 0, col, n) = col;

    // Fix position (1,0) to partition the work
    GET_AS_MAT(P_copy[0].arr, 1, 0, n) = first_val;

    // Set up work data
    work_items[active_threads].P = P_copy;
    work_items[active_threads].len = len;
    work_items[active_threads].f = f;
    work_items[active_threads].data = data;
    work_items[active_threads].ctx = ctx;
    work_items[active_threads].start_index = first_val;
    work_items[active_threads].thread_id = active_threads;

    // Create thread
    pthread_create(&threads[active_threads], NULL, thread_worker, &work_items[active_threads]);

    active_threads++;
  }

  // Wait for all remaining threads
  for (size_t i = 0; i < active_threads; ++i)
  {
    pthread_join(threads[i], NULL);

    // Free the copied P arrays
    for (uint64_t j = 0; j < len; ++j)
      latin_square_clear(&work_items[i].P[j]);
    free(work_items[i].P);
  }

  result = !ctx->stop_flag;

  // Cleanup
  free(work_items);
  free(threads);

  return result;
}
