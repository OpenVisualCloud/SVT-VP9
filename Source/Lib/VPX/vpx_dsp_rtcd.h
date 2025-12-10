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

#include <stddef.h>
#include <stdint.h>
#include "vpx_dsp_common.h"
#include "vpx_filter.h"

#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

// TT temporary untill we fix the linking error
void eb_vp9_filter_block1d8_v2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                     ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_v2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                      ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_v2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                     ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_h2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                      ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d8_h2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                     ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_h2_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                     ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_v2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                          ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d16_h2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                          ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d8_v2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                         ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d8_h2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                         ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_v2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                         ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);
void eb_vp9_filter_block1d4_h2_avg_ssse3(const uint8_t *src_ptr, ptrdiff_t src_pitch, uint8_t *output_ptr,
                                         ptrdiff_t out_pitch, uint32_t output_height, const int16_t *filter);

void eb_vp9_convolve8_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                        const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w, int h);
void eb_vp9_convolve8_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                           const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                           int h);
RTCD_EXTERN void (*eb_vp9_convolve8)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                     const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
                                     int w, int h);

void eb_vp9_convolve8_avg_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                            const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                            int h);
void eb_vp9_convolve8_avg_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                               const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                               int h);
RTCD_EXTERN void (*eb_vp9_convolve8_avg)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                         const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
                                         int w, int h);

void eb_vp9_convolve8_avg_horiz_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                  const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                                  int h);
void eb_vp9_convolve8_avg_horiz_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                     const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
                                     int w, int h);
RTCD_EXTERN void (*eb_vp9_convolve8_avg_horiz)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst,
                                               ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4,
                                               int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_avg_vert_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                 const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                                 int h);
void eb_vp9_convolve8_avg_vert_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                    const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
                                    int w, int h);
RTCD_EXTERN void (*eb_vp9_convolve8_avg_vert)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst,
                                              ptrdiff_t dst_stride, const InterpKernel *filter, int x0_q4,
                                              int x_step_q4, int y0_q4, int y_step_q4, int w, int h);

void eb_vp9_convolve8_horiz_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                              const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                              int h);
void eb_vp9_convolve8_horiz_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                 const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                                 int h);
RTCD_EXTERN void (*eb_vp9_convolve8_horiz)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                           const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4,
                                           int y_step_q4, int w, int h);

void eb_vp9_convolve8_vert_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                             const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                             int h);
void eb_vp9_convolve8_vert_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                                int h);
RTCD_EXTERN void (*eb_vp9_convolve8_vert)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                          const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4,
                                          int y_step_q4, int w, int h);

void eb_vp9_convolve_avg_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                           const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                           int h);
void eb_vpx_convolve_avg_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                              const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                              int h);
RTCD_EXTERN void (*eb_vpx_convolve_avg)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                        const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
                                        int w, int h);

void eb_vp9_convolve_copy_c(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                            const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                            int h);
void eb_vpx_convolve_copy_avx2(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                               const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4, int w,
                               int h);
RTCD_EXTERN void (*eb_vpx_convolve_copy)(const uint8_t *src, ptrdiff_t src_stride, uint8_t *dst, ptrdiff_t dst_stride,
                                         const InterpKernel *filter, int x0_q4, int x_step_q4, int y0_q4, int y_step_q4,
                                         int w, int h);

void eb_vp9_d117_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d117_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d117_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_32x32_avx2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d117_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d117_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d117_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d117_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d117_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d117_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d135_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d135_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d135_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_32x32_avx2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d135_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d135_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d135_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d135_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d135_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d135_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d153_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_d153_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d153_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d153_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d153_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d153_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d153_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d153_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d153_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d153_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d207_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d207_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d207_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d207_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_d207_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d207_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d207_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d207_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d207_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_d45_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d45_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                               const uint8_t *left);

void eb_vp9_d45_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d45_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                               const uint8_t *left);

void eb_vp9_d45_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d45_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_d45_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d45_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d45_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_d63_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_16x16_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d63_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                               const uint8_t *left);

void eb_vp9_d63_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_32x32_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d63_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                               const uint8_t *left);

void eb_vp9_d63_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_4x4_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d63_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_d63_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_d63_predictor_8x8_ssse3(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_d63_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_dc_128_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_128_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                  const uint8_t *left);

void eb_vp9_dc_128_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_128_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                  const uint8_t *left);

void eb_vp9_dc_128_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_128_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_dc_128_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_128_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_128_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_dc_left_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_left_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                   const uint8_t *left);

