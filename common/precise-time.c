/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2014 Telegram Messenger Inc
              2014 Anton Maydell
*/
#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
/* unistd.h defines _POSIX_TIMERS */
#include <unistd.h>
#endif

#include "precise-time.h"
__thread int now;
__thread double precise_now;
__thread long long precise_now_rdtsc;
long long precise_time;
long long precise_time_rdtsc;

double get_utime_monotonic (void) __attribute__ ((weak));
double get_utime_monotonic (void) {
#if defined(_WIN32)
  static LARGE_INTEGER frequency;
  static int initialized = 0;
  LARGE_INTEGER counter;
  if (!initialized) {
    QueryPerformanceFrequency(&frequency);
    initialized = 1;
  }
  QueryPerformanceCounter(&counter);
  precise_now_rdtsc = rdtsc();
  return precise_now = (double)counter.QuadPart / frequency.QuadPart;
#elif _POSIX_TIMERS
  struct timespec T;
  assert (clock_gettime (CLOCK_MONOTONIC, &T) >= 0);
  precise_now_rdtsc = rdtsc ();
  return precise_now = T.tv_sec + (double) T.tv_nsec * 1e-9;
#else
#error "No high-precision clock"
  struct timespec T = {0, 0};
  return precise_now = time ();
#endif
}

double get_double_time (void) {
  static double last_double_time = -1;
  static long long next_rdtsc;
  long long cur_rdtsc = rdtsc ();
  if (cur_rdtsc > next_rdtsc) {
#if defined(_WIN32)
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    last_double_time = (double)uli.QuadPart / 10000000.0 - 11644473600.0;
#else
    struct timeval tv;
    gettimeofday (&tv, NULL);
    last_double_time = tv.tv_sec + 1e-6 * tv.tv_usec;
#endif
    next_rdtsc = cur_rdtsc + 1000000;
    return last_double_time;
  } else {
    return last_double_time;
  }
}

double get_utime (int clock_id) {
#if defined(_WIN32)
  static LARGE_INTEGER frequency;
  static int initialized = 0;
  LARGE_INTEGER counter;
  if (!initialized) {
    QueryPerformanceFrequency(&frequency);
    initialized = 1;
  }
  QueryPerformanceCounter(&counter);
  double res = (double)counter.QuadPart / frequency.QuadPart;
#elif _POSIX_TIMERS
  struct timespec T;
  assert (clock_gettime (clock_id, &T) >= 0);
  double res = T.tv_sec + (double) T.tv_nsec * 1e-9;
#else
#error "No high-precision clock"
  double res = time ();
#endif
  if (clock_id == CLOCK_REALTIME) {
    precise_time = (long long) (res * (1LL << 32));
    precise_time_rdtsc = rdtsc ();
  }
  return res;
}

long long get_precise_time (unsigned precision) {
  unsigned long long diff = rdtsc() - precise_time_rdtsc;
  if (diff > precision) {
    get_utime (CLOCK_REALTIME);
  }
  return precise_time;
}
