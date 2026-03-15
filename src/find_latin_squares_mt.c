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
#include "types.h"

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

  return;
}

void mt_context_init_with_Q(mt_context *ctx, size_t max_threads, uint32_t Q_len, uint32_t Q_elem_size)
{
  mt_context_init(ctx, max_threads);

  ctx->Q_len = Q_len;
  ctx->Q_elem_size = Q_elem_size;
  ctx->Q_arrays = calloc(max_threads, sizeof(latin_square *));
  if (ctx->Q_arrays == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (size_t i = 0; i < max_threads; ++i)
  {
    ctx->Q_arrays[i] = calloc(Q_len, sizeof(latin_square));
    if (ctx->Q_arrays[i] == NULL)
    {
      fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
      exit(1);
    }

    for (uint32_t j = 0; j < Q_len; ++j)
      latin_square_init(&ctx->Q_arrays[i][j], Q_elem_size);
  }

  return;
}

void mt_context_free(mt_context *ctx)
{
  if (ctx->Q_arrays != NULL)
  {
    for (size_t i = 0; i < ctx->max_threads; ++i)
      if (ctx->Q_arrays[i] != NULL)
      {
        for (uint32_t j = 0; j < ctx->Q_len; ++j)
          latin_square_clear(&ctx->Q_arrays[i][j]);
        free(ctx->Q_arrays[i]);
      }
    free(ctx->Q_arrays);
    ctx->Q_arrays = NULL;
  }

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
  // Reading the flag does not require a mutex, worst case scenario we miss it by one cycle: no big deal
  uint8_t should_stop = work->ctx->stop_flag;

  if (should_stop) return 0;

  // If this is the last square in the array, call the user's callback
  if ((uint64_t)(P - work->P) == work->len - 1)
  {
    uint8_t result = work->f(work->P, work->len, work->data);

    // Update iteration count
    mt_data->local_iterations++;

#define STATS_REFRESH_FREQUENCY (100)

    // Update stats every STATS_REFRESH_FREQUENCY iterations
    if (mt_data->local_iterations % STATS_REFRESH_FREQUENCY == 0)
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
      work->ctx->total_iterations += STATS_REFRESH_FREQUENCY;
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

  // Calculate starting position after NUM_FIXED_POSITIONS fixed cells
  // Fixed positions fill (1,0), (1,1), ..., potentially wrapping to row 2
  uint64_t start_row = 1 + (NUM_FIXED_POSITIONS / n);
  uint64_t start_col = NUM_FIXED_POSITIONS % n;

  // If we're at the end of a row, move to next row, first column
  if (start_col >= n)
  {
    start_row++;
    start_col = 0;
  }


  // Fill the remainder of this square, starting after fixed positions
  uint8_t result = fill_latin_square_remainder(&s, P, start_row, start_col, mt_iterate_next_square, data);

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

extern __thread uint64_t g_current_thread_id;

static void *thread_worker(void *arg)
{
  thread_work_data *work = (thread_work_data *)arg;

  g_current_thread_id = work->thread_id;

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

// Helper to convert linear partition index to position values
// partition_id: linear index (0, 1, 2, ...)
// n: size of Latin square
// values: output array of NUM_FIXED_POSITIONS values
//
// IMPORTANT: Row 0 is [0, 1, 2, ..., n-1], so column j contains j in row 0
// Therefore position (1, j) cannot contain value j (already in that column)
// Each position has exactly (n-1) valid values
static void partition_id_to_values(uint64_t partition_id, uint64_t n, uint8_t *values)
{
  for (uint64_t pos = 0; pos < NUM_FIXED_POSITIONS; ++pos)
  {
    // Position (1, pos) is in column 'pos', which already contains 'pos' in row 0
    // So we can use values 0, 1, ..., pos-1, pos+1, ..., n-1 (n-1 values total)

    uint64_t value_index = partition_id % (n - 1);
    partition_id /= (n - 1);

    // Map value_index (0 to n-2) to actual value (skipping 'pos')
    if (value_index < pos)
      values[pos] = value_index;
    else
      values[pos] = value_index + 1;  // Skip the value 'pos'
  }
}

// Calculate total number of partitions
static uint64_t calculate_num_partitions(uint64_t n)
{
  // Each of the NUM_FIXED_POSITIONS positions has (n-1) valid values
  uint64_t count = 1;
  for (int i = 0; i < NUM_FIXED_POSITIONS; ++i)
    count *= (n - 1);
  return count;
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
  const uint64_t num_partitions = calculate_num_partitions(n);

  thread_work_data *work_items = calloc(ctx->max_threads, sizeof(thread_work_data));
  pthread_t *threads = calloc(ctx->max_threads, sizeof(pthread_t));

  if (work_items == NULL || threads == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  size_t active_threads = 0;
  size_t next_slot = 0;

  // iterate over all partitions
  for (uint64_t partition_id = 0; partition_id < num_partitions && !ctx->stop_flag; ++partition_id)
  {
    // Wait if we've reached max threads
    if (active_threads >= ctx->max_threads)
    {
      // Wait for the thread in the current slot to finish
      pthread_join(threads[next_slot], NULL);

      // Free the old work data
      for (uint64_t j = 0; j < len; ++j)
        latin_square_clear(&work_items[next_slot].P[j]);
      free(work_items[next_slot].P);

      active_threads--;
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
    }

    // Fix the first row of the first square (standard form: 0, 1, 2, ..., n-1)
    for (uint64_t col = 0; col < n; ++col)
      GET_AS_MAT(P_copy[0].arr, 0, col, n) = col;

    // Fix NUM_FIXED_POSITIONS positions to partition the work
    uint8_t fixed_values[NUM_FIXED_POSITIONS] = {0};
    partition_id_to_values(partition_id, n, fixed_values);

    for (int pos = 0; pos < NUM_FIXED_POSITIONS; ++ pos)
    {
      uint64_t row = 1 + (pos / n); // start at row 1
      uint64_t col = pos % n;
      GET_AS_MAT(P_copy[0].arr, row, col, n) = fixed_values[pos];
    }

    // Set up work data
    work_items[active_threads].P = P_copy;
    work_items[active_threads].len = len;
    work_items[active_threads].f = f;
    work_items[active_threads].data = data;
    work_items[active_threads].ctx = ctx;
    work_items[active_threads].start_index = partition_id;
    work_items[active_threads].thread_id = next_slot;

    // Create thread
    pthread_create(&threads[next_slot], NULL, thread_worker, &work_items[active_threads]);

    active_threads++;
    next_slot = (next_slot + 1) % ctx->max_threads;
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