void eb_vp9_dc_left_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_left_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                   const uint8_t *left);

void eb_vp9_dc_left_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_left_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                 const uint8_t *left);

void eb_vp9_dc_left_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_left_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_left_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                 const uint8_t *left);

void eb_vp9_dc_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_dc_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_dc_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                            const uint8_t *left);

void eb_vp9_dc_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                            const uint8_t *left);

void eb_vp9_dc_top_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_top_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                  const uint8_t *left);

void eb_vp9_dc_top_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_top_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                  const uint8_t *left);

void eb_vp9_dc_top_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_top_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_dc_top_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_dc_top_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_dc_top_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                                const uint8_t *left);

void eb_vp9_fdct32x32_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vp9_fdct32x32_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void (*eb_vp9_fdct32x32)(const int16_t *input, tran_low_t *output, int stride);

void eb_vpx_partial_fdct32x32_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vpx_partial_fdct32x32_avx2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void (*eb_vpx_partial_fdct32x32)(const int16_t *input, tran_low_t *output, int stride);

void eb_vp9_fdct4x4_c(const int16_t *input, tran_low_t *output, int stride);
void eb_vp9_fdct4x4_sse2(const int16_t *input, tran_low_t *output, int stride);
RTCD_EXTERN void (*eb_vpx_fdct4x4)(const int16_t *input, tran_low_t *output, int stride);

void eb_vp9_fdct8x8_c(const int16_t *input, tran_low_t *output, int stride);

void eb_vp9_h_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_h_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_h_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_h_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_h_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_h_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_h_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_h_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_h_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_idct16x16_10_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_10_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct16x16_10_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct16x16_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct16x16_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct16x16_256_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_256_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct16x16_256_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct16x16_38_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct16x16_38_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct16x16_38_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_1024_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct32x32_1024_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vp9_idct32x32_1024_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_135_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct32x32_135_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vp9_idct32x32_135_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vpx_idct32x32_1_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct32x32_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct32x32_34_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct32x32_34_add_avx2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vp9_idct32x32_34_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct4x4_16_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct4x4_16_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct4x4_16_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct4x4_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct4x4_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct4x4_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct8x8_12_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct8x8_12_add_ssse3(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vp9_idct8x8_12_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct8x8_1_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct8x8_1_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vpx_idct8x8_1_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_idct8x8_64_add_c(const tran_low_t *input, uint8_t *dest, int stride);
void eb_vp9_idct8x8_64_add_sse2(const tran_low_t *input, uint8_t *dest, int stride);
RTCD_EXTERN void (*eb_vp9_idct8x8_64_add)(const tran_low_t *input, uint8_t *dest, int stride);

void eb_vp9_lpf_horizontal_16_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                const uint8_t *thresh);
void eb_vp9_lpf_horizontal_16_avx2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                   const uint8_t *thresh);
RTCD_EXTERN void (*eb_vp9_lpf_horizontal_16)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                             const uint8_t *thresh);

void eb_vp9_lpf_horizontal_16_dual_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                     const uint8_t *thresh);
void eb_vp9_lpf_horizontal_16_dual_avx2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                        const uint8_t *thresh);
RTCD_EXTERN void (*eb_vp9_lpf_horizontal_16_dual)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                                  const uint8_t *thresh);

void eb_vp9_lpf_horizontal_4_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                               const uint8_t *thresh);
void eb_vp9_lpf_horizontal_4_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                  const uint8_t *thresh);
RTCD_EXTERN void (*eb_vpx_lpf_horizontal_4)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                            const uint8_t *thresh);

void eb_vp9_lpf_horizontal_4_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                    const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                    const uint8_t *thresh1);
void eb_vp9_lpf_horizontal_4_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                       const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                       const uint8_t *thresh1);
RTCD_EXTERN void (*eb_vpx_lpf_horizontal_4_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                                 const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                                 const uint8_t *thresh1);

void eb_vp9_lpf_horizontal_8_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                               const uint8_t *thresh);
void eb_vp9_lpf_horizontal_8_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                  const uint8_t *thresh);
RTCD_EXTERN void (*eb_vpx_lpf_horizontal_8)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                            const uint8_t *thresh);

