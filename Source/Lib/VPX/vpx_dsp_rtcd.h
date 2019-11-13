// This file is generated. Do not edit.
#ifndef VPX_DSP_RTCD_H_
#define VPX_DSP_RTCD_H_

#ifdef RTCD_C
#define RTCD_EXTERN
#else
#define RTCD_EXTERN extern
#endif

/*
 * DSP
 */

#define HAS_MMX 0x001
#define HAS_SSE 0x002
#define HAS_SSE2 0x004
#define HAS_SSE3 0x008
#define HAS_SSSE3 0x010
#define HAS_SSE4_1 0x020
#define HAS_AVX 0x040
#define HAS_AVX2 0x080
#define HAS_AVX512 0x100

#define INLINE __inline

#include <stddef.h>
#include <stdint.h>
#include "vpx_dsp_common.h"
#include "vpx_filter.h"

#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int vpx_avg_4x4_c(const uint8_t *, int p);
unsigned int vpx_avg_4x4_sse2(const uint8_t *, int p);
RTCD_EXTERN unsigned int(*vpx_avg_4x4)(const uint8_t *, int p);

unsigned int vpx_avg_8x8_c(const uint8_t *, int p);
unsigned int vpx_avg_8x8_sse2(const uint8_t *, int p);
RTCD_EXTERN unsigned int(*vpx_avg_8x8)(const uint8_t *, int p);

void vpx_comp_avg_pred_c(uint8_t *comp_pred, const uint8_t *pred, int width, int height, const uint8_t *ref, int ref_stride);
void vpx_comp_avg_pred_sse2(uint8_t *comp_pred, const uint8_t *pred, int width, int height, const uint8_t *ref, int ref_stride);
RTCD_EXTERN void(*vpx_comp_avg_pred)(uint8_t *comp_pred, const uint8_t *pred, int width, int height, const uint8_t *ref, int ref_stride);

// TT temporary untill we fix the linking error
void eb_vp9_filter_block1d8_v2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_v2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_v2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_h2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d8_h2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_h2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_v2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_h2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d8_v2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d8_h2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_v2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_h2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr, ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);

void eb_vp9_convolve8_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*eb_vp9_convolve8)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_avg_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_avg_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*eb_vp9_convolve8_avg)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_avg_horiz_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_avg_horiz_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*eb_vp9_convolve8_avg_horiz)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_avg_vert_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_avg_vert_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*eb_vp9_convolve8_avg_vert)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_horiz_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_horiz_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*eb_vp9_convolve8_horiz)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_vert_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_vert_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*eb_vp9_convolve8_vert)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve_avg_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void vpx_convolve_avg_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_convolve_avg)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve_copy_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void vpx_convolve_copy_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_convolve_copy)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_d117_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d117_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d117_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_32x32_avx2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d117_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d117_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d117_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d117_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d117_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d135_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d135_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d135_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_32x32_avx2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d135_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d135_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d135_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d135_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d135_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d153_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d153_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d153_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d153_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d153_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d153_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d153_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d153_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d153_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d207_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d207_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d207_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d207_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d207_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d207_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d207_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d207_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d45_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d45_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d45_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d45_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d45_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d45_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d45_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d45_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d45e_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d45e_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d63_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d63_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d63_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d63_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d63_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d63_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d63_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d63_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d63e_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_d63e_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_128_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_128_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_128_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_128_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_128_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_128_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_128_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_128_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_left_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_left_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_left_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_left_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_left_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_left_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_left_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_left_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_top_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_top_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_top_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_top_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_top_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_top_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_dc_top_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_dc_top_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_fdct16x16_c(const int16_t *input, tran_low_t *output, int stride);
void vpx_fdct16x16_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_fdct16x16)(const int16_t *input, tran_low_t *output, int stride);

#if 0
void eb_vp9_fdct16x16_1_c(const int16_t *input, tran_low_t *output, int stride);
void vpx_fdct16x16_1_sse2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_fdct16x16_1)(const int16_t *input, tran_low_t *output, int stride);
#endif
void eb_vp9_fdct32x32_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vp9_fdct32x32_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*eb_vp9_fdct32x32)(const int16_t *input, tran_low_t *output, int stride);

void vpx_partial_fdct32x32_c(const int16_t *input, tran_low_t *output, int stride);
void vpx_partial_fdct32x32_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_partial_fdct32x32)(const int16_t *input, tran_low_t *output, int stride);

#if 0
void eb_vp9_fdct32x32_1_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vp9_fdct32x32_1_sse2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*eb_vp9_fdct32x32_1)(const int16_t *input, tran_low_t *output, int stride);

void eb_vp9_fdct32x32_rd_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vp9_fdct32x32_rd_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*eb_vp9_fdct32x32_rd)(const int16_t *input, tran_low_t *output, int stride);
#endif
void eb_vp9_fdct4x4_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vp9_fdct4x4_sse2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_fdct4x4)(const int16_t *input, tran_low_t *output, int stride);

#if 0
void eb_vp9_fdct4x4_1_c(const int16_t *input, tran_low_t *output, int stride);
void vpx_fdct4x4_1_sse2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_fdct4x4_1)(const int16_t *input, tran_low_t *output, int stride);
#endif
void eb_vp9_fdct8x8_c(const int16_t *input, tran_low_t *output, int stride);
void vpx_fdct8x8_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_fdct8x8)(const int16_t *input, tran_low_t *output, int stride);

#if 0
void eb_vp9_fdct8x8_1_c(const int16_t *input, tran_low_t *output, int stride);
void vpx_fdct8x8_1_sse2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void(*vpx_fdct8x8_1)(const int16_t *input, tran_low_t *output, int stride);

void vpx_get16x16var_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse, int *sum);
void vpx_get16x16var_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse, int *sum);
RTCD_EXTERN void(*vpx_get16x16var)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse, int *sum);

unsigned int vpx_get4x4sse_cs_c(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride);
RTCD_EXTERN unsigned int(*vpx_get4x4sse_cs)(const unsigned char *src_ptr, int source_stride, const unsigned char *ref_ptr, int  ref_stride);

void vpx_get8x8var_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse, int *sum);
void vpx_get8x8var_sse2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse, int *sum);
RTCD_EXTERN void(*vpx_get8x8var)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse, int *sum);

unsigned int vpx_get_mb_ss_c(const int16_t *);
unsigned int vpx_get_mb_ss_sse2(const int16_t *);
RTCD_EXTERN unsigned int(*vpx_get_mb_ss)(const int16_t *);
#endif

void eb_vp9_h_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_h_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_h_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_h_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_h_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_h_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_h_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_h_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

#if 0
void vpx_hadamard_16x16_c(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);

void vpx_hadamard_16x16_avx2(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);
RTCD_EXTERN void(*vpx_hadamard_16x16)(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);

void vpx_hadamard_32x32_c(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);
void vpx_hadamard_32x32_avx2(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);
RTCD_EXTERN void(*vpx_hadamard_32x32)(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);

void vpx_hadamard_8x8_c(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);
void vpx_hadamard_8x8_sse2(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);
RTCD_EXTERN void(*vpx_hadamard_8x8)(const int16_t *src_diff, ptrdiff_t src_stride, int16_t *coeff);

void eb_vp9_he_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*vpx_he_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
#endif
void eb_vp9_idct16x16_10_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_10_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct16x16_10_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct16x16_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct16x16_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct16x16_256_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_256_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct16x16_256_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct16x16_38_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_38_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct16x16_38_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_1024_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void vpx_idct32x32_1024_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct32x32_1024_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_135_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct32x32_135_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*eb_vp9_idct32x32_135_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void vpx_idct32x32_1_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct32x32_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_34_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct32x32_34_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*eb_vp9_idct32x32_34_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct4x4_16_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct4x4_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct4x4_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct4x4_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct8x8_12_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct8x8_12_add_ssse3(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*eb_vp9_idct8x8_12_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct8x8_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct8x8_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct8x8_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct8x8_64_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_idct8x8_64_add)(const tran_low_t *input, uint8_t *dest, int stride);

