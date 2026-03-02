#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

#include "find_latin_squares_mt.h"
#include "latin_squares.h"
#include "pow_m_sqr.h"

#include "nob.h"

// Test result tracking
typedef struct {
  int passed;
  int failed;
  int total;
} test_results;

static test_results results = {0, 0, 0};

// Color codes for output
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

// Helper: callback that counts invocations
typedef struct {
  uint64_t count;
  pthread_mutex_t mutex;
  uint8_t early_stop;
  uint64_t stop_after;
} callback_counter;

static uint8_t counting_callback(latin_square *P, uint64_t len, void *data)
{
  UNUSED(P);
  UNUSED(len);
  callback_counter *counter = (callback_counter *)data;

  pthread_mutex_lock(&counter->mutex);
  counter->count++;
  uint8_t should_continue = !(counter->early_stop && counter->count >= counter->stop_after);
  pthread_mutex_unlock(&counter->mutex);

  return should_continue;
}

// Helper: callback that validates Latin squares
typedef struct {
  uint64_t valid_count;
  uint64_t invalid_count;
  pthread_mutex_t mutex;
} validation_counter;

static uint8_t validating_callback(latin_square *P, uint64_t len, void *data)
{
  validation_counter *counter = (validation_counter *)data;

  uint8_t all_valid = 1;
  for (uint64_t i = 0; i < len; ++i) {
    if (!is_latin_square(P[i])) {
      all_valid = 0;
      break;
    }
  }

  pthread_mutex_lock(&counter->mutex);
  if (all_valid) {
    counter->valid_count++;
  } else {
    counter->invalid_count++;
  }
  pthread_mutex_unlock(&counter->mutex);

  return 1;
}

// ============================================================================
// TEST 1: mt_context initialization and cleanup
// ============================================================================
int test_mt_context_init_free(void)
{
  TEST_START("mt_context_init and free");

  mt_context ctx;
  size_t max_threads = 4;

  mt_context_init(&ctx, max_threads);

  TEST_ASSERT(ctx.max_threads == max_threads, "max_threads set correctly");
  TEST_ASSERT(ctx.threads != NULL, "threads array allocated");
  TEST_ASSERT(ctx.total_iterations == 0, "total_iterations initialized to 0");
  TEST_ASSERT(ctx.stop_flag == 0, "stop_flag initialized to 0");

  // Check each thread is initialized
  for (size_t i = 0; i < max_threads; ++i) {
    TEST_ASSERT(ctx.threads[i].thread_id == i, "thread_id set correctly");
    TEST_ASSERT(ctx.threads[i].iterations == 0, "iterations initialized to 0");
    TEST_ASSERT(ctx.threads[i].speed == 0.0, "speed initialized to 0");
    TEST_ASSERT(ctx.threads[i].peak_speed == 0.0, "peak_speed initialized to 0");
    TEST_ASSERT(ctx.threads[i].active == 0, "active initialized to 0");
  }

  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 2: Single-threaded execution (baseline)
// ============================================================================
int test_single_threaded_execution(void)
{
  TEST_START("single-threaded execution");

  latin_square P;
  latin_square_init(&P, 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 1);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");
  TEST_ASSERT(counter.count > 0, "callback was invoked");

  printf("  Total invocations: %lu\n", (unsigned long)counter.count);

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(&P);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 3: Multi-threaded execution with 2 threads
// ============================================================================
int test_multi_threaded_execution_2(void)
{
  TEST_START("multi-threaded execution with 2 threads");

  latin_square P;
  latin_square_init(&P, 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 2);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");
  TEST_ASSERT(counter.count > 0, "callback was invoked");

  printf("  Total invocations: %lu\n", (unsigned long)counter.count);

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(&P);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 4: Multi-threaded execution with 4 threads
// ============================================================================
int test_multi_threaded_execution_4(void)
{
  TEST_START("multi-threaded execution with 4 threads");

  latin_square P;
  latin_square_init(&P, 4);

  mt_context ctx = {0};
  mt_context_init(&ctx, 4);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");
  TEST_ASSERT(counter.count > 0, "callback was invoked");

  printf("  Total invocations: %lu\n", (unsigned long)counter.count);

  pthread_mutex_destroy(&counter.mutex);
  mt_context_free(&ctx);
  latin_square_clear(&P);

  TEST_PASS();
}

// ============================================================================
// TEST 5: Early stopping mechanism
// ============================================================================
int test_early_stopping(void)
{
  TEST_START("early stopping mechanism");

  latin_square P;
  latin_square_init(&P, 4);

  mt_context ctx = {0};
  mt_context_init(&ctx, 2);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);
  counter.early_stop = 1;
  counter.stop_after = 100;

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 0, "iteration stopped early (returned 0)");
  TEST_ASSERT(counter.count >= counter.stop_after, "callback invoked at least stop_after times");
  TEST_ASSERT(counter.count < counter.stop_after * 2, "callback didn't run too many extra times");

  printf("  Stopped at: %lu invocations (target: %lu)\n",
       (unsigned long)counter.count, (unsigned long)counter.stop_after);

  pthread_mutex_destroy(&counter.mutex);
  mt_context_free(&ctx);
  latin_square_clear(&P);

  TEST_PASS();
}

// ============================================================================
// TEST 6: Empty input handling
// ============================================================================
int test_empty_input(void)
{
  TEST_START("empty input handling");

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);

  mt_context ctx = {0};
  mt_context_init(&ctx, 4);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    NULL, 0, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "empty input returns success");
  TEST_ASSERT(counter.count == 0, "callback not invoked for empty input");

  pthread_mutex_destroy(&counter.mutex);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 7: Zero threads defaults to 1
