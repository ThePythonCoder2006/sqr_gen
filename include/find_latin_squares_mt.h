#ifndef __FIND_LATIN_SQUARES_MT__
#define __FIND_LATIN_SQUARES_MT__

#include <stdint.h>
#include <pthread.h>

#include "types.h"
#include "find_latin_squares.h"

#include "perf_counter.h"

// Number of positions to fix for work partitioning
// Fixes positions (1,0), (1,1), ..., (1, NUM_FIXED_POSITIONS-1) or wraps to row 2
// This creates approximately (n-1)^NUM_FIXED_POSITIONS partitions for parallelization
#ifndef NUM_FIXED_POSITIONS
#define NUM_FIXED_POSITIONS 2
#endif

typedef struct {
    uint64_t thread_id;
    uint64_t iterations;
    double speed;
    double peak_speed;
    pthread_t pthread;
    uint8_t active;
} thread_stats;

typedef struct {
    size_t max_threads;
    thread_stats *threads;
    pthread_mutex_t mutex;
    uint64_t total_iterations;
    uint8_t stop_flag;

    // Display thread control
    pthread_t display_thread;
    uint8_t display_active;
    uint8_t display_stop;
    
    // Display data (updated by worker threads, read by display thread)
    uint64_t display_mark_count;
    perf_counter *display_perf;

    // Pre-allocated Q arrays for taxicab search (one per thread)
    latin_square **Q_arrays;
    uint32_t Q_len;
    uint32_t Q_elem_size;
} mt_context;

void mt_context_init(mt_context *ctx, size_t max_threads);
void mt_context_init_with_Q(mt_context *ctx, size_t max_threads, uint32_t Q_len, uint32_t Q_elem_size);
void mt_context_free(mt_context *ctx);

uint8_t iterate_over_all_square_array_multithreaded(
    latin_square *P, 
    uint64_t len, 
    latin_square_array_callback f, 
    void *data,
    mt_context* ctx
);

#endif // __FIND_LATIN_SQUARES_MT__
