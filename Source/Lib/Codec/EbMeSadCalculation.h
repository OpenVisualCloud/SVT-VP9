/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMeSadCalculation_h
#define EbMeSadCalculation_h

#include "EbMeSadCalculation_C.h"
#include "EbMeSadCalculation_SSE2.h"

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

/***************************************
* Function Types
***************************************/
typedef void(*EbSadCalculation8x8and16x16Type)(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint32_t  *p_sad16x16);

typedef void(*EbSadCalculation32x32and64x64Type)(
    uint32_t  *p_sad16x16,
    uint32_t  *p_best_sad32x32,
    uint32_t  *p_best_sad64x64,
    uint32_t  *p_best_mv32x32,
    uint32_t  *p_best_mv64x64,
    uint32_t   mv);

typedef void(*EbInializeBuffer32Bits)(
    uint32_t *pointer,
    uint32_t  count128,
    uint32_t  count32,
    uint32_t  value);

/***************************************
* Function Tables
***************************************/
static EbSadCalculation8x8and16x16Type eb_vp9_sad_calculation_8x8_16x16_func_ptr_array[ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    eb_vp9_sad_calculation_8x8_16x16,
    // AVX2
    eb_vp9_sad_calculation_8x8_16x16_sse2_intrin,
};

static EbSadCalculation32x32and64x64Type eb_vp9_sad_calculation_32x32_64x64_func_ptr_array[ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    eb_vp9_sad_calculation_32x32_64x64,
    // AVX2
    eb_vp9_sad_calculation_32x32_64x64_sse2_intrin,
};

static EbInializeBuffer32Bits eb_vp9_initialize_buffer_32bits_func_ptr_array[ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    eb_vp9_initialize_buffer_32bits,
    // AVX2
    eb_vp9_initialize_buffer_32bits_sse2_intrin
};

#ifdef __cplusplus
}
#endif
#endif // EbMeSadCalculation_h