int16_t vpx_int_pro_col_c(const uint8_t *ref, const int width);
int16_t vpx_int_pro_col_sse2(const uint8_t *ref, const int width);
RTCD_EXTERN int16_t(*vpx_int_pro_col)(const uint8_t *ref, const int width);

void vpx_int_pro_row_c(int16_t *hbuf, const uint8_t *ref, const int ref_stride, const int height);
void vpx_int_pro_row_sse2(int16_t *hbuf, const uint8_t *ref, const int ref_stride, const int height);
RTCD_EXTERN void(*vpx_int_pro_row)(int16_t *hbuf, const uint8_t *ref, const int ref_stride, const int height);

void eb_vp9_iwht4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void vpx_iwht4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_iwht4x4_16_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_iwht4x4_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void(*vpx_iwht4x4_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_lpf_horizontal_16_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_horizontal_16_avx2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*eb_vp9_lpf_horizontal_16)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_horizontal_16_dual_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_horizontal_16_dual_avx2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*eb_vp9_lpf_horizontal_16_dual)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

#if 1
void eb_vp9_lpf_horizontal_4_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_horizontal_4_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*vpx_lpf_horizontal_4)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_horizontal_4_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
void eb_vp9_lpf_horizontal_4_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
RTCD_EXTERN void(*vpx_lpf_horizontal_4_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);

void eb_vp9_lpf_horizontal_8_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_horizontal_8_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*vpx_lpf_horizontal_8)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_horizontal_8_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
void eb_vp9_lpf_horizontal_8_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
RTCD_EXTERN void(*vpx_lpf_horizontal_8_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);

void eb_vp9_lpf_vertical_16_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_vertical_16_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*vpx_lpf_vertical_16)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_vertical_16_dual_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void vpx_lpf_vertical_16_dual_avx2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

RTCD_EXTERN void(*vpx_lpf_vertical_16_dual)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_vertical_4_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_vertical_4_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*vpx_lpf_vertical_4)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_vertical_4_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
void eb_vp9_lpf_vertical_4_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
RTCD_EXTERN void(*vpx_lpf_vertical_4_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);

void eb_vp9_lpf_vertical_8_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_vertical_8_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
RTCD_EXTERN void(*vpx_lpf_vertical_8)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);

void eb_vp9_lpf_vertical_8_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
void eb_vp9_lpf_vertical_8_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);
RTCD_EXTERN void(*vpx_lpf_vertical_8_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1, const uint8_t *thresh1);

void vpx_mbpost_proc_across_ip_c(unsigned char *dst, int pitch, int rows, int cols, int flimit);
void vpx_mbpost_proc_across_ip_sse2(unsigned char *dst, int pitch, int rows, int cols, int flimit);
RTCD_EXTERN void(*vpx_mbpost_proc_across_ip)(unsigned char *dst, int pitch, int rows, int cols, int flimit);

void vpx_mbpost_proc_down_c(unsigned char *dst, int pitch, int rows, int cols, int flimit);
void vpx_mbpost_proc_down_sse2(unsigned char *dst, int pitch, int rows, int cols, int flimit);
RTCD_EXTERN void(*vpx_mbpost_proc_down)(unsigned char *dst, int pitch, int rows, int cols, int flimit);

void vpx_minmax_8x8_c(const uint8_t *s, int p, const uint8_t *d, int dp, int *min, int *max);
void vpx_minmax_8x8_sse2(const uint8_t *s, int p, const uint8_t *d, int dp, int *min, int *max);
RTCD_EXTERN void(*vpx_minmax_8x8)(const uint8_t *s, int p, const uint8_t *d, int dp, int *min, int *max);

unsigned int eb_vp9_mse16x16_c(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
unsigned int eb_vp9_mse16x16_avx2(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*eb_vp9_mse16x16)(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);

unsigned int eb_vp9_mse16x8_c(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
unsigned int eb_vp9_mse16x8_avx2(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*eb_vp9_mse16x8)(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);

unsigned int vpx_mse8x16_c(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
unsigned int vpx_mse8x16_sse2(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_mse8x16)(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);

unsigned int vpx_mse8x8_c(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
unsigned int vpx_mse8x8_sse2(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_mse8x8)(const uint8_t *src_ptr, int  source_stride, const uint8_t *ref_ptr, int  recon_stride, unsigned int *sse);

void vpx_plane_add_noise_c(uint8_t *start, const int8_t *noise, int blackclamp, int whiteclamp, int width, int height, int pitch);
void vpx_plane_add_noise_sse2(uint8_t *start, const int8_t *noise, int blackclamp, int whiteclamp, int width, int height, int pitch);
RTCD_EXTERN void(*vpx_plane_add_noise)(uint8_t *start, const int8_t *noise, int blackclamp, int whiteclamp, int width, int height, int pitch);

void vpx_post_proc_down_and_across_mb_row_c(unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, int cols, unsigned char *flimits, int size);
void vpx_post_proc_down_and_across_mb_row_sse2(unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, int cols, unsigned char *flimits, int size);
RTCD_EXTERN void(*vpx_post_proc_down_and_across_mb_row)(unsigned char *src, unsigned char *dst, int src_pitch, int dst_pitch, int cols, unsigned char *flimits, int size);
#endif
void eb_vp9_quantize_b_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
void eb_vp9_quantize_b_avx(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
RTCD_EXTERN void(*eb_vp9_quantize_b)(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);

void eb_vp9_quantize_b_32x32_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
void eb_vp9_quantize_b_32x32_avx(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
RTCD_EXTERN void(*eb_vp9_quantize_b_32x32)(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);

#if 0
unsigned int vpx_sad16x16_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad16x16_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad16x16)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad16x16_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad16x16_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad16x16_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad16x16x3_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad16x16x3_ssse3(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x16x3)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

void vpx_sad16x16x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad16x16x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x16x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

void vpx_sad16x16x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad16x16x8_sse4_1(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x16x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad16x32_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad16x32_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad16x32)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad16x32_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad16x32_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad16x32_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad16x32x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad16x32x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x32x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad16x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad16x8_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad16x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad16x8_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad16x8_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad16x8_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad16x8x3_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad16x8x3_ssse3(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x8x3)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

void vpx_sad16x8x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad16x8x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x8x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

void vpx_sad16x8x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad16x8x8_sse4_1(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad16x8x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad32x16_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad32x16_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad32x16)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad32x16_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad32x16_avg_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad32x16_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad32x16x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad32x16x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad32x16x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad32x32_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad32x32_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad32x32)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad32x32_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad32x32_avg_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad32x32_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad32x32x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad32x32x4d_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad32x32x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad32x64_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad32x64_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad32x64)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad32x64_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad32x64_avg_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad32x64_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad32x64x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad32x64x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad32x64x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad4x4_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad4x4_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad4x4)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad4x4_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad4x4_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad4x4_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad4x4x3_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad4x4x3_sse3(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad4x4x3)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

void vpx_sad4x4x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad4x4x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad4x4x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

void vpx_sad4x4x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad4x4x8_sse4_1(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad4x4x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad4x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad4x8_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad4x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad4x8_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad4x8_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad4x8_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad4x8x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad4x8x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad4x8x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad64x32_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad64x32_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad64x32)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad64x32_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad64x32_avg_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad64x32_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad64x32x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad64x32x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad64x32x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad64x64_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad64x64_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad64x64)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad64x64_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad64x64_avg_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad64x64_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad64x64x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad64x64x4d_avx2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad64x64x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad8x16_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad8x16_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad8x16)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad8x16_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad8x16_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad8x16_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad8x16x3_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad8x16x3_sse3(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x16x3)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

void vpx_sad8x16x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad8x16x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x16x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

void vpx_sad8x16x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad8x16x8_sse4_1(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x16x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad8x4_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad8x4_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad8x4)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad8x4_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad8x4_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad8x4_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad8x4x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad8x4x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x4x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

unsigned int vpx_sad8x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
unsigned int vpx_sad8x8_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);
RTCD_EXTERN unsigned int(*vpx_sad8x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride);

