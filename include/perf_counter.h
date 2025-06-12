#ifndef __PERF_COUNTER__
#define __PERF_COUNTER__

#include <stdint.h>

#include "timer.h"

#include "gmp.h"
#include "curses.h"

typedef struct
{
  mpz_t boards_tested;
  timer time;
} perf_counter;

void print_perfw(perf_counter perf, const char *const name);
void printf_perf(perf_counter perf, const char *const name);

#endif // __PERF_COUNTER__

#ifdef __PERF_COUNTER_IMPLEMENTATION__

void print_perfw(perf_counter perf, const char *const name)
{
  char coeffs[] = {'k', 'M', 'G', 'T', 'P'};
  size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  mpf_t speed;
  mpf_init(speed);
  mpf_set_z(speed, perf.boards_tested);
  mpf_t time;
  mpf_init_set_d(time, timer_stop(&(perf.time)));
  mpf_div(speed, speed, time);

  char buff[256] = {0};

  if (mpf_cmp_ui(speed, 1000) < 0)
  {
    gmp_snprintf(buff, 255, "%.2Ff %s/s", speed, name);
    printw("%s", buff);
    return;
  }

  uint8_t k = 0;
  for (; k < len; ++k)
  {
    mpf_div_ui(speed, speed, 1000);
    if (mpf_cmp_ui(speed, 1000) < 0)
      break;
  }

  gmp_snprintf(buff, 255, "%.2Ff %c%s/s", speed, coeffs[k], name);
  printw("%s", buff);
  return;
}

void printf_perf(perf_counter perf, const char *const name)
{
  char coeffs[] = {'k', 'M', 'G', 'T', 'P'};
  size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  mpf_t speed;
  mpf_init(speed);
  mpf_set_z(speed, perf.boards_tested);
  mpf_t time;
  mpf_init_set_d(time, timer_stop(&(perf.time)));
  mpf_div(speed, speed, time);

  if (mpf_cmp_ui(speed, 1000) < 0)
  {
    gmp_printf("%.2Ff %s/s", speed, name);
    return;
  }

  uint8_t k = 0;
  for (; k < len; ++k)
  {
    mpf_div_ui(speed, speed, 1000);
    if (mpf_cmp_ui(speed, 1000) < 0)
      break;
  }

  gmp_printf("%.2Ff %c%s/s", speed, coeffs[k], name);
  return;
}

#endif
