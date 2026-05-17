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
    pthread_mutex_t mutex;
    uint64_t total_iterations;
    uint8_t stop_flag;
    uint32_t r, s; // sizes of the latin squares (and arrays)

    pthread_t display_thread;
} mt_context;

void mt_context_init(mt_context *ctx, uint32_t r, uint32_t s);
void mt_context_free(mt_context *ctx);

uint8_t action_on_all_latin_square_arrays_mt(const char*const base_file_name, const char*const name, size_t thread_count, perf_counter* perf, action func, void* init_data(void*), void clear_data(void *), void* data);

#endif // __FIND_LATIN_SQUARES_MT__