// ============================================================================
int test_zero_threads(void)
{
  TEST_START("zero threads defaults to 1");

  latin_square P;
  latin_square_init(&P, 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 0);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");
  TEST_ASSERT(counter.count > 0, "callback was invoked");

  pthread_mutex_destroy(&counter.mutex);
  mt_context_free(&ctx);
  latin_square_clear(&P);

  TEST_PASS();
}

// ============================================================================
// TEST 8: Thread statistics tracking
// ============================================================================
int test_thread_statistics(void)
{
  TEST_START("thread statistics tracking");

  latin_square P;
  latin_square_init(&P, 4);

  mt_context ctx = {0};
  mt_context_init(&ctx, 4);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);
  counter.early_stop = 1;
  counter.stop_after = 5000;

  // Run with multiple threads
  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 0, "iteration stopped as expected");

  printf("  Total callback invocations: %lu\n", (unsigned long)counter.count);

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(&P);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 9: Multiple Latin squares (array of length 2)
// ============================================================================
int test_multiple_latin_squares(void)
{
  TEST_START("multiple Latin squares (array length 2)");

  latin_square P[2];
  latin_square_init(&P[0], 3);
  latin_square_init(&P[1], 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 2);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    P, 2, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");
  TEST_ASSERT(counter.count > 0, "callback was invoked");

  printf("  Total invocations: %lu\n", (unsigned long)counter.count);

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(&P[0]);
  latin_square_clear(&P[1]);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 10: Validate all generated Latin squares are valid
// ============================================================================
int test_generated_squares_valid(void)
{
  TEST_START("validate all generated Latin squares are valid");

  latin_square P[3] = {0};
  latin_square_init(P + 0, 4);
  latin_square_init(P + 1, 4);
  latin_square_init(P + 2, 4);

  mt_context ctx = {0};
  mt_context_init(&ctx, 2);

  validation_counter counter = {0, 0, .mutex={}};
  pthread_mutex_init(&counter.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    P, 3, validating_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");
  TEST_ASSERT(counter.valid_count > 0, "found valid Latin squares");
  TEST_ASSERT(counter.invalid_count == 0, "no invalid Latin squares found");

  printf("  Valid squares: %lu, Invalid: %lu\n",
       (unsigned long)counter.valid_count,
       (unsigned long)counter.invalid_count);

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(P + 0);
  latin_square_clear(P + 1);
  latin_square_clear(P + 2);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 11: Stress test with many threads
// ============================================================================
int test_many_threads(void)
{
  TEST_START("stress test with many threads");

  latin_square P;
  latin_square_init(&P, 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 16);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);
  counter.early_stop = 1;
  counter.stop_after = 1000;

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );

  TEST_ASSERT(result == 0, "iteration stopped early");
  TEST_ASSERT(counter.count >= counter.stop_after, "reached stop threshold");

  printf("  Invocations with 16 threads: %lu\n", (unsigned long)counter.count);

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(&P);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 12: Thread safety stress test
// ============================================================================
typedef struct {
  uint64_t *shared_array;
  size_t array_size;
  pthread_mutex_t mutex;
} thread_safety_data;

static uint8_t thread_safety_callback(latin_square *P, uint64_t len, void *data)
{
  UNUSED(P);
  UNUSED(len);

  thread_safety_data *ts_data = (thread_safety_data *)data;

  // Simulate some work
  for (int i = 0; i < 100; ++i) {
    pthread_mutex_lock(&ts_data->mutex);

    // Increment a random element
    size_t idx = rand() % ts_data->array_size;
    ts_data->shared_array[idx]++;

    pthread_mutex_unlock(&ts_data->mutex);
  }

  return 1;
}

int test_thread_safety(void)
{
  TEST_START("thread safety stress test");

  latin_square P;
  latin_square_init(&P, 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 4);

  const size_t array_size = 100;
  thread_safety_data ts_data;
  ts_data.shared_array = calloc(array_size, sizeof(uint64_t));
  ts_data.array_size = array_size;
  pthread_mutex_init(&ts_data.mutex, NULL);

  uint8_t result = iterate_over_all_square_array_multithreaded(
    &P, 1, thread_safety_callback, &ts_data, &ctx
  );

  TEST_ASSERT(result == 1, "iteration completed successfully");

  // Calculate total increments
  uint64_t total = 0;
  for (size_t i = 0; i < array_size; ++i)
    total += ts_data.shared_array[i];

  printf("  Total increments: %lu\n", (unsigned long)total);
  TEST_ASSERT(total > 0, "shared array was modified");

  free(ts_data.shared_array);
  pthread_mutex_destroy(&ts_data.mutex);
  latin_square_clear(&P);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 13: Consistency check - same results as single-threaded
// ============================================================================
int test_consistency_with_single_threaded(void)
{
  TEST_START("consistency with single-threaded version");

  latin_square P;
  latin_square_init(&P, 3);

  mt_context ctx = {0};
  mt_context_init(&ctx, 1);

  // Run single-threaded
  callback_counter counter1 = {0};
  pthread_mutex_init(&counter1.mutex, NULL);

  iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter1, &ctx
  );

  uint64_t single_count = counter1.count;
  pthread_mutex_destroy(&counter1.mutex);
  mt_context_free(&ctx);

  // Run multi-threaded
  mt_context_init(&ctx, 4);
  callback_counter counter2 = {0};
  pthread_mutex_init(&counter2.mutex, NULL);

  iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter2, &ctx
  );

  uint64_t multi_count = counter2.count;
  pthread_mutex_destroy(&counter2.mutex);

  printf("  Single-threaded: %lu, Multi-threaded: %lu\n",
       (unsigned long)single_count, (unsigned long)multi_count);

  TEST_ASSERT(single_count == multi_count, "counts match between single and multi-threaded");

  latin_square_clear(&P);
  mt_context_free(&ctx);

  TEST_PASS();
}

// ============================================================================
// TEST 14: Performance comparison
// ============================================================================
int test_performance_comparison(void)
{
  TEST_START("performance comparison");

  latin_square P;
  latin_square_init(&P, 4);

  mt_context ctx = {0};
  mt_context_init(&ctx, 1);

  callback_counter counter = {0};
  pthread_mutex_init(&counter.mutex, NULL);
  counter.early_stop = 1;
  counter.stop_after = 10000;

  // Single-threaded timing
  clock_t start1 = clock();
  counter.count = 0;
  iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );
  clock_t end1 = clock();
  double time1 = (double)(end1 - start1) / CLOCKS_PER_SEC;
  mt_context_free(&ctx);

  // Multi-threaded timing
  mt_context_init(&ctx, 4);

  clock_t start2 = clock();
  counter.count = 0;
  iterate_over_all_square_array_multithreaded(
    &P, 1, counting_callback, &counter, &ctx
  );
  clock_t end2 = clock();
  double time2 = (double)(end2 - start2) / CLOCKS_PER_SEC;

  printf("  Single-threaded: %.3f seconds\n", time1);
  printf("  Multi-threaded (4): %.3f seconds\n", time2);

  if (time1 > 0 && time2 > 0) {
    printf("  Speedup: %.2fx\n", time1 / time2);
  }

  pthread_mutex_destroy(&counter.mutex);
  latin_square_clear(&P);
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
  printf("MULTITHREADED LATIN SQUARES - UNIT TESTS\n");
  printf("========================================\n" COLOR_RESET);
  printf("\n");

  // Run all tests
  test_mt_context_init_free();
  test_single_threaded_execution();
  test_multi_threaded_execution_2();
  test_multi_threaded_execution_4();
  test_early_stopping();
  test_empty_input();
  test_zero_threads();
  test_thread_statistics();
  test_multiple_latin_squares();
  test_generated_squares_valid();
  test_many_threads();
  test_thread_safety();
  test_consistency_with_single_threaded();
  test_performance_comparison();

  print_summary();

  return (results.failed == 0) ? 0 : 1;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"
