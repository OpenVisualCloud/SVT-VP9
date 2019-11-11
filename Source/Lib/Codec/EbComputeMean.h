/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeMean_h
#define EbComputeMean_h

#include "EbComputeMean_SSE2.h"
#include "EbComputeMean_C.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX2.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t(*EbComputeMeanFunc)(
    uint8_t *input_samples,
    uint32_t input_stride,
    uint32_t input_area_width,
    uint32_t input_area_height);

static const EbComputeMeanFunc compute_mean_func[2][ASM_TYPE_TOTAL] = {
    {
        // C_DEFAULT
        compute_mean,
        // AVX2
        eb_vp9_compute_mean8x8_avx2_intrin
    },
    {
        // C_DEFAULT
        compute_mean_of_squared_values,
        // AVX2
        eb_vp9_compute_mean_of_squared_values8x8_sse2_intrin
    }
};

#ifdef __cplusplus
}
#endif

#endif
