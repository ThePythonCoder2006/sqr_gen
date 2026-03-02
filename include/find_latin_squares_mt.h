#ifndef __FIND_LATIN_SQUARES_MT__
#define __FIND_LATIN_SQUARES_MT__

#include <stdint.h>
#include <pthread.h>

#include "types.h"
#include "find_latin_squares.h"

#include "perf_counter.h"

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
} mt_context;

void mt_context_init(mt_context *ctx, size_t max_threads);
void mt_context_free(mt_context *ctx);

uint8_t iterate_over_all_square_array_multithreaded(
    latin_square *P, 
    uint64_t len, 
    latin_square_array_callback f, 
    void *data,
    mt_context* ctx
);

#endif // __FIND_LATIN_SQUARES_MT__
