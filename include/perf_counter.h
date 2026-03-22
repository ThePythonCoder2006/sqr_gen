#ifndef __PERF_COUNTER__
#define __PERF_COUNTER__

#include <stdint.h>

#include "timer.h"

typedef struct
{
  uint64_t counter, lcounter;
  timer time;
  double lspeed_time, lspeed_window;
  double speed, peak_speed, lspeed, peak_lspeed;
} perf_counter;

void print_perfw(perf_counter* perf, const char *const name);
void printf_perf(perf_counter* perf, const char *const name);
void perf_counter_init(perf_counter* perf, const double lspeed_windows);
void perf_counter_clear(perf_counter* perf);
void perf_counter_tick(perf_counter *perf);

#endif // __PERF_COUNTER__

#ifdef __PERF_COUNTER_IMPLEMENTATION__

#include <ncurses.h>

#ifndef __NO_GUI__

void print_perfw(perf_counter *perf, const char *const name)
{
  char coeffs[] = {' ', 'k', 'M', 'G', 'T', 'P'};
  size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  double time = timer_stop(&(perf->time));
  perf->speed = perf->counter / time;

  if (perf->speed > perf->peak_speed)
    perf->peak_speed = perf->speed;

  if (time - perf->lspeed_time >= perf->lspeed_window)
  {
    perf->lspeed_time = time;
    perf->lcounter = 0;

    perf->lspeed = perf->counter / time;

    if (perf->lspeed > perf->peak_lspeed)
      perf->peak_lspeed = perf->lspeed;
  }

  uint8_t k = 0;
  for (; k < len; ++k)
  {
    perf->speed /= 1000;
    perf->peak_speed /= 1000;
    if (perf->speed < 1000)
      break;
  }

  printw("%.2f %c%s/s peak: %.2f %c%s/s\n", perf->speed, coeffs[k], name, perf->peak_speed, coeffs[k], name);

  k = 0;
  for (; k < len; ++k)
  {
    perf->lspeed /= 1000;
    perf->peak_lspeed /= 1000;

    if (perf->lspeed < 1000)
      break;
  }

  printw("local: %.2f %c%s/s peak: %.2f %c%s/s\n", perf->lspeed, coeffs[k], name, perf->peak_lspeed, coeffs[k], name);

  printw("step time: %.2lfs\n", time);

  return;
}

#endif

void printf_perf(perf_counter* perf, const char *const name)
{
  char coeffs[] = {' ', 'k', 'M', 'G', 'T', 'P'};
  const size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  double time = timer_stop(&(perf->time));
  perf->speed = perf->counter / time;

  if (perf->speed > perf->peak_speed)
    perf->peak_speed = perf->speed;

  if (time - perf->lspeed_time >= perf->lspeed_window)
  {
    perf->lspeed_time = time;
    perf->lcounter = 0;

    perf->lspeed = perf->counter / time;

    if (perf->lspeed > perf->peak_lspeed)
      perf->peak_lspeed = perf->lspeed;
  }

  uint8_t k = 0;
  for (; k < len; ++k)
  {
    perf->speed /= 1000;
    perf->peak_speed /= 1000;
    if (perf->speed < 1000)
      break;
  }

  printf("%.2f %c%s/s peak: %.2f %c%s/s\n", perf->speed, coeffs[k], name, perf->peak_speed, coeffs[k], name);

  k = 0;
  for (; k < len; ++k)
  {
    perf->lspeed /= 1000;
    perf->peak_lspeed /= 1000;

    if (perf->lspeed < 1000)
      break;
  }

  printf("local: %.2f %c%s/s peak: %.2f %c%s/s\n", perf->lspeed, coeffs[k], name, perf->peak_lspeed, coeffs[k], name);

  printf("step time: %.2lfs\n", time);
  return;
}

void perf_counter_init(perf_counter* perf, const double lspeed_window)
{
  timer_start(&perf->time);
  perf->counter  = 0;
  perf->lcounter = 0;

  perf->speed = 0;
  perf->peak_speed = 0;
  perf->lspeed = 0;
  perf->peak_lspeed = 0;

  perf->lspeed_window = lspeed_window;
  perf->lspeed_time = timer_stop(&perf->time);
  return;
}

void perf_counter_clear(perf_counter* perf)
{
  UNUSED(perf);
  return;
}

void perf_counter_tick(perf_counter *perf)
{
  ++perf->counter;
  ++perf->lcounter;
}

#endif
