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

    int64_t eb_vp9_block_error_c(const tran_low_t *coeff, const tran_low_t *dqcoeff, intptr_t block_size, int64_t *ssz);
    int64_t eb_vp9_block_error_avx2(const tran_low_t *coeff, const tran_low_t *dqcoeff, intptr_t block_size, int64_t *ssz);
    RTCD_EXTERN int64_t(*eb_vp9_block_error)(const tran_low_t *coeff, const tran_low_t *dqcoeff, intptr_t block_size, int64_t *ssz);

    int64_t eb_vp9_block_error_fp_c(const tran_low_t *coeff, const tran_low_t *dqcoeff, int block_size);
    int64_t eb_vp9_block_error_fp_avx2(const tran_low_t *coeff, const tran_low_t *dqcoeff, int block_size);
    RTCD_EXTERN int64_t(*eb_vp9_block_error_fp)(const tran_low_t *coeff, const tran_low_t *dqcoeff, int block_size);

    int eb_vp9_diamond_search_sad_c(const struct macroblock *x, const struct search_site_config *cfg, struct mv *ref_mv, struct mv *best_mv, int search_param, int sad_per_bit, int *num00, const struct vp9_variance_vtable *fn_ptr, const struct mv *center_mv);
    int eb_vp9_diamond_search_sad_avx(const struct macroblock *x, const struct search_site_config *cfg, struct mv *ref_mv, struct mv *best_mv, int search_param, int sad_per_bit, int *num00, const struct vp9_variance_vtable *fn_ptr, const struct mv *center_mv);
    RTCD_EXTERN int(*eb_vp9_diamond_search_sad)(const struct macroblock *x, const struct search_site_config *cfg, struct mv *ref_mv, struct mv *best_mv, int search_param, int sad_per_bit, int *num00, const struct vp9_variance_vtable *fn_ptr, const struct mv *center_mv);

    void vp9_fdct8x8_quant_c(const int16_t *input, int stride, tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
    void vp9_fdct8x8_quant_ssse3(const int16_t *input, int stride, tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
    RTCD_EXTERN void(*vp9_fdct8x8_quant)(const int16_t *input, int stride, tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);

    void eb_vp9_fht16x16_c(const int16_t *input, tran_low_t *output, int stride, int tx_type);
    void vp9_fht16x16_avx2(const int16_t *input, tran_low_t *output, int stride, int tx_type);
    RTCD_EXTERN void(*vp9_fht16x16)(const int16_t *input, tran_low_t *output, int stride, int tx_type);

    void eb_vp9_fht4x4_c(const int16_t *input, tran_low_t *output, int stride, int tx_type);
    void eb_vp9_fht4x4_sse2(const int16_t *input, tran_low_t *output, int stride, int tx_type);
    RTCD_EXTERN void(*vp9_fht4x4)(const int16_t *input, tran_low_t *output, int stride, int tx_type);

    void eb_vp9_fht8x8_c(const int16_t *input, tran_low_t *output, int stride, int tx_type);
    void vp9_fht8x8_avx2(const int16_t *input, tran_low_t *output, int stride, int tx_type);
    RTCD_EXTERN void(*vp9_fht8x8)(const int16_t *input, tran_low_t *output, int stride, int tx_type);

    void eb_vp9_fwht4x4_c(const int16_t *input, tran_low_t *output, int stride);
    void vp9_fwht4x4_sse2(const int16_t *input, tran_low_t *output, int stride);
    RTCD_EXTERN void(*vp9_fwht4x4)(const int16_t *input, tran_low_t *output, int stride);

    void eb_vp9_iht16x16_256_add_c(const tran_low_t *input, uint8_t *output, int pitch, int tx_type);
    void vp9_iht16x16_256_add_avx2(const tran_low_t *input, uint8_t *output, int pitch, int tx_type);
    RTCD_EXTERN void(*vp9_iht16x16_256_add)(const tran_low_t *input, uint8_t *output, int pitch, int tx_type);

    void eb_vp9_iht4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
    void eb_vp9_iht4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
    RTCD_EXTERN void(*vp9_iht4x4_16_add)(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);

    void eb_vp9_iht8x8_64_add_c(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
    void eb_vp9_iht8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);
    RTCD_EXTERN void(*vp9_iht8x8_64_add)(const tran_low_t *input, uint8_t *dest, int stride, int tx_type);

    void eb_vp9_quantize_fp_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
    void eb_vp9_quantize_fp_avx2(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
    RTCD_EXTERN void(*eb_vp9_quantize_fp)(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);

    void eb_vp9_quantize_fp_32x32_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
    RTCD_EXTERN void(*eb_vp9_quantize_fp_32x32)(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *round_ptr, const int16_t *quant_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);

    void eb_vp9_scale_and_extend_frame_c(const struct yv12_buffer_config *src, struct yv12_buffer_config *dst, INTERP_FILTER filter_type, int phase_scaler);
    void eb_vp9_scale_and_extend_frame_ssse3(const struct yv12_buffer_config *src, struct yv12_buffer_config *dst, INTERP_FILTER filter_type, int phase_scaler);
    RTCD_EXTERN void(*eb_vp9_scale_and_extend_frame)(const struct yv12_buffer_config *src, struct yv12_buffer_config *dst, INTERP_FILTER filter_type, int phase_scaler);

    void vp9_temporal_filter_apply_c(const uint8_t *frame1, unsigned int stride, const uint8_t *frame2, unsigned int block_width, unsigned int block_height, int strength, int filter_weight, uint32_t *accumulator, uint16_t *count);
    void vp9_temporal_filter_apply_sse4_1(const uint8_t *frame1, unsigned int stride, const uint8_t *frame2, unsigned int block_width, unsigned int block_height, int strength, int filter_weight, uint32_t *accumulator, uint16_t *count);
    RTCD_EXTERN void(*vp9_temporal_filter_apply)(const uint8_t *frame1, unsigned int stride, const uint8_t *frame2, unsigned int block_width, unsigned int block_height, int strength, int filter_weight, uint32_t *accumulator, uint16_t *count);

    void vp9_rtcd(void);

#ifdef RTCD_C

    static void setup_rtcd_internal_vp9(uint32_t asm_type)
    {

        int flags = 0;

        if (asm_type > AVX2_MASK)
        {
            flags |= HAS_AVX2;
            flags |= HAS_AVX;
            flags |= HAS_SSE4_1;
            flags |= HAS_SSSE3;
            flags |= HAS_SSE3;
            flags |= HAS_SSE2;
            flags |= HAS_SSE;
            flags |= HAS_MMX;
        }
        else if (asm_type > PREAVX2_MASK)
        {
            flags |= HAS_AVX;
            flags |= HAS_SSE4_1;
            flags |= HAS_SSSE3;
            flags |= HAS_SSE3;
            flags |= HAS_SSE2;
            flags |= HAS_SSE;
            flags |= HAS_MMX;
        }

#if 0
        eb_vp9_block_error = eb_vp9_block_error_c;
        if (flags & HAS_AVX2) eb_vp9_block_error = eb_vp9_block_error_avx2;
        eb_vp9_block_error_fp = eb_vp9_block_error_fp_c;
        if (flags & HAS_AVX2) eb_vp9_block_error_fp = eb_vp9_block_error_fp_avx2;
        eb_vp9_diamond_search_sad = eb_vp9_diamond_search_sad_c;
        if (flags & HAS_AVX) eb_vp9_diamond_search_sad = eb_vp9_diamond_search_sad_avx;
        vp9_fdct8x8_quant = vp9_fdct8x8_quant_c;
        if (flags & HAS_SSSE3) vp9_fdct8x8_quant = vp9_fdct8x8_quant_ssse3;
#endif
        vp9_fht16x16 = eb_vp9_fht16x16_c;
        if (flags & HAS_AVX2) vp9_fht16x16 = vp9_fht16x16_avx2;
        vp9_fht4x4 = eb_vp9_fht4x4_c;
        if (flags & HAS_SSE2) vp9_fht4x4 = eb_vp9_fht4x4_sse2;
        vp9_fht8x8 = eb_vp9_fht8x8_c;
        if (flags & HAS_AVX2) vp9_fht8x8 = vp9_fht8x8_avx2;
#if 0
        vp9_fwht4x4 = eb_vp9_fwht4x4_c;
        if (flags & HAS_SSE2) vp9_fwht4x4 = vp9_fwht4x4_sse2;
#endif
        vp9_iht16x16_256_add = eb_vp9_iht16x16_256_add_c;
        if (flags & HAS_AVX2) vp9_iht16x16_256_add = vp9_iht16x16_256_add_avx2;
        vp9_iht4x4_16_add = eb_vp9_iht4x4_16_add_c;
        if (flags & HAS_SSE2) vp9_iht4x4_16_add = eb_vp9_iht4x4_16_add_sse2;
        vp9_iht8x8_64_add = eb_vp9_iht8x8_64_add_c;
        if (flags & HAS_SSE2) vp9_iht8x8_64_add = eb_vp9_iht8x8_64_add_sse2;
#if 0
        eb_vp9_quantize_fp = eb_vp9_quantize_fp_c;
        if (flags & HAS_AVX2) eb_vp9_quantize_fp = eb_vp9_quantize_fp_avx2;
        eb_vp9_quantize_fp_32x32 = eb_vp9_quantize_fp_32x32_c;
        eb_vp9_scale_and_extend_frame = eb_vp9_scale_and_extend_frame_c;
        if (flags & HAS_SSSE3) eb_vp9_scale_and_extend_frame = eb_vp9_scale_and_extend_frame_ssse3;
        vp9_temporal_filter_apply = vp9_temporal_filter_apply_c;
        if (flags & HAS_SSE4_1) vp9_temporal_filter_apply = vp9_temporal_filter_apply_sse4_1;
#endif

    }

#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
