/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSvtVp9Time_h
#define EbSvtVp9Time_h

#include "stdint.h"

#define NANOSECS_PER_SEC ((uint32_t)(1000000000L))

void eb_start_time(
    uint64_t *start_seconds,
    uint64_t *start_useconds);

void eb_finish_time(
    uint64_t *finish_seconds,
    uint64_t *finish_useconds);

void eb_compute_overall_elapsed_time(
    uint64_t start_seconds,
    uint64_t start_useconds,
    uint64_t finish_seconds,
    uint64_t finish_useconds,
    double  *duration);

void eb_compute_overall_elapsed_time_ms(
    uint64_t start_seconds,
    uint64_t start_useconds,
    uint64_t finish_seconds,
    uint64_t finish_useconds,
    double  *duration);

void eb_sleep(uint64_t milli_seconds);

void eb_injector(
    uint64_t processed_frame_count,
    uint32_t injector_frame_rate);

#endif // EbSvtVp9Time_h
/* File EOF */
