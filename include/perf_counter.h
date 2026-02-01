#ifndef __PERF_COUNTER__
#define __PERF_COUNTER__

#include <stdint.h>

#include "timer.h"

#include "gmp.h"

typedef struct
{
  mpz_t counter;
  timer time;
  mpf_t time_, speed, peak_speed;
} perf_counter;

void print_perfw(perf_counter* perf, const char *const name);
void printf_perf(perf_counter* perf, const char *const name);

#endif // __PERF_COUNTER__

#ifdef __PERF_COUNTER_IMPLEMENTATION__

#include <ncurses.h>

void print_perfw(perf_counter *perf, const char *const name)
{
  char coeffs[] = {'k', 'M', 'G', 'T', 'P'};
  size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  mpf_set_z(perf->speed, perf->counter);
  mpf_init_set_d(perf->time_, timer_stop(&(perf->time)));
  mpf_div(perf->speed, perf->speed, perf->time_);

  char buff[256] = {0};

  if (mpf_cmp(perf->speed, perf->peak_speed) > 0)
    mpf_set(perf->peak_speed, perf->speed);

  if (mpf_cmp_ui(perf->speed, 1000) < 0)
  {
    gmp_snprintf(buff, 255, "%.2Ff %s/s peak: %.2Ff %s/s", perf->speed, name, perf->peak_speed, name);
    printw("%s", buff);
    return;
  }

  uint8_t k = 0;
  for (; k < len; ++k)
  {
    mpf_div_ui(perf->speed, perf->speed, 1000);
    mpf_div_ui(perf->peak_speed, perf->peak_speed, 1000);
    if (mpf_cmp_ui(perf->speed, 1000) < 0)
      break;
  }

  gmp_snprintf(buff, 255, "%.2Ff %c%s/s peak: %.2Ff %c%s/s", perf->speed, coeffs[k], name, perf->peak_speed, coeffs[k], name);
  printw("%s", buff);

  return;
}

void printf_perf(perf_counter* perf, const char *const name)
{
  char coeffs[] = {'k', 'M', 'G', 'T', 'P'};
  const size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  mpf_set_z(perf->speed, perf->counter);
  mpf_init_set_d(perf->time_, timer_stop(&(perf->time)));
  mpf_div(perf->speed, perf->speed, perf->time_);

  if (mpf_cmp_ui(perf->speed, 1000) < 0)
  {
    gmp_printf("%.2Ff %s/s", perf->speed, name);
    return;
  }

  uint8_t k = 0;
  for (; k < len - 1; ++k)
  {
    mpf_div_ui(perf->speed, perf->speed, 1000);
    mpf_div_ui(perf->peak_speed, perf->peak_speed, 1000);
    if (mpf_cmp_ui(perf->speed, 1000) < 0)
      break;
  }

  gmp_printf("%.2Ff %c%s/s", perf->speed, coeffs[k], name);
  return;
}

#endif
