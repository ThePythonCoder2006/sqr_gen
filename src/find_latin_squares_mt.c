#include <assert.h>
#include <bits/pthreadtypes.h>
#include <curses.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <sys/mman.h>

#include "latin_squares.h"
#include "find_latin_squares_mt.h"
#include "find_latin_squares.h"
#include "perf_counter.h"
#include "types.h"

#include "nob.h"

#ifndef __NO_GUI__
#include <ncurses.h>
#endif

#define LATIN_SQUARE_UNSET UINT8_MAX

void mt_context_init(mt_context *ctx, uint32_t r, uint32_t s)
{
  pthread_mutex_init(&ctx->mutex, NULL);
  ctx->total_iterations = 0;
  ctx->stop_flag        = 0;
  ctx->r                = r;
  ctx->s                = s;

  return;
}

void mt_context_free(mt_context *ctx)
{
  pthread_mutex_destroy(&ctx->mutex);
}

typedef struct
{
  latin_square* P, * Q;   // arrays where to store the read latin squares
  pthread_t thread;       //
  size_t thread_idx;      //
  size_t count;           // number of latin squares to read
  action func;            // callback
  void* data;             // data to pass to the callback
  uint8_t* latin_squares; // where to read the latin_squares
  perf_counter perf;      // store the performance data
  mt_context* ctx;
} thread_data;

typedef struct
{
  size_t thread_count;
  thread_data* datas;
  mt_context* ctx;
} display_pack;

static void* display_thread_worker(void* arg)
{
  display_pack* pack = arg;
  mt_context* ctx = pack->ctx;

  // dont need a mutex here since we are just reading
  while (!pack->ctx->stop_flag)
  {
    ctx->total_iterations = 0;
    for (size_t thread_idx = 0; thread_idx < pack->thread_count; ++thread_idx)
      ctx->total_iterations += pack->datas[thread_idx].perf.counter;

#ifndef __NO_GUI__
    clear();
    move(0, 0);

    printw("Total iterations = %"PRIu64"\n", ctx->total_iterations);
    printw("elapsed time: %.2fs\n", timer_stop(&pack->datas[0].perf.time));

    // Print thread grid header
    printw("Thread Stats Grid:\n");
    printw("%-8s %-12s %-15s %-15s\n", "Thread", "Iterations", "Speed (it/s)", "Peak (it/s)");
    printw("--------------------------------------------------------------------------------\n");

    // Print stats for each thread
    for (size_t thread_idx = 0; thread_idx < pack->thread_count; ++thread_idx)
    {
      perf_counter* ts = &pack->datas[thread_idx].perf;

      // this writes to fields of ts which should not be wrote to by any other thread in normal operation
      perf_counter_update(ts);

      printw("%-8zu %-12"PRIu64" %-15.2f %-15.2f\n",
             thread_idx,
             ts->counter,
             ts->speed,
             ts->peak_speed);
    }

    printw("\n");
#else
#endif

    // Sleep for 100ms between updates
    struct timespec sleep_time = {0, 200000000}; // 200ms
    nanosleep(&sleep_time, NULL);
    refresh();
  }

  return NULL;
}

static void* thread_worker(void* arg)
{
  thread_data* data = arg;
  mt_context* ctx = data->ctx;
  const uint32_t r = ctx->r;
  const uint32_t s = ctx->s;

  uint8_t* arr = data->latin_squares;
  for (data->perf.counter = 0, data->perf.lcounter = 0;
       data->perf.counter < data->count;
       perf_counter_tick(&data->perf))
  {
    if (ctx->stop_flag)
      break;

    for (uint32_t i = 0; i < r; ++i, arr += s*s)
    {
      (data->P + i)->arr = arr;
      if (!is_latin_square(data->P[i]))
      {
        fprintf(stderr, "[UNREACHABLE] latin square P[%u], index number %zu (thread %zu) was not a latin square\n", i, data->thread_idx * data->count + data->perf.counter, data->thread_idx);
      }
    }
    for (uint32_t j = 0; j < s; ++j, arr += r*r)
    {
      (data->Q + j)->arr = arr;
      if (!is_latin_square(data->Q[j]))
      {
        fprintf(stderr, "[UNREACHABLE] latin square Q[%u], index number %zu (thread %zu) was not a latin square\n", j, data->thread_idx * data->count + data->perf.counter, data->thread_idx);
      }
    }

    if (!(*data->func)(data->P, r, data->Q, s, data->data))
    {
      pthread_mutex_lock(&ctx->mutex);
      ctx->stop_flag = 1;
      pthread_mutex_unlock(&ctx->mutex);
    }
  }

  return NULL;
}