unsigned int vpx_sad8x8_avg_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
unsigned int vpx_sad8x8_avg_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);
RTCD_EXTERN unsigned int(*vpx_sad8x8_avg)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, const uint8_t *second_pred);

void vpx_sad8x8x3_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad8x8x3_sse3(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x8x3)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

void vpx_sad8x8x4d_c(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
void vpx_sad8x8x4d_sse2(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x8x4d)(const uint8_t *src_ptr, int src_stride, const uint8_t * const ref_ptr[], int ref_stride, uint32_t *sad_array);

void vpx_sad8x8x8_c(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
void vpx_sad8x8x8_sse4_1(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);
RTCD_EXTERN void(*vpx_sad8x8x8)(const uint8_t *src_ptr, int src_stride, const uint8_t *ref_ptr, int ref_stride, uint32_t *sad_array);

int vpx_satd_c(const int16_t *coeff, int length);
int vpx_satd_avx2(const int16_t *coeff, int length);
RTCD_EXTERN int(*vpx_satd)(const int16_t *coeff, int length);

void eb_vp9_scaled_2d_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void vpx_scaled_2d_ssse3(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_scaled_2d)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_scaled_avg_2d_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_scaled_avg_2d)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);;

void eb_vp9_scaled_avg_horiz_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_scaled_avg_horiz)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_scaled_avg_vert_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_scaled_avg_vert)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_scaled_horiz_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_scaled_horiz)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_scaled_vert_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
RTCD_EXTERN void(*vpx_scaled_vert)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

uint32_t vpx_sub_pixel_avg_variance16x16_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance16x16_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance16x16)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance16x32_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance16x32_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance16x32)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance16x8_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance16x8_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance16x8)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance32x16_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance32x16_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance32x16)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance32x32_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance32x32_avx2(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance32x32)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance32x64_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance32x64_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance32x64)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance4x4_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance4x4_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance4x4)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance4x8_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance4x8_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance4x8)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance64x32_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance64x32_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance64x32)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance64x64_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance64x64_avx2(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance64x64)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance8x16_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance8x16_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance8x16)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance8x4_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance8x4_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance8x4)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_avg_variance8x8_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
uint32_t vpx_sub_pixel_avg_variance8x8_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_avg_variance8x8)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse, const uint8_t *second_pred);

uint32_t vpx_sub_pixel_variance16x16_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance16x16_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance16x16)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance16x32_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance16x32_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance16x32)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance16x8_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance16x8_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance16x8)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance32x16_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance32x16_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance32x16)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance32x32_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance32x32_avx2(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance32x32)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance32x64_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance32x64_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance32x64)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance4x4_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance4x4_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance4x4)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance4x8_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance4x8_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance4x8)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance64x32_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance64x32_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance64x32)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance64x64_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance64x64_avx2(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance64x64)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance8x16_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance8x16_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance8x16)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance8x4_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance8x4_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance8x4)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);

uint32_t vpx_sub_pixel_variance8x8_c(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
uint32_t vpx_sub_pixel_variance8x8_ssse3(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
RTCD_EXTERN uint32_t(*vpx_sub_pixel_variance8x8)(const uint8_t *src_ptr, int source_stride, int xoffset, int  yoffset, const uint8_t *ref_ptr, int ref_stride, uint32_t *sse);
#endif
void eb_vp9_subtract_block_c(int rows, int cols, int16_t *diff_ptr, ptrdiff_t diff_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, const uint8_t *pred_ptr, ptrdiff_t pred_stride);
void eb_vp9_subtract_block_sse2(int rows, int cols, int16_t *diff_ptr, ptrdiff_t diff_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, const uint8_t *pred_ptr, ptrdiff_t pred_stride);
RTCD_EXTERN void(*vpx_subtract_block)(int rows, int cols, int16_t *diff_ptr, ptrdiff_t diff_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, const uint8_t *pred_ptr, ptrdiff_t pred_stride);

#if 0
uint64_t vpx_sum_squares_2d_i16_c(const int16_t *src, int stride, int size);
uint64_t vpx_sum_squares_2d_i16_sse2(const int16_t *src, int stride, int size);
RTCD_EXTERN uint64_t(*vpx_sum_squares_2d_i16)(const int16_t *src, int stride, int size);
#endif
void eb_vp9_tm_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_tm_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_tm_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_tm_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_tm_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_tm_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_tm_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_tm_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_v_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_v_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_v_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_v_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_v_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_v_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_v_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*eb_vp9_v_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

#if 0
unsigned int vpx_variance16x16_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance16x16_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance16x16)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance16x32_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance16x32_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance16x32)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance16x8_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance16x8_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance16x8)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance32x16_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance32x16_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance32x16)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance32x32_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance32x32_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance32x32)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance32x64_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance32x64_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance32x64)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance4x4_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance4x4_sse2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance4x4)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance4x8_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance4x8_sse2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance4x8)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance64x32_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance64x32_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance64x32)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance64x64_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance64x64_avx2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance64x64)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance8x16_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance8x16_sse2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance8x16)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance8x4_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance8x4_sse2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance8x4)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

