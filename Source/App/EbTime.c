/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#include <stdint.h>
#ifdef _WIN32
#include <stdlib.h>
//#if  (WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
#include <time.h>
#include <windows.h>
//#endif

#elif __linux__
#include <stdio.h>
#include <stdlib.h>
//#if   (LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
#include <sys/time.h>
#include <time.h>
//#endif

#else
#error OS/Platform not supported.
#endif

#include "EbSvtVp9Time.h"

void eb_sleep(uint64_t milli_seconds) {

    if (milli_seconds) {
#if __linux__
        struct timespec req, rem;
        req.tv_sec = (int)(milli_seconds / 1000);
        milli_seconds -= req.tv_sec * 1000;
        req.tv_nsec = milli_seconds * 1000000UL;
        nanosleep(&req, &rem);
#elif _WIN32
        Sleep((DWORD)milli_seconds);
#else
#error OS Not supported
#endif
    }
}

void eb_injector(
    uint64_t processed_frame_count,
    uint32_t injector_frame_rate)
{
#if __linux__
    uint64_t                current_times_seconds = 0;
    uint64_t                current_timesu_seconds = 0;
    static uint64_t         start_times_seconds;
    static uint64_t         start_timesu_seconds;
#elif _WIN32
    static LARGE_INTEGER    start_count;               // this is the start time
    static LARGE_INTEGER    counter_freq;              // performance counter frequency
    LARGE_INTEGER           now_count;                 // this is the current time
#else
#error OS Not supported
#endif

    double                  injector_interval = (double)(1 << 16) / (double)injector_frame_rate;     // 1.0 / injector frame rate (in this case, 1.0/encodRate)
    double                  elapsed_time;
    double                  predicted_time;
    int                     buffer_frames = 1;         // How far ahead of time should we let it get
    int                     milli_sec_ahead;
    static int              first_time = 0;

    if (first_time == 0)
    {
        first_time = 1;

#if __linux__
        eb_start_time(&start_times_seconds, &start_timesu_seconds);
#elif _WIN32
        QueryPerformanceFrequency(&counter_freq);
        QueryPerformanceCounter(&start_count);
#endif
    }
    else
    {

#if __linux__
        eb_finish_time(&current_times_seconds, &current_timesu_seconds);
        eb_compute_overall_elapsed_time(start_times_seconds, start_timesu_seconds, current_times_seconds, current_timesu_seconds, &elapsed_time);
#elif _WIN32
        QueryPerformanceCounter(&now_count);
        elapsed_time = (double)(now_count.QuadPart - start_count.QuadPart) / (double)counter_freq.QuadPart;
#endif

        predicted_time = (processed_frame_count - buffer_frames) * injector_interval;
        milli_sec_ahead = (int)(1000 * (predicted_time - elapsed_time));
        if (milli_sec_ahead>0)
        {
            //  timeBeginPeriod(1);
            eb_sleep(milli_sec_ahead);
            //  timeEndPeriod (1);
        }
    }
}
