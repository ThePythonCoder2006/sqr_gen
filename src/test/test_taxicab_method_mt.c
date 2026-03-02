#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

#include "find_latin_squares_mt.h"
#include "taxicab.h"
#include "pow_m_sqr.h"

// Test result tracking
typedef struct {
  int passed;
  int failed;
  int total;
} test_results;

static test_results results = {0, 0, 0};

// Color codes
#define COLOR_RED   "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE  "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

// Test macros
#define TEST_START(name) \
  do { \
    printf(COLOR_BLUE "TEST: " COLOR_RESET "%s\n", name); \
    results.total++; \
  } while(0)

#define TEST_ASSERT(condition, message) \
  do { \
    if (!(condition)) { \
      printf(COLOR_RED "  FAIL: " COLOR_RESET "%s\n", message); \
      printf("  at %s:%d\n", __FILE__, __LINE__); \
      results.failed++; \
      return 0; \
    } \
  } while(0)

#define TEST_PASS() \
  do { \
    printf(COLOR_GREEN "  PASS" COLOR_RESET "\n"); \
    results.passed++; \
    return 1; \
  } while(0)

// ============================================================================
// TEST 1: Basic taxicab creation and validation
// ============================================================================
int test_taxicab_creation(void)
{
  TEST_START("taxicab creation and validation");

  taxicab a;
  taxicab_init(&a, 3, 4, 2);

  TEST_ASSERT(a.r == 3, "r dimension set correctly");
  TEST_ASSERT(a.s == 4, "s dimension set correctly");
  TEST_ASSERT(a.d == 2, "d (power) set correctly");

  taxicab_clear(&a);

  TEST_PASS();
}

// ============================================================================
// TEST 2: pow_m_sqr initialization
// ============================================================================
int test_pow_m_sqr_init(void)
{
  TEST_START("pow_m_sqr initialization");

  pow_m_sqr M;
  pow_m_sqr_init(&M, 6, 2);

  TEST_ASSERT(M.n == 6, "n (size) set correctly");
  TEST_ASSERT(M.d == 2, "d (power) set correctly");
  TEST_ASSERT(M.arr != NULL, "array allocated");

  pow_m_sqr_clear(&M);

  TEST_PASS();
}

// ============================================================================
// TEST 3: Latin square array initialization
// ============================================================================
int test_latin_square_array_init(void)
{
  TEST_START("Latin square array initialization");

  const uint32_t r = 3;
  const uint32_t s = 4;

  latin_square *P = calloc(r, sizeof(latin_square));
  latin_square *Q = calloc(s, sizeof(latin_square));

  TEST_ASSERT(P != NULL, "P array allocated");
  TEST_ASSERT(Q != NULL, "Q array allocated");

  for (uint32_t i = 0; i < r; ++i) {
    latin_square_init(&P[i], s);
    TEST_ASSERT(P[i].n == s, "P[i] size correct");
    TEST_ASSERT(P[i].arr != NULL, "P[i] array allocated");
  }

  for (uint32_t i = 0; i < s; ++i) {
    latin_square_init(&Q[i], r);
    TEST_ASSERT(Q[i].n == r, "Q[i] size correct");
    TEST_ASSERT(Q[i].arr != NULL, "Q[i] array allocated");
  }

  // Cleanup
  for (uint32_t i = 0; i < r; ++i) {
    latin_square_clear(&P[i]);
  }
  for (uint32_t i = 0; i < s; ++i) {
    latin_square_clear(&Q[i]);
  }
  free(P);
  free(Q);

  TEST_PASS();
}

