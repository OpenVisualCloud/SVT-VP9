// This file is generated. Do not edit.
#ifndef VP9_RTCD_H_
#define VP9_RTCD_H_

#ifdef RTCD_C
#define RTCD_EXTERN
#else
#define RTCD_EXTERN extern
#endif

/*
 * VP9
 */

#include "vp9_common.h"
#include "vp9_enums.h"
#include "vp9_filter.h"
#include "vpx_dsp_common.h"
#include "vpx_dsp_rtcd.h"
#include "EbDefinitions.h"

struct macroblockd;

/* Encoder forward decls */
struct macroblock;
struct vp9_variance_vtable;
struct search_site_config;
struct mv;
union int_mv;
struct yv12_buffer_config;

#ifdef __cplusplus
extern "C" {
#endif

void eb_vp9_fht16x16_c(const int16_t *input, tran_low_t *output, int stride, int tx_type);
void eb_vp9_fht16x16_avx2(const int16_t *input, tran_low_t *output, int stride, int tx_type);
RTCD_EXTERN void (*eb_vp9_fht16x16)(const int16_t *input, tran_low_t *output, int stride, int tx_type);

void eb_vp9_fht4x4_c(const int16_t *input, tran_low_t *output, int stride, int tx_type);
void eb_vp9_fht4x4_sse2(const int16_t *input, tran_low_t *output, int stride, int tx_type);
RTCD_EXTERN void (*eb_vp9_fht4x4)(const int16_t *input, tran_low_t *output, int stride, int tx_type);

void eb_vp9_fht8x8_c(const int16_t *input, tran_low_t *output, int stride, int tx_type);
void eb_vp9_fht8x8_avx2(const int16_t *input, tran_low_t *output, int stride, int tx_type);
RTCD_EXTERN void (*eb_vp9_fht8x8)(const int16_t *input, tran_low_t *output, int stride, int tx_type);

void eb_vp9_iht16x16_256_add_c(const tran_low_t *input, uint8_t *output, int pitch, int tx_type);
void eb_vp9_iht16x16_256_add_avx2(const tran_low_t *input, uint8_t *output, int pitch, int tx_type);
RTCD_EXTERN void (*eb_vp9_iht16x16_256_add)(const tran_low_t *input, uint8_t *output, int pitch, int tx_type);

void eb_vp9_iht4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
void eb_vp9_iht4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
RTCD_EXTERN void (*eb_vp9_iht4x4_16_add)(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);

void eb_vp9_iht8x8_64_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
void eb_vp9_iht8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
RTCD_EXTERN void (*eb_vp9_iht8x8_64_add)(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);

#ifdef RTCD_C

static void setup_rtcd_internal_vp9(uint32_t asm_type) {
    int flags = 0;

    if (asm_type > AVX2_MASK) {
        flags |= HAS_AVX2;
        flags |= HAS_AVX;
        flags |= HAS_SSE4_1;
        flags |= HAS_SSSE3;
        flags |= HAS_SSE3;
        flags |= HAS_SSE2;
        flags |= HAS_SSE;
        flags |= HAS_MMX;
    } else if (asm_type > PREAVX2_MASK) {
        flags |= HAS_AVX;
        flags |= HAS_SSE4_1;
        flags |= HAS_SSSE3;
        flags |= HAS_SSE3;
        flags |= HAS_SSE2;
        flags |= HAS_SSE;
        flags |= HAS_MMX;
    }

    eb_vp9_fht16x16 = eb_vp9_fht16x16_c;
    if (flags & HAS_AVX2)
        eb_vp9_fht16x16 = eb_vp9_fht16x16_avx2;
    eb_vp9_fht4x4 = eb_vp9_fht4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_fht4x4 = eb_vp9_fht4x4_sse2;
    eb_vp9_fht8x8 = eb_vp9_fht8x8_c;
    if (flags & HAS_AVX2)
        eb_vp9_fht8x8 = eb_vp9_fht8x8_avx2;
    eb_vp9_iht16x16_256_add = eb_vp9_iht16x16_256_add_c;
    if (flags & HAS_AVX2)
        eb_vp9_iht16x16_256_add = eb_vp9_iht16x16_256_add_avx2;
    eb_vp9_iht4x4_16_add = eb_vp9_iht4x4_16_add_c;
    if (flags & HAS_SSE2)
        eb_vp9_iht4x4_16_add = eb_vp9_iht4x4_16_add_sse2;
    eb_vp9_iht8x8_64_add = eb_vp9_iht8x8_64_add_c;
    if (flags & HAS_SSE2)
        eb_vp9_iht8x8_64_add = eb_vp9_iht8x8_64_add_sse2;
}

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