void eb_vp9_lpf_horizontal_8_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                    const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                    const uint8_t *thresh1);
void eb_vp9_lpf_horizontal_8_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                       const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                       const uint8_t *thresh1);
RTCD_EXTERN void (*eb_vpx_lpf_horizontal_8_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                                 const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                                 const uint8_t *thresh1);

void eb_vp9_lpf_vertical_16_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                              const uint8_t *thresh);
void eb_vp9_lpf_vertical_16_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                 const uint8_t *thresh);
RTCD_EXTERN void (*eb_vpx_lpf_vertical_16)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                           const uint8_t *thresh);

void eb_vp9_lpf_vertical_16_dual_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                   const uint8_t *thresh);
void eb_vpx_lpf_vertical_16_dual_avx2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                      const uint8_t *thresh);

RTCD_EXTERN void (*eb_vpx_lpf_vertical_16_dual)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                                const uint8_t *thresh);

void eb_vp9_lpf_vertical_4_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_vertical_4_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                const uint8_t *thresh);
RTCD_EXTERN void (*eb_vpx_lpf_vertical_4)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                          const uint8_t *thresh);

void eb_vp9_lpf_vertical_4_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                  const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                  const uint8_t *thresh1);
void eb_vp9_lpf_vertical_4_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                     const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                     const uint8_t *thresh1);
RTCD_EXTERN void (*eb_vpx_lpf_vertical_4_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                               const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                               const uint8_t *thresh1);

void eb_vp9_lpf_vertical_8_c(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit, const uint8_t *thresh);
void eb_vp9_lpf_vertical_8_sse2(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                const uint8_t *thresh);
RTCD_EXTERN void (*eb_vpx_lpf_vertical_8)(uint8_t *s, int pitch, const uint8_t *blimit, const uint8_t *limit,
                                          const uint8_t *thresh);

void eb_vp9_lpf_vertical_8_dual_c(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                  const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                  const uint8_t *thresh1);
void eb_vp9_lpf_vertical_8_dual_sse2(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                     const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                     const uint8_t *thresh1);
RTCD_EXTERN void (*eb_vpx_lpf_vertical_8_dual)(uint8_t *s, int pitch, const uint8_t *blimit0, const uint8_t *limit0,
                                               const uint8_t *thresh0, const uint8_t *blimit1, const uint8_t *limit1,
                                               const uint8_t *thresh1);

void eb_vp9_quantize_b_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr,
                         const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr,
                         tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
                         const int16_t *scan, const int16_t *iscan);
void eb_vp9_quantize_b_avx(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr,
                           const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr,
                           tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr,
                           uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
RTCD_EXTERN void (*eb_vp9_quantize_b)(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block,
                                      const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr,
                                      const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                                      const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan,
                                      const int16_t *iscan);

void eb_vp9_quantize_b_32x32_c(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block, const int16_t *zbin_ptr,
                               const int16_t *round_ptr, const int16_t *quant_ptr, const int16_t *quant_shift_ptr,
                               tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr,
                               uint16_t *eob_ptr, const int16_t *scan, const int16_t *iscan);
void eb_vp9_quantize_b_32x32_avx(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block,
                                 const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr,
                                 const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                                 const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan,
                                 const int16_t *iscan);
RTCD_EXTERN void (*eb_vp9_quantize_b_32x32)(const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block,
                                            const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr,
                                            const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
                                            tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
                                            const int16_t *scan, const int16_t *iscan);

void eb_vp9_tm_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_tm_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_tm_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_tm_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                              const uint8_t *left);

void eb_vp9_tm_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_tm_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                            const uint8_t *left);

void eb_vp9_tm_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_tm_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_tm_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                            const uint8_t *left);

void eb_vp9_v_predictor_16x16_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_16x16_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_v_predictor_16x16)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_v_predictor_32x32_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_32x32_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_v_predictor_32x32)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above,
                                             const uint8_t *left);

void eb_vp9_v_predictor_4x4_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_4x4_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_v_predictor_4x4)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

