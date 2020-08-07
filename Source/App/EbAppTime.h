/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

#ifndef EbAppTime_h
#define EbAppTime_h

#include <stdint.h>

void app_svt_vp9_sleep(const unsigned milliseconds);
double app_svt_vp9_compute_overall_elapsed_time(const uint64_t start_seconds,
                                                const uint64_t start_useconds,
                                                const uint64_t finish_seconds,
                                                const uint64_t finish_useconds);
void app_svt_vp9_get_time(uint64_t *const seconds, uint64_t *const useconds);

#endif  // EbAppTime_h
