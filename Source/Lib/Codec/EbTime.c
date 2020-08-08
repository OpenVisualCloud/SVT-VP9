/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

#ifdef _WIN32
#include <sys/timeb.h>
#include <windows.h>
#elif !defined(__USE_POSIX199309)
#define __USE_POSIX199309
#endif

#include <time.h>

#if !defined(CLOCK_MONOTONIC) && !defined(_WIN32)
#include <sys/time.h>
#endif

#include "EbTime.h"

void svt_vp9_sleep(const int milliseconds) {
    if (!milliseconds) return;
#ifdef _WIN32
    Sleep(milliseconds);
#else
    nanosleep(&(struct timespec){milliseconds / 1000,
                                 (milliseconds % 1000) * 1000000},
              NULL);
#endif
}

double svt_vp9_compute_overall_elapsed_time_ms(const uint64_t start_seconds,
                                               const uint64_t start_useconds,
                                               const uint64_t finish_seconds,
                                               const uint64_t finish_useconds) {
    const int64_t s_diff = (int64_t)finish_seconds - (int64_t)start_seconds,
                  u_diff = (int64_t)finish_useconds - (int64_t)start_useconds;
    return (double)s_diff * 1000.0 + (double)u_diff / 1000.0 + 0.5;
}

double svt_vp9_compute_overall_elapsed_time(const uint64_t start_seconds,
                                            const uint64_t start_useconds,
                                            const uint64_t finish_seconds,
                                            const uint64_t finish_useconds) {
    return svt_vp9_compute_overall_elapsed_time_ms(
               start_seconds, start_useconds, finish_seconds, finish_useconds) /
           1000.0;
}

void svt_vp9_get_time(uint64_t *const seconds, uint64_t *const useconds) {
#ifdef _WIN32
    struct _timeb curr_time;
    _ftime_s(&curr_time);
    *seconds = curr_time.time;
    *useconds = curr_time.millitm;
#elif defined(CLOCK_MONOTONIC)
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    *seconds = curr_time.tv_sec;
    *useconds = curr_time.tv_nsec / 1000;
#else
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    *seconds = curr_time.tv_sec;
    *useconds = curr_time.tv_usec;
#endif
}