void eb_vp9_v_predictor_8x8_c(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
void eb_vp9_v_predictor_8x8_sse2(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);
RTCD_EXTERN void (*eb_vp9_v_predictor_8x8)(uint8_t *dst, ptrdiff_t y_stride, const uint8_t *above, const uint8_t *left);

#ifdef RTCD_C

static void setup_rtcd_internal(uint32_t asm_type) {
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

    eb_vp9_convolve8 = eb_vp9_convolve8_c;
    if (flags & HAS_AVX2)
        eb_vp9_convolve8 = eb_vp9_convolve8_avx2;
    eb_vp9_convolve8_avg = eb_vp9_convolve8_avg_c;
    if (flags & HAS_AVX2)
        eb_vp9_convolve8_avg = eb_vp9_convolve8_avg_avx2;
    eb_vp9_convolve8_avg_horiz = eb_vp9_convolve8_avg_horiz_c;
    if (flags & HAS_AVX2)
        eb_vp9_convolve8_avg_horiz = eb_vp9_convolve8_avg_horiz_avx2;
    eb_vp9_convolve8_avg_vert = eb_vp9_convolve8_avg_vert_c;
    if (flags & HAS_AVX2)
        eb_vp9_convolve8_avg_vert = eb_vp9_convolve8_avg_vert_avx2;
    eb_vp9_convolve8_horiz = eb_vp9_convolve8_horiz_c;
    if (flags & HAS_AVX2)
        eb_vp9_convolve8_horiz = eb_vp9_convolve8_horiz_avx2;
    eb_vp9_convolve8_vert = eb_vp9_convolve8_vert_c;
    if (flags & HAS_AVX2)
        eb_vp9_convolve8_vert = eb_vp9_convolve8_vert_avx2;
    eb_vpx_convolve_avg = eb_vp9_convolve_avg_c;
    if (flags & HAS_AVX2)
        eb_vpx_convolve_avg = eb_vpx_convolve_avg_avx2;
    eb_vpx_convolve_copy = eb_vp9_convolve_copy_c;
    if (flags & HAS_AVX2)
        eb_vpx_convolve_copy = eb_vpx_convolve_copy_avx2;
    eb_vp9_d117_predictor_4x4 = eb_vp9_d117_predictor_4x4_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d117_predictor_4x4 = eb_vp9_d117_predictor_4x4_ssse3;
    eb_vp9_d117_predictor_8x8 = eb_vp9_d117_predictor_8x8_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d117_predictor_8x8 = eb_vp9_d117_predictor_8x8_ssse3;
    eb_vp9_d117_predictor_16x16 = eb_vp9_d117_predictor_16x16_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d117_predictor_16x16 = eb_vp9_d117_predictor_16x16_ssse3;
    eb_vp9_d117_predictor_32x32 = eb_vp9_d117_predictor_32x32_c;
    if (flags & HAS_AVX2)
        eb_vp9_d117_predictor_32x32 = eb_vp9_d117_predictor_32x32_avx2;
    eb_vp9_d135_predictor_4x4 = eb_vp9_d135_predictor_4x4_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d135_predictor_4x4 = eb_vp9_d135_predictor_4x4_ssse3;
    eb_vp9_d135_predictor_8x8 = eb_vp9_d135_predictor_8x8_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d135_predictor_8x8 = eb_vp9_d135_predictor_8x8_ssse3;
    eb_vp9_d135_predictor_16x16 = eb_vp9_d135_predictor_16x16_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d135_predictor_16x16 = eb_vp9_d135_predictor_16x16_ssse3;
    eb_vp9_d135_predictor_32x32 = eb_vp9_d135_predictor_32x32_c;
    if (flags & HAS_AVX2)
        eb_vp9_d135_predictor_32x32 = eb_vp9_d135_predictor_32x32_avx2;
    eb_vp9_d153_predictor_16x16 = eb_vp9_d153_predictor_16x16_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d153_predictor_16x16 = eb_vp9_d153_predictor_16x16_ssse3;
    eb_vp9_d153_predictor_32x32 = eb_vp9_d153_predictor_32x32_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d153_predictor_32x32 = eb_vp9_d153_predictor_32x32_ssse3;
    eb_vp9_d153_predictor_4x4 = eb_vp9_d153_predictor_4x4_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d153_predictor_4x4 = eb_vp9_d153_predictor_4x4_ssse3;
    eb_vp9_d153_predictor_8x8 = eb_vp9_d153_predictor_8x8_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d153_predictor_8x8 = eb_vp9_d153_predictor_8x8_ssse3;
    eb_vp9_d207_predictor_16x16 = eb_vp9_d207_predictor_16x16_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d207_predictor_16x16 = eb_vp9_d207_predictor_16x16_ssse3;
    eb_vp9_d207_predictor_32x32 = eb_vp9_d207_predictor_32x32_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d207_predictor_32x32 = eb_vp9_d207_predictor_32x32_ssse3;
    eb_vp9_d207_predictor_4x4 = eb_vp9_d207_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_d207_predictor_4x4 = eb_vp9_d207_predictor_4x4_sse2;
    eb_vp9_d207_predictor_8x8 = eb_vp9_d207_predictor_8x8_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d207_predictor_8x8 = eb_vp9_d207_predictor_8x8_ssse3;
    eb_vp9_d45_predictor_16x16 = eb_vp9_d45_predictor_16x16_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d45_predictor_16x16 = eb_vp9_d45_predictor_16x16_ssse3;
    eb_vp9_d45_predictor_32x32 = eb_vp9_d45_predictor_32x32_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d45_predictor_32x32 = eb_vp9_d45_predictor_32x32_ssse3;
    eb_vp9_d45_predictor_4x4 = eb_vp9_d45_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_d45_predictor_4x4 = eb_vp9_d45_predictor_4x4_sse2;
    eb_vp9_d45_predictor_8x8 = eb_vp9_d45_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_d45_predictor_8x8 = eb_vp9_d45_predictor_8x8_sse2;
    eb_vp9_d63_predictor_16x16 = eb_vp9_d63_predictor_16x16_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d63_predictor_16x16 = eb_vp9_d63_predictor_16x16_ssse3;
    eb_vp9_d63_predictor_32x32 = eb_vp9_d63_predictor_32x32_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d63_predictor_32x32 = eb_vp9_d63_predictor_32x32_ssse3;
    eb_vp9_d63_predictor_4x4 = eb_vp9_d63_predictor_4x4_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d63_predictor_4x4 = eb_vp9_d63_predictor_4x4_ssse3;
    eb_vp9_d63_predictor_8x8 = eb_vp9_d63_predictor_8x8_c;
    if (flags & HAS_SSSE3)
        eb_vp9_d63_predictor_8x8 = eb_vp9_d63_predictor_8x8_ssse3;
    eb_vp9_dc_128_predictor_16x16 = eb_vp9_dc_128_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_128_predictor_16x16 = eb_vp9_dc_128_predictor_16x16_sse2;
    eb_vp9_dc_128_predictor_32x32 = eb_vp9_dc_128_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_128_predictor_32x32 = eb_vp9_dc_128_predictor_32x32_sse2;
    eb_vp9_dc_128_predictor_4x4 = eb_vp9_dc_128_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_128_predictor_4x4 = eb_vp9_dc_128_predictor_4x4_sse2;
    eb_vp9_dc_128_predictor_8x8 = eb_vp9_dc_128_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_128_predictor_8x8 = eb_vp9_dc_128_predictor_8x8_sse2;
    eb_vp9_dc_left_predictor_16x16 = eb_vp9_dc_left_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_left_predictor_16x16 = eb_vp9_dc_left_predictor_16x16_sse2;
    eb_vp9_dc_left_predictor_32x32 = eb_vp9_dc_left_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_left_predictor_32x32 = eb_vp9_dc_left_predictor_32x32_sse2;
    eb_vp9_dc_left_predictor_4x4 = eb_vp9_dc_left_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_left_predictor_4x4 = eb_vp9_dc_left_predictor_4x4_sse2;
    eb_vp9_dc_left_predictor_8x8 = eb_vp9_dc_left_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_left_predictor_8x8 = eb_vp9_dc_left_predictor_8x8_sse2;
    eb_vp9_dc_predictor_16x16 = eb_vp9_dc_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_predictor_16x16 = eb_vp9_dc_predictor_16x16_sse2;
    eb_vp9_dc_predictor_32x32 = eb_vp9_dc_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_predictor_32x32 = eb_vp9_dc_predictor_32x32_sse2;
    eb_vp9_dc_predictor_4x4 = eb_vp9_dc_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_predictor_4x4 = eb_vp9_dc_predictor_4x4_sse2;
    eb_vp9_dc_predictor_8x8 = eb_vp9_dc_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_predictor_8x8 = eb_vp9_dc_predictor_8x8_sse2;
    eb_vp9_dc_top_predictor_16x16 = eb_vp9_dc_top_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_top_predictor_16x16 = eb_vp9_dc_top_predictor_16x16_sse2;
    eb_vp9_dc_top_predictor_32x32 = eb_vp9_dc_top_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_top_predictor_32x32 = eb_vp9_dc_top_predictor_32x32_sse2;
    eb_vp9_dc_top_predictor_4x4 = eb_vp9_dc_top_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_top_predictor_4x4 = eb_vp9_dc_top_predictor_4x4_sse2;
    eb_vp9_dc_top_predictor_8x8 = eb_vp9_dc_top_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_dc_top_predictor_8x8 = eb_vp9_dc_top_predictor_8x8_sse2;

    eb_vp9_fdct32x32 = eb_vp9_fdct32x32_c;
    if (flags & HAS_AVX2)
        eb_vp9_fdct32x32 = eb_vp9_fdct32x32_avx2;
    eb_vpx_partial_fdct32x32 = eb_vpx_partial_fdct32x32_c;
    if (flags & HAS_AVX2)
        eb_vpx_partial_fdct32x32 = eb_vpx_partial_fdct32x32_avx2;
    eb_vpx_fdct4x4 = eb_vp9_fdct4x4_c;
    if (flags & HAS_SSE2)
        eb_vpx_fdct4x4 = eb_vp9_fdct4x4_sse2;
    eb_vp9_h_predictor_16x16 = eb_vp9_h_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_h_predictor_16x16 = eb_vp9_h_predictor_16x16_sse2;
    eb_vp9_h_predictor_32x32 = eb_vp9_h_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_h_predictor_32x32 = eb_vp9_h_predictor_32x32_sse2;
    eb_vp9_h_predictor_4x4 = eb_vp9_h_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_h_predictor_4x4 = eb_vp9_h_predictor_4x4_sse2;
    eb_vp9_h_predictor_8x8 = eb_vp9_h_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_h_predictor_8x8 = eb_vp9_h_predictor_8x8_sse2;
    eb_vpx_idct16x16_10_add = eb_vp9_idct16x16_10_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct16x16_10_add = eb_vp9_idct16x16_10_add_sse2;
    eb_vpx_idct16x16_1_add = eb_vp9_idct16x16_1_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct16x16_1_add = eb_vp9_idct16x16_1_add_sse2;
    eb_vpx_idct16x16_256_add = eb_vp9_idct16x16_256_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct16x16_256_add = eb_vp9_idct16x16_256_add_sse2;
    eb_vpx_idct16x16_38_add = eb_vp9_idct16x16_38_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct16x16_38_add = eb_vp9_idct16x16_38_add_sse2;
    eb_vp9_idct32x32_1024_add = eb_vp9_idct32x32_1024_add_c;
    if (flags & HAS_AVX2)
        eb_vp9_idct32x32_1024_add = eb_vp9_idct32x32_1024_add_avx2;
    eb_vp9_idct32x32_135_add = eb_vp9_idct32x32_135_add_c;
    if (flags & HAS_AVX2)
        eb_vp9_idct32x32_135_add = eb_vp9_idct32x32_135_add_avx2;
    eb_vpx_idct32x32_1_add = eb_vp9_idct32x32_1_add_c;
    if (flags & HAS_AVX2)
        eb_vpx_idct32x32_1_add = eb_vpx_idct32x32_1_add_avx2;
    eb_vp9_idct32x32_34_add = eb_vp9_idct32x32_34_add_c;
    if (flags & HAS_AVX2)
        eb_vp9_idct32x32_34_add = eb_vp9_idct32x32_34_add_avx2;
    eb_vpx_idct4x4_16_add = eb_vp9_idct4x4_16_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct4x4_16_add = eb_vp9_idct4x4_16_add_sse2;
    eb_vpx_idct4x4_1_add = eb_vp9_idct4x4_1_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct4x4_1_add = eb_vp9_idct4x4_1_add_sse2;
    eb_vp9_idct8x8_12_add = eb_vp9_idct8x8_12_add_c;
    if (flags & HAS_SSSE3)
        eb_vp9_idct8x8_12_add = eb_vp9_idct8x8_12_add_ssse3;
    eb_vpx_idct8x8_1_add = eb_vp9_idct8x8_1_add_c;
    if (flags & HAS_SSE2)
        eb_vpx_idct8x8_1_add = eb_vp9_idct8x8_1_add_sse2;
    eb_vp9_idct8x8_64_add = eb_vp9_idct8x8_64_add_c;
    if (flags & HAS_SSE2)
        eb_vp9_idct8x8_64_add = eb_vp9_idct8x8_64_add_sse2;
    eb_vp9_lpf_horizontal_16 = eb_vp9_lpf_horizontal_16_c;
    if (flags & HAS_AVX2)
        eb_vp9_lpf_horizontal_16 = eb_vp9_lpf_horizontal_16_avx2;
    eb_vp9_lpf_horizontal_16_dual = eb_vp9_lpf_horizontal_16_dual_c;
    if (flags & HAS_AVX2)
        eb_vp9_lpf_horizontal_16_dual = eb_vp9_lpf_horizontal_16_dual_avx2;
    eb_vpx_lpf_horizontal_4 = eb_vp9_lpf_horizontal_4_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_horizontal_4 = eb_vp9_lpf_horizontal_4_sse2;
    eb_vpx_lpf_horizontal_4_dual = eb_vp9_lpf_horizontal_4_dual_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_horizontal_4_dual = eb_vp9_lpf_horizontal_4_dual_sse2;
    eb_vpx_lpf_horizontal_8 = eb_vp9_lpf_horizontal_8_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_horizontal_8 = eb_vp9_lpf_horizontal_8_sse2;
    eb_vpx_lpf_horizontal_8_dual = eb_vp9_lpf_horizontal_8_dual_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_horizontal_8_dual = eb_vp9_lpf_horizontal_8_dual_sse2;
    eb_vpx_lpf_vertical_16 = eb_vp9_lpf_vertical_16_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_vertical_16 = eb_vp9_lpf_vertical_16_sse2;
    eb_vpx_lpf_vertical_16_dual = eb_vp9_lpf_vertical_16_dual_c;
    if (flags & HAS_AVX2)
        eb_vpx_lpf_vertical_16_dual = eb_vpx_lpf_vertical_16_dual_avx2;

    eb_vpx_lpf_vertical_4 = eb_vp9_lpf_vertical_4_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_vertical_4 = eb_vp9_lpf_vertical_4_sse2;
    eb_vpx_lpf_vertical_4_dual = eb_vp9_lpf_vertical_4_dual_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_vertical_4_dual = eb_vp9_lpf_vertical_4_dual_sse2;
    eb_vpx_lpf_vertical_8 = eb_vp9_lpf_vertical_8_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_vertical_8 = eb_vp9_lpf_vertical_8_sse2;
    eb_vpx_lpf_vertical_8_dual = eb_vp9_lpf_vertical_8_dual_c;
    if (flags & HAS_SSE2)
        eb_vpx_lpf_vertical_8_dual = eb_vp9_lpf_vertical_8_dual_sse2;
    eb_vp9_quantize_b = eb_vp9_quantize_b_c;
    if (flags & HAS_AVX)
        eb_vp9_quantize_b = eb_vp9_quantize_b_avx;
    eb_vp9_quantize_b_32x32 = eb_vp9_quantize_b_32x32_c;
    if (flags & HAS_AVX)
        eb_vp9_quantize_b_32x32 = eb_vp9_quantize_b_32x32_avx;
    eb_vp9_tm_predictor_16x16 = eb_vp9_tm_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_tm_predictor_16x16 = eb_vp9_tm_predictor_16x16_sse2;
    eb_vp9_tm_predictor_32x32 = eb_vp9_tm_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_tm_predictor_32x32 = eb_vp9_tm_predictor_32x32_sse2;
    eb_vp9_tm_predictor_4x4 = eb_vp9_tm_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_tm_predictor_4x4 = eb_vp9_tm_predictor_4x4_sse2;
    eb_vp9_tm_predictor_8x8 = eb_vp9_tm_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_tm_predictor_8x8 = eb_vp9_tm_predictor_8x8_sse2;
    eb_vp9_v_predictor_16x16 = eb_vp9_v_predictor_16x16_c;
    if (flags & HAS_SSE2)
        eb_vp9_v_predictor_16x16 = eb_vp9_v_predictor_16x16_sse2;
    eb_vp9_v_predictor_32x32 = eb_vp9_v_predictor_32x32_c;
    if (flags & HAS_SSE2)
        eb_vp9_v_predictor_32x32 = eb_vp9_v_predictor_32x32_sse2;
    eb_vp9_v_predictor_4x4 = eb_vp9_v_predictor_4x4_c;
    if (flags & HAS_SSE2)
        eb_vp9_v_predictor_4x4 = eb_vp9_v_predictor_4x4_sse2;
    eb_vp9_v_predictor_8x8 = eb_vp9_v_predictor_8x8_c;
    if (flags & HAS_SSE2)
        eb_vp9_v_predictor_8x8 = eb_vp9_v_predictor_8x8_sse2;
}

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
