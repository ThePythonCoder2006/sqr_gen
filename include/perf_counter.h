#ifndef __PERF_COUNTER__
#define __PERF_COUNTER__

#include <stdint.h>

#include "timer.h"

#include "gmp.h"

typedef struct
{
  mpz_t counter, lcounter;
  timer time;
  double lspeed_time, lspeed_window;
  mpf_t time_, speed, peak_speed, lspeed, peak_lspeed;
} perf_counter;

void print_perfw(perf_counter* perf, const char *const name);
void printf_perf(perf_counter* perf, const char *const name);
void perf_counter_init(perf_counter* perf, const double lspeed_windows);
void perf_counter_clear(perf_counter* perf);

#endif // __PERF_COUNTER__

#ifdef __PERF_COUNTER_IMPLEMENTATION__

#include <ncurses.h>

void print_perfw(perf_counter *perf, const char *const name)
{
  char coeffs[] = {'k', 'M', 'G', 'T', 'P'};
  size_t len = (sizeof(coeffs)) / sizeof(coeffs[0]);

  double time = timer_stop(&(perf->time));
  mpf_set_z(perf->speed, perf->counter);
  mpf_init_set_d(perf->time_, time);
  mpf_div(perf->speed, perf->speed, perf->time_);

  if (mpf_cmp(perf->speed, perf->peak_speed) > 0)
    mpf_set(perf->peak_speed, perf->speed);

  if (time - perf->lspeed_time >= perf->lspeed_window)
  {
    perf->lspeed_time = time;
    mpz_set_ui(perf->lcounter, 0);

    mpf_set_z(perf->lspeed, perf->counter);
    mpf_div(perf->lspeed, perf->lspeed, perf->time_);

    if (mpf_cmp(perf->lspeed, perf->peak_lspeed) > 0)
      mpf_set(perf->peak_lspeed, perf->lspeed);
  }

  char buff[256] = {0};

  if (mpf_cmp_ui(perf->speed, 1000) < 0)
  {
    gmp_snprintf(buff, 255, "%.2Ff %s/s peak: %.2Ff %s/s\n", perf->speed, name, perf->peak_speed, name);
    printw("%s", buff);
  }
  else
  {
    uint8_t k = 0;
    for (; k < len; ++k)
    {
      mpf_div_ui(perf->speed, perf->speed, 1000);
      mpf_div_ui(perf->peak_speed, perf->peak_speed, 1000);
      if (mpf_cmp_ui(perf->speed, 1000) < 0)
        break;
    }

    gmp_snprintf(buff, 255, "%.2Ff %c%s/s peak: %.2Ff %c%s/s\n", perf->speed, coeffs[k], name, perf->peak_speed, coeffs[k], name);
    printw("%s", buff);
  }

  if (mpf_cmp_ui(perf->lspeed, 1000) < 0)
  {
    gmp_snprintf(buff, 255, "local: %.2Ff %s/s peak: %.2Ff %s/s", perf->lspeed, name, perf->peak_lspeed, name);
    printw("%s\n", buff);
  }
  else
  {
    uint8_t k = 0;
    for (; k < len; ++k)
    {
      mpf_div_ui(perf->lspeed, perf->lspeed, 1000);
      mpf_div_ui(perf->peak_lspeed, perf->peak_lspeed, 1000);
      if (mpf_cmp_ui(perf->lspeed, 1000) < 0)
        break;
    }

    gmp_snprintf(buff, 255, "local: %.2Ff %c%s/s peak: %.2Ff %c%s/s", perf->lspeed, coeffs[k], name, perf->peak_lspeed, coeffs[k], name);
    printw("%s\n", buff);
  }

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

void perf_counter_init(perf_counter* perf, const double lspeed_window)
{
  timer_start(&perf->time);
  mpz_init_set_ui(perf->counter, 0);
  mpz_init_set_ui(perf->lcounter, 0);

  mpf_init_set_ui(perf->time_, 0);
  mpf_init_set_ui(perf->speed, 0);
  mpf_init_set_ui(perf->peak_speed, 0);
  mpf_init_set_ui(perf->lspeed, 0);
  mpf_init_set_ui(perf->peak_lspeed, 0);

  perf->lspeed_window = lspeed_window;
  perf->lspeed_time = timer_stop(&perf->time);
  return;
}

void perf_counter_clear(perf_counter* perf)
{
  mpz_clear(perf->counter);
  mpf_clears(perf->time_, perf->speed, perf->peak_speed, perf->lspeed, perf->peak_lspeed, NULL);

  return;
}

#endif