// ============================================================================
// TEST 4: Thread statistics initialization
// ============================================================================
int test_thread_stats_init(void)
{
  TEST_START("thread statistics initialization");

  mt_context ctx;
  size_t max_threads = 4;

  mt_context_init(&ctx, max_threads);

  for (size_t i = 0; i < max_threads; ++i) {
    TEST_ASSERT(ctx.threads[i].thread_id == i, "thread_id initialized");
    TEST_ASSERT(ctx.threads[i].iterations == 0, "iterations at 0");
    TEST_ASSERT(ctx.threads[i].speed == 0.0, "speed at 0");
    TEST_ASSERT(ctx.threads[i].peak_speed == 0.0, "peak_speed at 0");
    TEST_ASSERT(ctx.threads[i].active == 0, "active flag at 0");
  }

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 5: Thread statistics update simulation
// ============================================================================
int test_thread_stats_update(void)
{
  TEST_START("thread statistics update");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  // Simulate thread updates
  pthread_mutex_lock(&ctx.mutex);
  ctx.threads[0].iterations = 1000;
  ctx.threads[0].speed = 1234.56;
  ctx.threads[0].peak_speed = 1500.0;
  ctx.threads[0].active = 1;
  pthread_mutex_unlock(&ctx.mutex);

  TEST_ASSERT(ctx.threads[0].iterations == 1000, "iterations updated");
  TEST_ASSERT(ctx.threads[0].speed > 1234.0, "speed updated");
  TEST_ASSERT(ctx.threads[0].peak_speed > 1499.0, "peak_speed updated");
  TEST_ASSERT(ctx.threads[0].active == 1, "active flag set");

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 6: Stop flag mechanism
// ============================================================================
int test_stop_flag(void)
{
  TEST_START("stop flag mechanism");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  TEST_ASSERT(ctx.stop_flag == 0, "stop_flag starts at 0");

  pthread_mutex_lock(&ctx.mutex);
  ctx.stop_flag = 1;
  pthread_mutex_unlock(&ctx.mutex);

  TEST_ASSERT(ctx.stop_flag == 1, "stop_flag set to 1");

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 7: Mutex lock/unlock
// ============================================================================
int test_mutex_operations(void)
{
  TEST_START("mutex lock/unlock operations");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  // Test that we can lock and unlock
  int lock_result = pthread_mutex_lock(&ctx.mutex);
  TEST_ASSERT(lock_result == 0, "mutex lock successful");

  // Update some data while locked
  ctx.total_iterations = 999;

  int unlock_result = pthread_mutex_unlock(&ctx.mutex);
  TEST_ASSERT(unlock_result == 0, "mutex unlock successful");

  TEST_ASSERT(ctx.total_iterations == 999, "data update persisted");

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 8: Multiple threads updating total iterations
// ============================================================================
typedef struct {
  mt_context *ctx;
  uint64_t increments;
} increment_data;

void *increment_thread(void *arg)
{
  increment_data *data = (increment_data *)arg;

  for (uint64_t i = 0; i < data->increments; ++i) {
    pthread_mutex_lock(&data->ctx->mutex);
    data->ctx->total_iterations++;
    pthread_mutex_unlock(&data->ctx->mutex);
  }

  return NULL;
}

int test_concurrent_updates(void)
{
  TEST_START("concurrent updates to total_iterations");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  const size_t num_threads = 4;
  const uint64_t increments_per_thread = 10000;

  pthread_t threads[num_threads];
  increment_data data[num_threads];

  // Create threads
  for (size_t i = 0; i < num_threads; ++i) {
    data[i].ctx = &ctx;
    data[i].increments = increments_per_thread;
    pthread_create(&threads[i], NULL, increment_thread, &data[i]);
  }

  // Wait for threads
  for (size_t i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  uint64_t expected = num_threads * increments_per_thread;

  printf("  Expected: %lu, Actual: %lu\n",
       (unsigned long)expected,
       (unsigned long)ctx.total_iterations);

  TEST_ASSERT(ctx.total_iterations == expected,
        "all increments accounted for (no race condition)");

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 9: Thread-safe speed calculations
// ============================================================================
void *speed_update_thread(void *arg)
{
  mt_context *ctx = (mt_context *)arg;

  for (int i = 0; i < 100; ++i) {
    pthread_mutex_lock(&ctx->mutex);

    size_t tid = rand() % ctx->max_threads;
    ctx->threads[tid].speed = (double)(rand() % 1000) + 100.0;

    if (ctx->threads[tid].speed > ctx->threads[tid].peak_speed) {
      ctx->threads[tid].peak_speed = ctx->threads[tid].speed;
    }

    pthread_mutex_unlock(&ctx->mutex);

    // Small delay to increase chance of contention
    for (volatile int j = 0; j < 1000; ++j);
  }

  return NULL;
}

int test_thread_safe_speed_updates(void)
{
  TEST_START("thread-safe speed updates");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  const size_t num_threads = 8;
  pthread_t threads[num_threads];

  // Create threads that update speeds
  for (size_t i = 0; i < num_threads; ++i) {
    pthread_create(&threads[i], NULL, speed_update_thread, &ctx);
  }

  // Wait for all threads
  for (size_t i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  // Verify peak_speed >= speed for all threads
  for (size_t i = 0; i < ctx.max_threads; ++i) {
    TEST_ASSERT(ctx.threads[i].peak_speed >= ctx.threads[i].speed,
           "peak_speed >= current speed");
  }

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 10: Context initialization with various thread counts
// ============================================================================
int test_various_thread_counts(void)
{
  TEST_START("context initialization with various thread counts");

  size_t thread_counts[] = {1, 2, 4, 8, 16, 32};
  size_t num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);

  for (size_t i = 0; i < num_tests; ++i) {
    mt_context ctx;
    size_t count = thread_counts[i];

    mt_context_init(&ctx, count);

    TEST_ASSERT(ctx.max_threads == count, "max_threads matches request");
    TEST_ASSERT(ctx.threads != NULL, "threads array allocated");

    // Verify all thread stats are initialized
    for (size_t j = 0; j < count; ++j) {
      TEST_ASSERT(ctx.threads[j].thread_id == j, "thread_id correct");
    }

    mt_context_free(&ctx);
  }

  printf("  Tested thread counts: ");
  for (size_t i = 0; i < num_tests; ++i) {
    printf("%lu ", (unsigned long)thread_counts[i]);
  }
  printf("\n");

  TEST_PASS();
}

// ============================================================================
// TEST 11: Memory allocation and deallocation
// ============================================================================
int test_memory_lifecycle(void)
{
  TEST_START("memory allocation and deallocation");

  const int num_iterations = 100;

  for (int i = 0; i < num_iterations; ++i) {
    mt_context ctx;
    mt_context_init(&ctx, 4);

    // Simulate some work
    pthread_mutex_lock(&ctx.mutex);
    ctx.total_iterations = i;
    pthread_mutex_unlock(&ctx.mutex);

    mt_context_free(&ctx);
  }

  printf("  Completed %d init/free cycles\n", num_iterations);

  TEST_PASS();
}

// ============================================================================
// TEST 12: Stress test - rapid thread creation and destruction
// ============================================================================
void *dummy_worker(void *arg)
{
  mt_context *ctx = (mt_context *)arg;

  pthread_mutex_lock(&ctx->mutex);
  ctx->total_iterations++;
  pthread_mutex_unlock(&ctx->mutex);

  return NULL;
}

int test_rapid_thread_lifecycle(void)
{
  TEST_START("rapid thread creation and destruction");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  const int num_rounds = 10;
  const int threads_per_round = 4;

  for (int round = 0; round < num_rounds; ++round) {
    pthread_t threads[threads_per_round];

    // Create threads
    for (int i = 0; i < threads_per_round; ++i) {
      pthread_create(&threads[i], NULL, dummy_worker, &ctx);
    }

    // Join threads
    for (int i = 0; i < threads_per_round; ++i) {
      pthread_join(threads[i], NULL);
    }
  }

  uint64_t expected = num_rounds * threads_per_round;

  printf("  Completed %d rounds with %d threads each\n",
       num_rounds, threads_per_round);
  printf("  Total iterations: %lu (expected: %lu)\n",
       (unsigned long)ctx.total_iterations,
       (unsigned long)expected);

  TEST_ASSERT(ctx.total_iterations == expected,
        "correct number of iterations");

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 13: Active flag toggling
// ============================================================================
void *active_flag_worker(void *arg)
{
  increment_data *data = (increment_data *)arg;
  mt_context *ctx = data->ctx;
  size_t tid = data->increments; // Reuse as thread id

  pthread_mutex_lock(&ctx->mutex);
  ctx->threads[tid].active = 1;
  pthread_mutex_unlock(&ctx->mutex);

  // Simulate work
  for (volatile int i = 0; i < 100000; ++i);

  pthread_mutex_lock(&ctx->mutex);
  ctx->threads[tid].active = 0;
  pthread_mutex_unlock(&ctx->mutex);

  return NULL;
}

int test_active_flag_toggling(void)
{
  TEST_START("active flag toggling");

  mt_context ctx;
  mt_context_init(&ctx, 4);

  pthread_t threads[4];
  increment_data data[4];

  // Create threads that toggle active flag
  for (size_t i = 0; i < 4; ++i) {
    data[i].ctx = &ctx;
    data[i].increments = i; // Use as thread id
    pthread_create(&threads[i], NULL, active_flag_worker, &data[i]);
  }

  // Wait for threads
  for (size_t i = 0; i < 4; ++i) {
    pthread_join(threads[i], NULL);
  }

  // All threads should be inactive now
  for (size_t i = 0; i < 4; ++i) {
    TEST_ASSERT(ctx.threads[i].active == 0, "thread inactive after completion");
  }

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// Main test runner
// ============================================================================
void print_summary(void)
{
  printf("\n");
  printf("========================================\n");
  printf("TEST SUMMARY\n");
  printf("========================================\n");
  printf("Total tests:  %d\n", results.total);
  printf(COLOR_GREEN "Passed:     %d" COLOR_RESET "\n", results.passed);

  if (results.failed > 0) {
    printf(COLOR_RED "Failed:     %d" COLOR_RESET "\n", results.failed);
  } else {
    printf("Failed:     %d\n", results.failed);
  }

  double pass_rate = results.total > 0 ?
    (double)results.passed / results.total * 100.0 : 0.0;
  printf("Pass rate:  %.1f%%\n", pass_rate);
  printf("========================================\n");
}

int main(void)
{
  srand(time(NULL));

  printf("\n");
  printf(COLOR_YELLOW "========================================\n");
  printf("TAXICAB METHOD MT - UNIT TESTS\n");
  printf("========================================\n" COLOR_RESET);
  printf("\n");

  // Run all tests
  test_taxicab_creation();
  test_pow_m_sqr_init();
  test_latin_square_array_init();
  test_thread_stats_init();
  test_thread_stats_update();
  test_stop_flag();
  test_mutex_operations();
  test_concurrent_updates();
  test_thread_safe_speed_updates();
  test_various_thread_counts();
  test_memory_lifecycle();
  test_rapid_thread_lifecycle();
  test_active_flag_toggling();

  print_summary();

  return (results.failed == 0) ? 0 : 1;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"
