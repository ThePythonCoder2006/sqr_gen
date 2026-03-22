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
#include "serialize.h"

#include "nob.h"

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

void mt_context_free(mt_context *ctx)
{
  pthread_mutex_destroy(&ctx->mutex);
  free(ctx->threads);
  ctx->threads = NULL;
}

typedef struct
{
  latin_square* P, * Q;
  uint32_t r, s;
  pthread_t thread;
  size_t start_idx, end_idx; // start is inclusive and end is exclusive
  action func;
  void* data;
  FILE *f;
  mt_context* ctx;
} thread_data;


static void *thread_worker(void *arg)
{
  thread_data* data = arg;

  for (size_t idx = data->start_idx; idx < data->end_idx; ++idx)
  {
    for (uint32_t i = 0; i < data->r; ++i)
      fread_latin_square_array(data->f, data->P + i);
    for (uint32_t j = 0; j < data->s; ++j)
      fread_latin_square_array(data->f, data->Q + j);

    if (!(*data->func)(data->P, data->r, data->Q, data->s, data))
    {
      pthread_mutex_lock(&data->ctx->mutex);
      data->ctx->stop_flag = 1;
      pthread_mutex_unlock(&data->ctx->mutex);
    }

  }


  return NULL;
}

uint8_t action_on_all_latin_square_arrays_mt(const char*const base_file_name, const char*const name, size_t thread_count, action func, void* data)
{
  if (thread_count <= 0)
    thread_count = 1;

  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  size_t count;
  uint32_t r, s;
  fread(&count, sizeof(count), 1, f);
  fread(&r, sizeof(r), 1, f);
  fread(&s, sizeof(s), 1, f);

  fclose(f);

  thread_data* datas = calloc(thread_count, sizeof(thread_data));
  mt_context ctx;
  mt_context_init(&ctx, thread_count);

  const size_t step_size = count / thread_count;

  for (size_t thread_idx = 0; thread_idx < thread_count; ++thread_idx)
  {
    datas[thread_idx].P = calloc(r, sizeof(latin_square));
    datas[thread_idx].Q = calloc(s, sizeof(latin_square));
    if (datas[thread_idx].P == NULL || datas[thread_idx].Q == NULL)
    {
      fprintf(stderr, "[OoM] Buy more RAM LOL!!\n");
      exit(1);
    }
    datas[thread_idx].r = r;
    datas[thread_idx].s = s;

    for (uint32_t i = 0; i < r; ++i)
      latin_square_init(datas[thread_idx].P + i, s);
    for (uint32_t j = 0; j < s; ++j)
      latin_square_init(datas[thread_idx].Q + j, r);

    datas[thread_count].f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "r");
    if (datas[thread_count].f == NULL)
    {
      fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
      exit(1);
    }

    datas[thread_idx].start_idx = thread_idx * step_size;
    datas[thread_idx].end_idx = (thread_idx + 1) * step_size;
    datas[thread_idx].func = func;
    datas[thread_idx].data = data;
    datas[thread_idx].ctx  = &ctx;

    pthread_create(&datas[thread_idx].thread, NULL, thread_worker, &datas[thread_idx]);
  }

  for (size_t thread_idx = 0; thread_idx < thread_count; ++thread_idx)
  {
    pthread_join(datas[thread_idx].thread, NULL);

    // free the threads personnal data
    for (uint32_t i = 0; i < r; ++i)
      latin_square_clear(datas[thread_idx].P + i);
    for (uint32_t j = 0; j < s; ++j)
      latin_square_clear(datas[thread_idx].Q + j);
    free(datas[thread_idx].P);
    free(datas[thread_idx].Q);
  }

  free(datas);

  return 0;
}

