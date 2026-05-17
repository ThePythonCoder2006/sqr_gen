#ifndef __TIMER__
#define __TIMER__

#ifndef __HELPER_BUFF_SIZE__
#define __HELPER_BUFF_SIZE__ 64
#endif

#ifdef _WIN32
#include <windows.h>
#undef MOUSE_MOVED

typedef struct timer_s
{
  LARGE_INTEGER frequency;
  LARGE_INTEGER start;
} timer;

#else

#include "nob.h"

typedef struct timer_s
{
  uint64_t nanos_start;
} timer;

#endif

void timer_start(timer *timer);
// return time elapst in seconds
double timer_stop(timer *timer);

#endif // __TIMER__

#ifdef __TIMER_IMPLEMENTATION__
#ifdef _WIN32

void timer_start(timer *timer)
{
  QueryPerformanceFrequency(&(timer->frequency));
  QueryPerformanceCounter(&(timer->start));
  return;
}

double timer_stop(timer *timer)
{
  double interval;
  LARGE_INTEGER end;
  QueryPerformanceCounter(&end);
  interval = (double)(end.QuadPart - timer->start.QuadPart) / timer->frequency.QuadPart;
  return interval;
}

#else

void timer_start(timer *timer)
{
	timer->nanos_start = nob_nanos_since_unspecified_epoch();
	return;
}

double timer_stop(timer *timer)
{
	uint64_t nanos_stop = nob_nanos_since_unspecified_epoch();
	return ((double) (nanos_stop - timer->nanos_start)) / NOB_NANOS_PER_SEC;
}

#endif

#endif // __TIMER_IMPLEMENTATION__