unsigned int vpx_variance8x8_c(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
unsigned int vpx_variance8x8_sse2(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);
RTCD_EXTERN unsigned int(*vpx_variance8x8)(const uint8_t *src_ptr, int source_stride, const uint8_t *ref_ptr, int ref_stride, unsigned int *sse);

void eb_vp9_ve_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void(*vpx_ve_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

int vpx_vector_var_c(const int16_t *ref, const int16_t *src, const int bwl);
int vpx_vector_var_sse2(const int16_t *ref, const int16_t *src, const int bwl);
RTCD_EXTERN int(*vpx_vector_var)(const int16_t *ref, const int16_t *src, const int bwl);

void vpx_dsp_rtcd(void);
#endif

#ifdef RTCD_C

static void setup_rtcd_internal(uint32_t asm_type)
{
#if 1

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
    vpx_avg_4x4 = vpx_avg_4x4_c;
    if (flags & HAS_SSE2) vpx_avg_4x4 = vpx_avg_4x4_sse2;
    vpx_avg_8x8 = vpx_avg_8x8_c;
    if (flags & HAS_SSE2) vpx_avg_8x8 = vpx_avg_8x8_sse2;
    vpx_comp_avg_pred = vpx_comp_avg_pred_c;
    if (flags & HAS_SSE2) vpx_comp_avg_pred = vpx_comp_avg_pred_sse2;
#endif
    eb_vp9_convolve8 = eb_vp9_convolve8_c;
    if (flags & HAS_AVX2) eb_vp9_convolve8 = eb_vp9_convolve8_avx2;
    eb_vp9_convolve8_avg = eb_vp9_convolve8_avg_c;
    if (flags & HAS_AVX2) eb_vp9_convolve8_avg = eb_vp9_convolve8_avg_avx2;
    eb_vp9_convolve8_avg_horiz = eb_vp9_convolve8_avg_horiz_c;
    if (flags & HAS_AVX2) eb_vp9_convolve8_avg_horiz = eb_vp9_convolve8_avg_horiz_avx2;
    eb_vp9_convolve8_avg_vert = eb_vp9_convolve8_avg_vert_c;
    if (flags & HAS_AVX2) eb_vp9_convolve8_avg_vert = eb_vp9_convolve8_avg_vert_avx2;
    eb_vp9_convolve8_horiz = eb_vp9_convolve8_horiz_c;
    if (flags & HAS_AVX2) eb_vp9_convolve8_horiz = eb_vp9_convolve8_horiz_avx2;
    eb_vp9_convolve8_vert = eb_vp9_convolve8_vert_c;
    if (flags & HAS_AVX2) eb_vp9_convolve8_vert = eb_vp9_convolve8_vert_avx2;
    vpx_convolve_avg = eb_vp9_convolve_avg_c;
    if (flags & HAS_AVX2) vpx_convolve_avg = vpx_convolve_avg_avx2;
    vpx_convolve_copy = eb_vp9_convolve_copy_c;
    if (flags & HAS_AVX2) vpx_convolve_copy = vpx_convolve_copy_avx2;
    eb_vp9_d117_predictor_4x4 = eb_vp9_d117_predictor_4x4_c;
    if (flags & HAS_SSSE3) eb_vp9_d117_predictor_4x4 = eb_vp9_d117_predictor_4x4_ssse3;
    eb_vp9_d117_predictor_8x8 = eb_vp9_d117_predictor_8x8_c;
    if (flags & HAS_SSSE3) eb_vp9_d117_predictor_8x8 = eb_vp9_d117_predictor_8x8_ssse3;
    eb_vp9_d117_predictor_16x16 = eb_vp9_d117_predictor_16x16_c;
    if (flags & HAS_SSSE3) eb_vp9_d117_predictor_16x16 = eb_vp9_d117_predictor_16x16_ssse3;
    eb_vp9_d117_predictor_32x32 = eb_vp9_d117_predictor_32x32_c;
    if (flags & HAS_AVX2) eb_vp9_d117_predictor_32x32 = eb_vp9_d117_predictor_32x32_avx2;
    eb_vp9_d135_predictor_4x4 = eb_vp9_d135_predictor_4x4_c;
    if (flags & HAS_SSSE3) eb_vp9_d135_predictor_4x4 = eb_vp9_d135_predictor_4x4_ssse3;
    eb_vp9_d135_predictor_8x8 = eb_vp9_d135_predictor_8x8_c;
    if (flags & HAS_SSSE3) eb_vp9_d135_predictor_8x8 = eb_vp9_d135_predictor_8x8_ssse3;
    eb_vp9_d135_predictor_16x16 = eb_vp9_d135_predictor_16x16_c;
    if (flags & HAS_SSSE3) eb_vp9_d135_predictor_16x16 = eb_vp9_d135_predictor_16x16_ssse3;
    eb_vp9_d135_predictor_32x32 = eb_vp9_d135_predictor_32x32_c;
    if (flags & HAS_AVX2) eb_vp9_d135_predictor_32x32 = eb_vp9_d135_predictor_32x32_avx2;
    eb_vp9_d153_predictor_16x16 = eb_vp9_d153_predictor_16x16_c;
    if (flags & HAS_SSSE3) eb_vp9_d153_predictor_16x16 = eb_vp9_d153_predictor_16x16_ssse3;
    eb_vp9_d153_predictor_32x32 = eb_vp9_d153_predictor_32x32_c;
    if (flags & HAS_SSSE3) eb_vp9_d153_predictor_32x32 = eb_vp9_d153_predictor_32x32_ssse3;
    eb_vp9_d153_predictor_4x4 = eb_vp9_d153_predictor_4x4_c;
    if (flags & HAS_SSSE3) eb_vp9_d153_predictor_4x4 = eb_vp9_d153_predictor_4x4_ssse3;
    eb_vp9_d153_predictor_8x8 = eb_vp9_d153_predictor_8x8_c;
    if (flags & HAS_SSSE3) eb_vp9_d153_predictor_8x8 = eb_vp9_d153_predictor_8x8_ssse3;
    eb_vp9_d207_predictor_16x16 = eb_vp9_d207_predictor_16x16_c;
    if (flags & HAS_SSSE3) eb_vp9_d207_predictor_16x16 = eb_vp9_d207_predictor_16x16_ssse3;
    eb_vp9_d207_predictor_32x32 = eb_vp9_d207_predictor_32x32_c;
    if (flags & HAS_SSSE3) eb_vp9_d207_predictor_32x32 = eb_vp9_d207_predictor_32x32_ssse3;
    eb_vp9_d207_predictor_4x4 = eb_vp9_d207_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_d207_predictor_4x4 = eb_vp9_d207_predictor_4x4_sse2;
    eb_vp9_d207_predictor_8x8 = eb_vp9_d207_predictor_8x8_c;
    if (flags & HAS_SSSE3) eb_vp9_d207_predictor_8x8 = eb_vp9_d207_predictor_8x8_ssse3;
    eb_vp9_d45_predictor_16x16 = eb_vp9_d45_predictor_16x16_c;
    if (flags & HAS_SSSE3) eb_vp9_d45_predictor_16x16 = eb_vp9_d45_predictor_16x16_ssse3;
    eb_vp9_d45_predictor_32x32 = eb_vp9_d45_predictor_32x32_c;
    if (flags & HAS_SSSE3) eb_vp9_d45_predictor_32x32 = eb_vp9_d45_predictor_32x32_ssse3;
    eb_vp9_d45_predictor_4x4 = eb_vp9_d45_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_d45_predictor_4x4 = eb_vp9_d45_predictor_4x4_sse2;
    eb_vp9_d45_predictor_8x8 = eb_vp9_d45_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_d45_predictor_8x8 = eb_vp9_d45_predictor_8x8_sse2;
    eb_vp9_d45e_predictor_4x4 = eb_vp9_d45e_predictor_4x4_c;
    eb_vp9_d63_predictor_16x16 = eb_vp9_d63_predictor_16x16_c;
    if (flags & HAS_SSSE3) eb_vp9_d63_predictor_16x16 = eb_vp9_d63_predictor_16x16_ssse3;
    eb_vp9_d63_predictor_32x32 = eb_vp9_d63_predictor_32x32_c;
    if (flags & HAS_SSSE3) eb_vp9_d63_predictor_32x32 = eb_vp9_d63_predictor_32x32_ssse3;
    eb_vp9_d63_predictor_4x4 = eb_vp9_d63_predictor_4x4_c;
    if (flags & HAS_SSSE3) eb_vp9_d63_predictor_4x4 = eb_vp9_d63_predictor_4x4_ssse3;
    eb_vp9_d63_predictor_8x8 = eb_vp9_d63_predictor_8x8_c;
    if (flags & HAS_SSSE3) eb_vp9_d63_predictor_8x8 = eb_vp9_d63_predictor_8x8_ssse3;
    eb_vp9_d63e_predictor_4x4 = eb_vp9_d63e_predictor_4x4_c;
    eb_vp9_dc_128_predictor_16x16 = eb_vp9_dc_128_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_dc_128_predictor_16x16 = eb_vp9_dc_128_predictor_16x16_sse2;
    eb_vp9_dc_128_predictor_32x32 = eb_vp9_dc_128_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_dc_128_predictor_32x32 = eb_vp9_dc_128_predictor_32x32_sse2;
    eb_vp9_dc_128_predictor_4x4 = eb_vp9_dc_128_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_dc_128_predictor_4x4 = eb_vp9_dc_128_predictor_4x4_sse2;
    eb_vp9_dc_128_predictor_8x8 = eb_vp9_dc_128_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_dc_128_predictor_8x8 = eb_vp9_dc_128_predictor_8x8_sse2;
    eb_vp9_dc_left_predictor_16x16 = eb_vp9_dc_left_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_dc_left_predictor_16x16 = eb_vp9_dc_left_predictor_16x16_sse2;
    eb_vp9_dc_left_predictor_32x32 = eb_vp9_dc_left_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_dc_left_predictor_32x32 = eb_vp9_dc_left_predictor_32x32_sse2;
    eb_vp9_dc_left_predictor_4x4 = eb_vp9_dc_left_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_dc_left_predictor_4x4 = eb_vp9_dc_left_predictor_4x4_sse2;
    eb_vp9_dc_left_predictor_8x8 = eb_vp9_dc_left_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_dc_left_predictor_8x8 = eb_vp9_dc_left_predictor_8x8_sse2;
    eb_vp9_dc_predictor_16x16 = eb_vp9_dc_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_dc_predictor_16x16 = eb_vp9_dc_predictor_16x16_sse2;
    eb_vp9_dc_predictor_32x32 = eb_vp9_dc_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_dc_predictor_32x32 = eb_vp9_dc_predictor_32x32_sse2;
    eb_vp9_dc_predictor_4x4 = eb_vp9_dc_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_dc_predictor_4x4 = eb_vp9_dc_predictor_4x4_sse2;
    eb_vp9_dc_predictor_8x8 = eb_vp9_dc_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_dc_predictor_8x8 = eb_vp9_dc_predictor_8x8_sse2;
    eb_vp9_dc_top_predictor_16x16 = eb_vp9_dc_top_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_dc_top_predictor_16x16 = eb_vp9_dc_top_predictor_16x16_sse2;
    eb_vp9_dc_top_predictor_32x32 = eb_vp9_dc_top_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_dc_top_predictor_32x32 = eb_vp9_dc_top_predictor_32x32_sse2;
    eb_vp9_dc_top_predictor_4x4 = eb_vp9_dc_top_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_dc_top_predictor_4x4 = eb_vp9_dc_top_predictor_4x4_sse2;
    eb_vp9_dc_top_predictor_8x8 = eb_vp9_dc_top_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_dc_top_predictor_8x8 = eb_vp9_dc_top_predictor_8x8_sse2;

    vpx_fdct16x16 = eb_vp9_fdct16x16_c;
    if (flags & HAS_AVX2) vpx_fdct16x16 = vpx_fdct16x16_avx2;
#if 0
    vpx_fdct16x16_1 = eb_vp9_fdct16x16_1_c;
    if (flags & HAS_SSE2) vpx_fdct16x16_1 = vpx_fdct16x16_1_sse2;
    //if (flags & HAS_AVX2) vpx_fdct16x16 = vpx_fdct16x16_avx2;
#endif
    eb_vp9_fdct32x32 = eb_vp9_fdct32x32_c;
    if (flags & HAS_AVX2) eb_vp9_fdct32x32 = eb_vp9_fdct32x32_avx2;
    vpx_partial_fdct32x32 = vpx_partial_fdct32x32_c;
    if (flags & HAS_AVX2) vpx_partial_fdct32x32 = vpx_partial_fdct32x32_avx2;
#if 0
    eb_vp9_fdct32x32_1 = eb_vp9_fdct32x32_1_c;
    if (flags & HAS_SSE2) eb_vp9_fdct32x32_1 = eb_vp9_fdct32x32_1_sse2;
    eb_vp9_fdct32x32_rd = eb_vp9_fdct32x32_rd_c;
    if (flags & HAS_AVX2) eb_vp9_fdct32x32_rd = eb_vp9_fdct32x32_rd_avx2;
#endif
    vpx_fdct4x4 = eb_vp9_fdct4x4_c;
    if (flags & HAS_SSE2) vpx_fdct4x4 = eb_vp9_fdct4x4_sse2;
    vpx_fdct8x8 = eb_vp9_fdct8x8_c;
    if (flags & HAS_AVX2) vpx_fdct8x8 = vpx_fdct8x8_avx2;
#if 0
    vpx_fdct4x4_1 = eb_vp9_fdct4x4_1_c;
    if (flags & HAS_SSE2) vpx_fdct4x4_1 = vpx_fdct4x4_1_sse2;
    vpx_fdct8x8_1 = eb_vp9_fdct8x8_1_c;
    if (flags & HAS_SSE2) vpx_fdct8x8_1 = vpx_fdct8x8_1_sse2;
    vpx_get16x16var = vpx_get16x16var_c;
    if (flags & HAS_AVX2) vpx_get16x16var = vpx_get16x16var_avx2;
    vpx_get4x4sse_cs = vpx_get4x4sse_cs_c;
    vpx_get8x8var = vpx_get8x8var_c;
    if (flags & HAS_SSE2) vpx_get8x8var = vpx_get8x8var_sse2;
    vpx_get_mb_ss = vpx_get_mb_ss_c;
    if (flags & HAS_SSE2) vpx_get_mb_ss = vpx_get_mb_ss_sse2;
#endif
    eb_vp9_h_predictor_16x16 = eb_vp9_h_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_h_predictor_16x16 = eb_vp9_h_predictor_16x16_sse2;
    eb_vp9_h_predictor_32x32 = eb_vp9_h_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_h_predictor_32x32 = eb_vp9_h_predictor_32x32_sse2;
    eb_vp9_h_predictor_4x4 = eb_vp9_h_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_h_predictor_4x4 = eb_vp9_h_predictor_4x4_sse2;
    eb_vp9_h_predictor_8x8 = eb_vp9_h_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_h_predictor_8x8 = eb_vp9_h_predictor_8x8_sse2;
#if 0
    vpx_hadamard_16x16 = vpx_hadamard_16x16_c;
    if (flags & HAS_AVX2) vpx_hadamard_16x16 = vpx_hadamard_16x16_avx2;
    vpx_hadamard_32x32 = vpx_hadamard_32x32_c;
    if (flags & HAS_AVX2) vpx_hadamard_32x32 = vpx_hadamard_32x32_avx2;
    vpx_hadamard_8x8 = vpx_hadamard_8x8_c;
    if (flags & HAS_SSE2) vpx_hadamard_8x8 = vpx_hadamard_8x8_sse2;
    vpx_he_predictor_4x4 = eb_vp9_he_predictor_4x4_c;
#endif
    vpx_idct16x16_10_add = eb_vp9_idct16x16_10_add_c;
    if (flags & HAS_SSE2) vpx_idct16x16_10_add = eb_vp9_idct16x16_10_add_sse2;
    vpx_idct16x16_1_add = eb_vp9_idct16x16_1_add_c;
    if (flags & HAS_SSE2) vpx_idct16x16_1_add = eb_vp9_idct16x16_1_add_sse2;
    vpx_idct16x16_256_add = eb_vp9_idct16x16_256_add_c;
    if (flags & HAS_SSE2) vpx_idct16x16_256_add = eb_vp9_idct16x16_256_add_sse2;
    vpx_idct16x16_38_add = eb_vp9_idct16x16_38_add_c;
    if (flags & HAS_SSE2) vpx_idct16x16_38_add = eb_vp9_idct16x16_38_add_sse2;
    vpx_idct32x32_1024_add = eb_vp9_idct32x32_1024_add_c;
    if (flags & HAS_AVX2) vpx_idct32x32_1024_add = vpx_idct32x32_1024_add_avx2;
    eb_vp9_idct32x32_135_add = eb_vp9_idct32x32_135_add_c;
    if (flags & HAS_AVX2) eb_vp9_idct32x32_135_add = eb_vp9_idct32x32_135_add_avx2;
    vpx_idct32x32_1_add = eb_vp9_idct32x32_1_add_c;
    if (flags & HAS_AVX2) vpx_idct32x32_1_add = vpx_idct32x32_1_add_avx2;
    eb_vp9_idct32x32_34_add = eb_vp9_idct32x32_34_add_c;
    if (flags & HAS_AVX2) eb_vp9_idct32x32_34_add = eb_vp9_idct32x32_34_add_avx2;
    vpx_idct4x4_16_add = eb_vp9_idct4x4_16_add_c;
    if (flags & HAS_SSE2) vpx_idct4x4_16_add = eb_vp9_idct4x4_16_add_sse2;
    vpx_idct4x4_1_add = eb_vp9_idct4x4_1_add_c;
    if (flags & HAS_SSE2) vpx_idct4x4_1_add = eb_vp9_idct4x4_1_add_sse2;
    eb_vp9_idct8x8_12_add = eb_vp9_idct8x8_12_add_c;
    if (flags & HAS_SSSE3) eb_vp9_idct8x8_12_add = eb_vp9_idct8x8_12_add_ssse3;
    vpx_idct8x8_1_add = eb_vp9_idct8x8_1_add_c;
    if (flags & HAS_SSE2) vpx_idct8x8_1_add = eb_vp9_idct8x8_1_add_sse2;
    vpx_idct8x8_64_add = eb_vp9_idct8x8_64_add_c;
    if (flags & HAS_SSE2) vpx_idct8x8_64_add = eb_vp9_idct8x8_64_add_sse2;
    eb_vp9_lpf_horizontal_16 = eb_vp9_lpf_horizontal_16_c;
    if (flags & HAS_AVX2) eb_vp9_lpf_horizontal_16 = eb_vp9_lpf_horizontal_16_avx2;
    eb_vp9_lpf_horizontal_16_dual = eb_vp9_lpf_horizontal_16_dual_c;
    if (flags & HAS_AVX2) eb_vp9_lpf_horizontal_16_dual = eb_vp9_lpf_horizontal_16_dual_avx2;
    vpx_lpf_horizontal_4 = eb_vp9_lpf_horizontal_4_c;
    if (flags & HAS_SSE2) vpx_lpf_horizontal_4 = eb_vp9_lpf_horizontal_4_sse2;
    vpx_lpf_horizontal_4_dual = eb_vp9_lpf_horizontal_4_dual_c;
    if (flags & HAS_SSE2) vpx_lpf_horizontal_4_dual = eb_vp9_lpf_horizontal_4_dual_sse2;
    vpx_lpf_horizontal_8 = eb_vp9_lpf_horizontal_8_c;
    if (flags & HAS_SSE2) vpx_lpf_horizontal_8 = eb_vp9_lpf_horizontal_8_sse2;
    vpx_lpf_horizontal_8_dual = eb_vp9_lpf_horizontal_8_dual_c;
    if (flags & HAS_SSE2) vpx_lpf_horizontal_8_dual = eb_vp9_lpf_horizontal_8_dual_sse2;
    vpx_lpf_vertical_16 = eb_vp9_lpf_vertical_16_c;
    if (flags & HAS_SSE2) vpx_lpf_vertical_16 = eb_vp9_lpf_vertical_16_sse2;
    vpx_lpf_vertical_16_dual = eb_vp9_lpf_vertical_16_dual_c;
    if (flags & HAS_AVX2) vpx_lpf_vertical_16_dual = vpx_lpf_vertical_16_dual_avx2;

    vpx_lpf_vertical_4 = eb_vp9_lpf_vertical_4_c;
    if (flags & HAS_SSE2) vpx_lpf_vertical_4 = eb_vp9_lpf_vertical_4_sse2;
    vpx_lpf_vertical_4_dual = eb_vp9_lpf_vertical_4_dual_c;
    if (flags & HAS_SSE2) vpx_lpf_vertical_4_dual = eb_vp9_lpf_vertical_4_dual_sse2;
    vpx_lpf_vertical_8 = eb_vp9_lpf_vertical_8_c;
    if (flags & HAS_SSE2) vpx_lpf_vertical_8 = eb_vp9_lpf_vertical_8_sse2;
    vpx_lpf_vertical_8_dual = eb_vp9_lpf_vertical_8_dual_c;
    if (flags & HAS_SSE2) vpx_lpf_vertical_8_dual = eb_vp9_lpf_vertical_8_dual_sse2;
#if 0
    vpx_int_pro_col = vpx_int_pro_col_c;
    if (flags & HAS_SSE2) vpx_int_pro_col = vpx_int_pro_col_sse2;
    vpx_int_pro_row = vpx_int_pro_row_c;
    if (flags & HAS_SSE2) vpx_int_pro_row = vpx_int_pro_row_sse2;
    vpx_iwht4x4_16_add = eb_vp9_iwht4x4_16_add_c;
    if (flags & HAS_SSE2) vpx_iwht4x4_16_add = vpx_iwht4x4_16_add_sse2;
    vpx_iwht4x4_1_add = eb_vp9_iwht4x4_1_add_c;
    vpx_mbpost_proc_across_ip = vpx_mbpost_proc_across_ip_c;
    if (flags & HAS_SSE2) vpx_mbpost_proc_across_ip = vpx_mbpost_proc_across_ip_sse2;
    vpx_mbpost_proc_down = vpx_mbpost_proc_down_c;
    if (flags & HAS_SSE2) vpx_mbpost_proc_down = vpx_mbpost_proc_down_sse2;
    vpx_minmax_8x8 = vpx_minmax_8x8_c;
    if (flags & HAS_SSE2) vpx_minmax_8x8 = vpx_minmax_8x8_sse2;
    eb_vp9_mse16x16 = eb_vp9_mse16x16_c;
    if (flags & HAS_AVX2) eb_vp9_mse16x16 = eb_vp9_mse16x16_avx2;
    eb_vp9_mse16x8 = eb_vp9_mse16x8_c;
    if (flags & HAS_AVX2) eb_vp9_mse16x8 = eb_vp9_mse16x8_avx2;
    vpx_mse8x16 = vpx_mse8x16_c;
    if (flags & HAS_SSE2) vpx_mse8x16 = vpx_mse8x16_sse2;
    vpx_mse8x8 = vpx_mse8x8_c;
    if (flags & HAS_SSE2) vpx_mse8x8 = vpx_mse8x8_sse2;
    vpx_plane_add_noise = vpx_plane_add_noise_c;
    if (flags & HAS_SSE2) vpx_plane_add_noise = vpx_plane_add_noise_sse2;
    vpx_post_proc_down_and_across_mb_row = vpx_post_proc_down_and_across_mb_row_c;
    if (flags & HAS_SSE2) vpx_post_proc_down_and_across_mb_row = vpx_post_proc_down_and_across_mb_row_sse2;
#endif
    eb_vp9_quantize_b = eb_vp9_quantize_b_c;
    if (flags & HAS_AVX) eb_vp9_quantize_b = eb_vp9_quantize_b_avx;
    eb_vp9_quantize_b_32x32 = eb_vp9_quantize_b_32x32_c;
    if (flags & HAS_AVX) eb_vp9_quantize_b_32x32 = eb_vp9_quantize_b_32x32_avx;
#if 0
    vpx_sad16x16 = vpx_sad16x16_c;
    if (flags & HAS_SSE2) vpx_sad16x16 = vpx_sad16x16_sse2;
    vpx_sad16x16_avg = vpx_sad16x16_avg_c;
    if (flags & HAS_SSE2) vpx_sad16x16_avg = vpx_sad16x16_avg_sse2;
    vpx_sad16x16x3 = vpx_sad16x16x3_c;
    if (flags & HAS_SSSE3) vpx_sad16x16x3 = vpx_sad16x16x3_ssse3;
    vpx_sad16x16x4d = vpx_sad16x16x4d_c;
    if (flags & HAS_SSE2) vpx_sad16x16x4d = vpx_sad16x16x4d_sse2;
    vpx_sad16x16x8 = vpx_sad16x16x8_c;
    if (flags & HAS_SSE4_1) vpx_sad16x16x8 = vpx_sad16x16x8_sse4_1;
    vpx_sad16x32 = vpx_sad16x32_c;
    if (flags & HAS_SSE2) vpx_sad16x32 = vpx_sad16x32_sse2;
    vpx_sad16x32_avg = vpx_sad16x32_avg_c;
    if (flags & HAS_SSE2) vpx_sad16x32_avg = vpx_sad16x32_avg_sse2;
    vpx_sad16x32x4d = vpx_sad16x32x4d_c;
    if (flags & HAS_SSE2) vpx_sad16x32x4d = vpx_sad16x32x4d_sse2;
    vpx_sad16x8 = vpx_sad16x8_c;
    if (flags & HAS_SSE2) vpx_sad16x8 = vpx_sad16x8_sse2;
    vpx_sad16x8_avg = vpx_sad16x8_avg_c;
    if (flags & HAS_SSE2) vpx_sad16x8_avg = vpx_sad16x8_avg_sse2;
    vpx_sad16x8x3 = vpx_sad16x8x3_c;
    if (flags & HAS_SSSE3) vpx_sad16x8x3 = vpx_sad16x8x3_ssse3;
    vpx_sad16x8x4d = vpx_sad16x8x4d_c;
    if (flags & HAS_SSE2) vpx_sad16x8x4d = vpx_sad16x8x4d_sse2;
    vpx_sad16x8x8 = vpx_sad16x8x8_c;
    if (flags & HAS_SSE4_1) vpx_sad16x8x8 = vpx_sad16x8x8_sse4_1;
    vpx_sad32x16 = vpx_sad32x16_c;
    if (flags & HAS_AVX2) vpx_sad32x16 = vpx_sad32x16_avx2;
    vpx_sad32x16_avg = vpx_sad32x16_avg_c;
    if (flags & HAS_AVX2) vpx_sad32x16_avg = vpx_sad32x16_avg_avx2;
    vpx_sad32x16x4d = vpx_sad32x16x4d_c;
    if (flags & HAS_SSE2) vpx_sad32x16x4d = vpx_sad32x16x4d_sse2;
    vpx_sad32x32 = vpx_sad32x32_c;
    if (flags & HAS_AVX2) vpx_sad32x32 = vpx_sad32x32_avx2;
    vpx_sad32x32_avg = vpx_sad32x32_avg_c;
    if (flags & HAS_AVX2) vpx_sad32x32_avg = vpx_sad32x32_avg_avx2;
    vpx_sad32x32x4d = vpx_sad32x32x4d_c;
    if (flags & HAS_AVX2) vpx_sad32x32x4d = vpx_sad32x32x4d_avx2;
    vpx_sad32x64 = vpx_sad32x64_c;
    if (flags & HAS_AVX2) vpx_sad32x64 = vpx_sad32x64_avx2;
    vpx_sad32x64_avg = vpx_sad32x64_avg_c;
    if (flags & HAS_AVX2) vpx_sad32x64_avg = vpx_sad32x64_avg_avx2;
    vpx_sad32x64x4d = vpx_sad32x64x4d_c;
    if (flags & HAS_SSE2) vpx_sad32x64x4d = vpx_sad32x64x4d_sse2;
    vpx_sad4x4 = vpx_sad4x4_c;
    if (flags & HAS_SSE2) vpx_sad4x4 = vpx_sad4x4_sse2;
    vpx_sad4x4_avg = vpx_sad4x4_avg_c;
    if (flags & HAS_SSE2) vpx_sad4x4_avg = vpx_sad4x4_avg_sse2;
    vpx_sad4x4x3 = vpx_sad4x4x3_c;
    if (flags & HAS_SSE3) vpx_sad4x4x3 = vpx_sad4x4x3_sse3;
    vpx_sad4x4x4d = vpx_sad4x4x4d_c;
    if (flags & HAS_SSE2) vpx_sad4x4x4d = vpx_sad4x4x4d_sse2;
    vpx_sad4x4x8 = vpx_sad4x4x8_c;
    if (flags & HAS_SSE4_1) vpx_sad4x4x8 = vpx_sad4x4x8_sse4_1;
    vpx_sad4x8 = vpx_sad4x8_c;
    if (flags & HAS_SSE2) vpx_sad4x8 = vpx_sad4x8_sse2;
    vpx_sad4x8_avg = vpx_sad4x8_avg_c;
    if (flags & HAS_SSE2) vpx_sad4x8_avg = vpx_sad4x8_avg_sse2;
    vpx_sad4x8x4d = vpx_sad4x8x4d_c;
    if (flags & HAS_SSE2) vpx_sad4x8x4d = vpx_sad4x8x4d_sse2;
    vpx_sad64x32 = vpx_sad64x32_c;
    if (flags & HAS_AVX2) vpx_sad64x32 = vpx_sad64x32_avx2;
    vpx_sad64x32_avg = vpx_sad64x32_avg_c;
    if (flags & HAS_AVX2) vpx_sad64x32_avg = vpx_sad64x32_avg_avx2;
    vpx_sad64x32x4d = vpx_sad64x32x4d_c;
    if (flags & HAS_SSE2) vpx_sad64x32x4d = vpx_sad64x32x4d_sse2;
    vpx_sad64x64 = vpx_sad64x64_c;
    if (flags & HAS_AVX2) vpx_sad64x64 = vpx_sad64x64_avx2;
    vpx_sad64x64_avg = vpx_sad64x64_avg_c;
    if (flags & HAS_AVX2) vpx_sad64x64_avg = vpx_sad64x64_avg_avx2;
    vpx_sad64x64x4d = vpx_sad64x64x4d_c;
    if (flags & HAS_AVX2) vpx_sad64x64x4d = vpx_sad64x64x4d_avx2;
    vpx_sad8x16 = vpx_sad8x16_c;
    if (flags & HAS_SSE2) vpx_sad8x16 = vpx_sad8x16_sse2;
    vpx_sad8x16_avg = vpx_sad8x16_avg_c;
    if (flags & HAS_SSE2) vpx_sad8x16_avg = vpx_sad8x16_avg_sse2;
    vpx_sad8x16x3 = vpx_sad8x16x3_c;
    if (flags & HAS_SSE3) vpx_sad8x16x3 = vpx_sad8x16x3_sse3;
    vpx_sad8x16x4d = vpx_sad8x16x4d_c;
    if (flags & HAS_SSE2) vpx_sad8x16x4d = vpx_sad8x16x4d_sse2;
    vpx_sad8x16x8 = vpx_sad8x16x8_c;
    if (flags & HAS_SSE4_1) vpx_sad8x16x8 = vpx_sad8x16x8_sse4_1;
    vpx_sad8x4 = vpx_sad8x4_c;
    if (flags & HAS_SSE2) vpx_sad8x4 = vpx_sad8x4_sse2;
    vpx_sad8x4_avg = vpx_sad8x4_avg_c;
    if (flags & HAS_SSE2) vpx_sad8x4_avg = vpx_sad8x4_avg_sse2;
    vpx_sad8x4x4d = vpx_sad8x4x4d_c;
    if (flags & HAS_SSE2) vpx_sad8x4x4d = vpx_sad8x4x4d_sse2;
    vpx_sad8x8 = vpx_sad8x8_c;
    if (flags & HAS_SSE2) vpx_sad8x8 = vpx_sad8x8_sse2;
    vpx_sad8x8_avg = vpx_sad8x8_avg_c;
    if (flags & HAS_SSE2) vpx_sad8x8_avg = vpx_sad8x8_avg_sse2;
    vpx_sad8x8x3 = vpx_sad8x8x3_c;
    if (flags & HAS_SSE3) vpx_sad8x8x3 = vpx_sad8x8x3_sse3;
    vpx_sad8x8x4d = vpx_sad8x8x4d_c;
    if (flags & HAS_SSE2) vpx_sad8x8x4d = vpx_sad8x8x4d_sse2;
    vpx_sad8x8x8 = vpx_sad8x8x8_c;
    if (flags & HAS_SSE4_1) vpx_sad8x8x8 = vpx_sad8x8x8_sse4_1;
    vpx_satd = vpx_satd_c;
    if (flags & HAS_AVX2) vpx_satd = vpx_satd_avx2;
    vpx_scaled_2d = eb_vp9_scaled_2d_c;
    if (flags & HAS_SSSE3) vpx_scaled_2d = vpx_scaled_2d_ssse3;
    vpx_scaled_avg_2d = eb_vp9_scaled_avg_2d_c;
    vpx_scaled_avg_horiz = eb_vp9_scaled_avg_horiz_c;
    vpx_scaled_avg_vert = eb_vp9_scaled_avg_vert_c;
    vpx_scaled_horiz = eb_vp9_scaled_horiz_c;
    vpx_scaled_vert = eb_vp9_scaled_vert_c
    vpx_sub_pixel_avg_variance16x16 = vpx_sub_pixel_avg_variance16x16_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance16x16 = vpx_sub_pixel_avg_variance16x16_ssse3;
    vpx_sub_pixel_avg_variance16x32 = vpx_sub_pixel_avg_variance16x32_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance16x32 = vpx_sub_pixel_avg_variance16x32_ssse3;
    vpx_sub_pixel_avg_variance16x8 = vpx_sub_pixel_avg_variance16x8_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance16x8 = vpx_sub_pixel_avg_variance16x8_ssse3;
    vpx_sub_pixel_avg_variance32x16 = vpx_sub_pixel_avg_variance32x16_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance32x16 = vpx_sub_pixel_avg_variance32x16_ssse3;
    vpx_sub_pixel_avg_variance32x32 = vpx_sub_pixel_avg_variance32x32_c;
    if (flags & HAS_AVX2) vpx_sub_pixel_avg_variance32x32 = vpx_sub_pixel_avg_variance32x32_avx2;
    vpx_sub_pixel_avg_variance32x64 = vpx_sub_pixel_avg_variance32x64_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance32x64 = vpx_sub_pixel_avg_variance32x64_ssse3;
    vpx_sub_pixel_avg_variance4x4 = vpx_sub_pixel_avg_variance4x4_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance4x4 = vpx_sub_pixel_avg_variance4x4_ssse3;
    vpx_sub_pixel_avg_variance4x8 = vpx_sub_pixel_avg_variance4x8_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance4x8 = vpx_sub_pixel_avg_variance4x8_ssse3;
    vpx_sub_pixel_avg_variance64x32 = vpx_sub_pixel_avg_variance64x32_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance64x32 = vpx_sub_pixel_avg_variance64x32_ssse3;
    vpx_sub_pixel_avg_variance64x64 = vpx_sub_pixel_avg_variance64x64_c;
    if (flags & HAS_AVX2) vpx_sub_pixel_avg_variance64x64 = vpx_sub_pixel_avg_variance64x64_avx2;
    vpx_sub_pixel_avg_variance8x16 = vpx_sub_pixel_avg_variance8x16_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance8x16 = vpx_sub_pixel_avg_variance8x16_ssse3;
    vpx_sub_pixel_avg_variance8x4 = vpx_sub_pixel_avg_variance8x4_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance8x4 = vpx_sub_pixel_avg_variance8x4_ssse3;
    vpx_sub_pixel_avg_variance8x8 = vpx_sub_pixel_avg_variance8x8_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_avg_variance8x8 = vpx_sub_pixel_avg_variance8x8_ssse3;
    vpx_sub_pixel_variance16x16 = vpx_sub_pixel_variance16x16_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance16x16 = vpx_sub_pixel_variance16x16_ssse3;
    vpx_sub_pixel_variance16x32 = vpx_sub_pixel_variance16x32_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance16x32 = vpx_sub_pixel_variance16x32_ssse3;
    vpx_sub_pixel_variance16x8 = vpx_sub_pixel_variance16x8_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance16x8 = vpx_sub_pixel_variance16x8_ssse3;
    vpx_sub_pixel_variance32x16 = vpx_sub_pixel_variance32x16_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance32x16 = vpx_sub_pixel_variance32x16_ssse3;
    vpx_sub_pixel_variance32x32 = vpx_sub_pixel_variance32x32_c;
    if (flags & HAS_AVX2) vpx_sub_pixel_variance32x32 = vpx_sub_pixel_variance32x32_avx2;
    vpx_sub_pixel_variance32x64 = vpx_sub_pixel_variance32x64_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance32x64 = vpx_sub_pixel_variance32x64_ssse3;
    vpx_sub_pixel_variance4x4 = vpx_sub_pixel_variance4x4_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance4x4 = vpx_sub_pixel_variance4x4_ssse3;
    vpx_sub_pixel_variance4x8 = vpx_sub_pixel_variance4x8_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance4x8 = vpx_sub_pixel_variance4x8_ssse3;
    vpx_sub_pixel_variance64x32 = vpx_sub_pixel_variance64x32_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance64x32 = vpx_sub_pixel_variance64x32_ssse3;
    vpx_sub_pixel_variance64x64 = vpx_sub_pixel_variance64x64_c;
    if (flags & HAS_AVX2) vpx_sub_pixel_variance64x64 = vpx_sub_pixel_variance64x64_avx2;
    vpx_sub_pixel_variance8x16 = vpx_sub_pixel_variance8x16_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance8x16 = vpx_sub_pixel_variance8x16_ssse3;
    vpx_sub_pixel_variance8x4 = vpx_sub_pixel_variance8x4_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance8x4 = vpx_sub_pixel_variance8x4_ssse3;
    vpx_sub_pixel_variance8x8 = vpx_sub_pixel_variance8x8_c;
    if (flags & HAS_SSSE3) vpx_sub_pixel_variance8x8 = vpx_sub_pixel_variance8x8_ssse3;
#endif
    vpx_subtract_block = eb_vp9_subtract_block_c;
    if (flags & HAS_SSE2) vpx_subtract_block = eb_vp9_subtract_block_sse2;
#if 0
    vpx_sum_squares_2d_i16 = vpx_sum_squares_2d_i16_c;
    if (flags & HAS_SSE2) vpx_sum_squares_2d_i16 = vpx_sum_squares_2d_i16_sse2;
#endif
    eb_vp9_tm_predictor_16x16 = eb_vp9_tm_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_tm_predictor_16x16 = eb_vp9_tm_predictor_16x16_sse2;
    eb_vp9_tm_predictor_32x32 = eb_vp9_tm_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_tm_predictor_32x32 = eb_vp9_tm_predictor_32x32_sse2;
    eb_vp9_tm_predictor_4x4 = eb_vp9_tm_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_tm_predictor_4x4 = eb_vp9_tm_predictor_4x4_sse2;
    eb_vp9_tm_predictor_8x8 = eb_vp9_tm_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_tm_predictor_8x8 = eb_vp9_tm_predictor_8x8_sse2;
    eb_vp9_v_predictor_16x16 = eb_vp9_v_predictor_16x16_c;
    if (flags & HAS_SSE2) eb_vp9_v_predictor_16x16 = eb_vp9_v_predictor_16x16_sse2;
    eb_vp9_v_predictor_32x32 = eb_vp9_v_predictor_32x32_c;
    if (flags & HAS_SSE2) eb_vp9_v_predictor_32x32 = eb_vp9_v_predictor_32x32_sse2;
    eb_vp9_v_predictor_4x4 = eb_vp9_v_predictor_4x4_c;
    if (flags & HAS_SSE2) eb_vp9_v_predictor_4x4 = eb_vp9_v_predictor_4x4_sse2;
    eb_vp9_v_predictor_8x8 = eb_vp9_v_predictor_8x8_c;
    if (flags & HAS_SSE2) eb_vp9_v_predictor_8x8 = eb_vp9_v_predictor_8x8_sse2;
#if 0
    vpx_variance16x16 = vpx_variance16x16_c;
    if (flags & HAS_AVX2) vpx_variance16x16 = vpx_variance16x16_avx2;
    vpx_variance16x32 = vpx_variance16x32_c;
    if (flags & HAS_AVX2) vpx_variance16x32 = vpx_variance16x32_avx2;
    vpx_variance16x8 = vpx_variance16x8_c;
    if (flags & HAS_AVX2) vpx_variance16x8 = vpx_variance16x8_avx2;
    vpx_variance32x16 = vpx_variance32x16_c;
    if (flags & HAS_AVX2) vpx_variance32x16 = vpx_variance32x16_avx2;
    vpx_variance32x32 = vpx_variance32x32_c;
    if (flags & HAS_AVX2) vpx_variance32x32 = vpx_variance32x32_avx2;
    vpx_variance32x64 = vpx_variance32x64_c;
    if (flags & HAS_AVX2) vpx_variance32x64 = vpx_variance32x64_avx2;
    vpx_variance4x4 = vpx_variance4x4_c;
    if (flags & HAS_SSE2) vpx_variance4x4 = vpx_variance4x4_sse2;
    vpx_variance4x8 = vpx_variance4x8_c;
    if (flags & HAS_SSE2) vpx_variance4x8 = vpx_variance4x8_sse2;
    vpx_variance64x32 = vpx_variance64x32_c;
    if (flags & HAS_AVX2) vpx_variance64x32 = vpx_variance64x32_avx2;
    vpx_variance64x64 = vpx_variance64x64_c;
    if (flags & HAS_AVX2) vpx_variance64x64 = vpx_variance64x64_avx2;
    vpx_variance8x16 = vpx_variance8x16_c;
    if (flags & HAS_SSE2) vpx_variance8x16 = vpx_variance8x16_sse2;
    vpx_variance8x4 = vpx_variance8x4_c;
    if (flags & HAS_SSE2) vpx_variance8x4 = vpx_variance8x4_sse2;
    vpx_variance8x8 = vpx_variance8x8_c;
    if (flags & HAS_SSE2) vpx_variance8x8 = vpx_variance8x8_sse2;
    vpx_ve_predictor_4x4 = eb_vp9_ve_predictor_4x4_c;
    vpx_vector_var = vpx_vector_var_c;
    if (flags & HAS_SSE2) vpx_vector_var = vpx_vector_var_sse2;
#endif
#endif
}

#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