uint8_t action_on_all_latin_square_arrays_mt(const char*const base_file_name, const char*const name, size_t thread_count, perf_counter* perf, action func, void* init_data(void*), void clear_data(void *), void* data)
{
  if (thread_count <= 0)
    thread_count = 1;

  /*
   * get the info about the latin square list
   */
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  size_t count;
  uint32_t r, s;
  fread(&count, sizeof(count), 1, f);
  fread(&r,     sizeof(r),     1, f);
  fread(&s,     sizeof(s),     1, f);

  latin_square P;
  const long header_size       = sizeof(count) + sizeof(r) + sizeof(s);
  const long header_ele        = header_size / sizeof(*P.arr);
  const long square_array_ele  = r * s*s + s * r*r;
  const long square_array_size = square_array_ele * sizeof(*P.arr);

  uint8_t* latin_squares = mmap(NULL,
                                 count * square_array_size + header_size,
                                 PROT_READ,
                                 MAP_SHARED,
                                 fileno(f),
                                 0);

  latin_squares += header_ele; // skip the count, r, s header

  if (latin_squares == MAP_FAILED)
  {
    fprintf(stderr, "[ERROR] Could not mmap file %u: %s\n", errno, strerror(errno));
    exit(1);
  }

#ifndef __NO_GUI__
  printw("Mmaped %zu bytes for latin square array\n", count * square_array_size + header_size);
#else
  printf("Mmaped %zu bytes for latin square array\n", count * square_array_size + header_size);
#endif

  fclose(f);


  /*
   * allocate thread data
   */
  thread_data* datas = calloc(thread_count, sizeof(thread_data));
  mt_context ctx;
  mt_context_init(&ctx, r, s);

  /*
   * start the display thread
   */
  display_pack display_thread_pack = {.ctx = &ctx, .datas = datas, .thread_count = thread_count};
  pthread_create(&ctx.display_thread, NULL, display_thread_worker, &display_thread_pack);

  const size_t step_size = count / thread_count;

  for (size_t thread_idx = 0; thread_idx < thread_count; ++thread_idx)
  {
    /*
     * prepare the latin squares arrays
     */
    datas[thread_idx].P = calloc(r, sizeof(latin_square));
    datas[thread_idx].Q = calloc(s, sizeof(latin_square));
    if (datas[thread_idx].P == NULL || datas[thread_idx].Q == NULL)
    {
      fprintf(stderr, "[OoM] Buy more RAM LOL!!\n");
      exit(1);
    }
    // dont need to actually init since we will point P.arr to a part of latin_squares
    for (uint32_t i = 0; i < r; ++i)
      (datas[thread_idx].P + i)->n = s;
    for (uint32_t j = 0; j < s; ++j)
      (datas[thread_idx].Q + j)->n = r;

    /*
     * Set the correct pointer for latin squares array
     */
    datas[thread_idx].latin_squares = latin_squares + thread_idx * step_size * square_array_ele;

    /*
     * set all the random data
     */
    perf_counter_init(&datas[thread_idx].perf, perf->lspeed_window);

    datas[thread_idx].count      = step_size;
    datas[thread_idx].func       = func;
    datas[thread_idx].data       = init_data(data);
    datas[thread_idx].ctx        = &ctx;
    datas[thread_idx].thread_idx = thread_idx;

    /*
     * start the thread
     */
    pthread_create(&datas[thread_idx].thread, NULL, thread_worker, &datas[thread_idx]);
  }

  for (size_t thread_idx = 0; thread_idx < thread_count; ++thread_idx)
  {
    pthread_join(datas[thread_idx].thread, NULL);

    // free the threads personnal data
    // the squares dont need to be cleared since their arrays haven't actually been allocated
    free(datas[thread_idx].P);
    free(datas[thread_idx].Q);
    clear_data(datas[thread_idx].data);
  }

  pthread_mutex_lock(&ctx.mutex);
  ctx.stop_flag = 1;
  pthread_mutex_unlock(&ctx.mutex);
  pthread_join(ctx.display_thread, NULL);

  free(datas);
  munmap(latin_squares, count * square_array_size);

  return 0;
}

