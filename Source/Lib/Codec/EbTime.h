/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

#ifndef EbTime_h
#define EbTime_h

#include <stdint.h>

void svt_vp9_sleep(const int milliseconds);
double svt_vp9_compute_overall_elapsed_time_ms(const uint64_t start_seconds,
                                               const uint64_t start_useconds,
                                               const uint64_t finish_seconds,
                                               const uint64_t finish_useconds);
double svt_vp9_compute_overall_elapsed_time(const uint64_t start_seconds,
                                            const uint64_t start_useconds,
                                            const uint64_t finish_seconds,
                                            const uint64_t finish_useconds);
void svt_vp9_get_time(uint64_t *const seconds, uint64_t *const useconds);

#endif  // EbTime_h
