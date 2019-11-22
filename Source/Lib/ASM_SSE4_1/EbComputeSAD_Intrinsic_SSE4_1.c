/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeSAD_SSE4_1.h"
#include "EbDefinitions.h"
#include "smmintrin.h"

#define UPDATE_BEST(s, k, offset) \
  tem_sum = _mm_extract_epi32(s, k); \
  if (tem_sum < low_sum) { \
    low_sum = tem_sum; \
    x_best = j + offset + k; \
    y_best = i; \
  }

/*******************************************************************************
* Requirement: width   = 4, 8, 16, 24, 32, 48 or 64
* Requirement: height <= 64
* Requirement: height % 2 = 0 when width = 4 or 8
*******************************************************************************/
void eb_vp9_sad_loop_kernel_sse4_1_hme_l0_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width,                          // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t  *x_search_center,
    int16_t  *y_search_center,
    uint32_t  src_stride_raw,                   // input parameter, source stride (no line skipping)
    int16_t   search_area_width,
    int16_t   search_area_height)
{
    int16_t x_best = *x_search_center, y_best = *y_search_center;
    uint32_t low_sum = 0xffffff;
    uint32_t tem_sum = 0;
    int16_t i, j;
    uint32_t k, l;
    const uint8_t *p_ref, *p_src;
    __m128i s0, s1, s2, s3, s4, s5, s6, s7, s9, s10, s11;

    switch (width) {
    case 4:
        for (i = 0; i<search_area_height; i++) {
            for (j = 0; j <= search_area_width - 8; j += 8) {
                p_src = src;
                p_ref = ref + j;
                s3 = _mm_setzero_si128();
                for (k = 0; k<height; k += 2) {
                    s0 = _mm_loadu_si128((__m128i*)p_ref);
                    s1 = _mm_loadu_si128((__m128i*)(p_ref + ref_stride));
                    s2 = _mm_cvtsi32_si128(*(uint32_t *)p_src);
                    s5 = _mm_cvtsi32_si128(*(uint32_t *)(p_src + src_stride));
                    s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                    s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
                    p_src += src_stride << 1;
                    p_ref += ref_stride << 1;
                }
                s3 = _mm_minpos_epu16(s3);
                tem_sum = _mm_extract_epi16(s3, 0);
                if (tem_sum < low_sum) {
                    low_sum = tem_sum;
                    x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
                    y_best = i;
                }
            }
            ref += src_stride_raw;
        }
        break;

    case 8:
        for (i = 0; i<search_area_height; i++) {
            for (j = 0; j <= search_area_width - 8; j += 8) {
                p_src = src;
                p_ref = ref + j;
                s3 = s4 = _mm_setzero_si128();
                for (k = 0; k<height; k += 2) {
                    s0 = _mm_loadu_si128((__m128i*)p_ref);
                    s1 = _mm_loadu_si128((__m128i*)(p_ref + ref_stride));
                    s2 = _mm_loadl_epi64((__m128i*)p_src);
                    s5 = _mm_loadl_epi64((__m128i*)(p_src + src_stride));
                    s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                    s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                    s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
                    s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));
                    p_src += src_stride << 1;
                    p_ref += ref_stride << 1;
                }
                s3 = _mm_adds_epu16(s3, s4);
                s3 = _mm_minpos_epu16(s3);
                tem_sum = _mm_extract_epi16(s3, 0);
                if (tem_sum < low_sum) {
                    low_sum = tem_sum;
                    x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
                    y_best = i;
                }
            }

            ref += src_stride_raw;
        }
        break;

    case 16:
        if (height <= 16) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 16; j += 16) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    s7 = s9 = s10 = s11 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s7 = _mm_adds_epu16(s7, _mm_mpsadbw_epu8(s1, s2, 0));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 5));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 2));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s3 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s3 = _mm_minpos_epu16(s3);
                    tem_sum = _mm_extract_epi16(s3, 0);
                    if (tem_sum < low_sum) {
                        low_sum = tem_sum;
                        x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
                        y_best = i;
                    }

                    s7 = _mm_adds_epu16(_mm_adds_epu16(s7, s11), _mm_adds_epu16(s9, s10));
                    s7 = _mm_minpos_epu16(s7);
                    tem_sum = _mm_extract_epi16(s7, 0);
                    if (tem_sum < low_sum) {
                        low_sum = tem_sum;
                        x_best = (int16_t)(j + 8 + _mm_extract_epi16(s7, 1));
                        y_best = i;
                    }
                }

                ref += src_stride_raw;
            }
        }
        else if (height <= 32) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s3 = _mm_adds_epu16(s3, s4);
                    s5 = _mm_adds_epu16(s5, s6);
                    s4 = _mm_minpos_epu16(s3);
                    s6 = _mm_minpos_epu16(s5);
                    s4 = _mm_unpacklo_epi16(s4, s4);
                    s4 = _mm_unpacklo_epi32(s4, s4);
                    s4 = _mm_unpacklo_epi64(s4, s4);
                    s6 = _mm_unpacklo_epi16(s6, s6);
                    s6 = _mm_unpacklo_epi32(s6, s6);
                    s6 = _mm_unpacklo_epi64(s6, s6);
                    s3 = _mm_sub_epi16(s3, s4);
                    s5 = _mm_adds_epu16(s5, s3);
                    s5 = _mm_sub_epi16(s5, s6);
                    s5 = _mm_minpos_epu16(s5);
                    tem_sum = _mm_extract_epi16(s5, 0);
                    tem_sum += _mm_extract_epi16(s4, 0);
                    tem_sum += _mm_extract_epi16(s6, 0);
                    if (tem_sum < low_sum) {
                        low_sum = tem_sum;
                        x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
                        y_best = i;
                    }
                }

                ref += src_stride_raw;
            }
        }
        else {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                            s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                            s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                            s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                            s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                            s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                            s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                            s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7));
                            s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6));
                            UPDATE_BEST(s0, 0, 0);
                            UPDATE_BEST(s0, 1, 0);
                            UPDATE_BEST(s0, 2, 0);
                            UPDATE_BEST(s0, 3, 0);
                            UPDATE_BEST(s3, 0, 4);
                            UPDATE_BEST(s3, 1, 4);
                            UPDATE_BEST(s3, 2, 4);
                            UPDATE_BEST(s3, 3, 4);
                        }
                    }
                }
                ref += src_stride_raw;
            }
        }
        break;

    case 24:
        if (height <= 16) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s2 = _mm_loadl_epi64((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s3 = _mm_adds_epu16(s3, s4);
                    s5 = _mm_adds_epu16(s5, s6);
                    s4 = _mm_minpos_epu16(s3);
                    s6 = _mm_minpos_epu16(s5);
                    s4 = _mm_unpacklo_epi16(s4, s4);
                    s4 = _mm_unpacklo_epi32(s4, s4);
                    s4 = _mm_unpacklo_epi64(s4, s4);
                    s6 = _mm_unpacklo_epi16(s6, s6);
                    s6 = _mm_unpacklo_epi32(s6, s6);
                    s6 = _mm_unpacklo_epi64(s6, s6);
                    s3 = _mm_sub_epi16(s3, s4);
                    s5 = _mm_adds_epu16(s5, s3);
                    s5 = _mm_sub_epi16(s5, s6);
                    s5 = _mm_minpos_epu16(s5);
                    tem_sum = _mm_extract_epi16(s5, 0);
                    tem_sum += _mm_extract_epi16(s4, 0);
                    tem_sum += _mm_extract_epi16(s6, 0);
                    if (tem_sum < low_sum) {
                        low_sum = tem_sum;
                        x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
                        y_best = i;
                    }
                }
                ref += src_stride_raw;
            }
        }
        else {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s2 = _mm_loadl_epi64((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                            s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                            s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                            s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                            s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                            s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                            s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                            s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7));
                            s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6));
                            UPDATE_BEST(s0, 0, 0);
                            UPDATE_BEST(s0, 1, 0);
                            UPDATE_BEST(s0, 2, 0);
                            UPDATE_BEST(s0, 3, 0);
                            UPDATE_BEST(s3, 0, 4);
                            UPDATE_BEST(s3, 1, 4);
                            UPDATE_BEST(s3, 2, 4);
                            UPDATE_BEST(s3, 3, 4);
                        }
                    }
                }

                ref += src_stride_raw;
            }
        }
        break;

    case 32:
        if (height <= 16) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s3 = _mm_adds_epu16(s3, s4);
                    s5 = _mm_adds_epu16(s5, s6);
                    s4 = _mm_minpos_epu16(s3);
                    s6 = _mm_minpos_epu16(s5);
                    s4 = _mm_unpacklo_epi16(s4, s4);
                    s4 = _mm_unpacklo_epi32(s4, s4);
                    s4 = _mm_unpacklo_epi64(s4, s4);
                    s6 = _mm_unpacklo_epi16(s6, s6);
                    s6 = _mm_unpacklo_epi32(s6, s6);
                    s6 = _mm_unpacklo_epi64(s6, s6);
                    s3 = _mm_sub_epi16(s3, s4);
                    s5 = _mm_adds_epu16(s5, s3);
                    s5 = _mm_sub_epi16(s5, s6);
                    s5 = _mm_minpos_epu16(s5);
                    tem_sum = _mm_extract_epi16(s5, 0);
                    tem_sum += _mm_extract_epi16(s4, 0);
                    tem_sum += _mm_extract_epi16(s6, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        low_sum = tem_sum;
                        x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
                        y_best = i;
                    }
                }

                ref += src_stride_raw;
            }
        }
        else if (height <= 32) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                            s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                            s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                            s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                            s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                            s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                            s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                            s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7));
                            s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6));
                            UPDATE_BEST(s0, 0, 0);
                            UPDATE_BEST(s0, 1, 0);
                            UPDATE_BEST(s0, 2, 0);
                            UPDATE_BEST(s0, 3, 0);
                            UPDATE_BEST(s3, 0, 4);
                            UPDATE_BEST(s3, 1, 4);
                            UPDATE_BEST(s3, 2, 4);
                            UPDATE_BEST(s3, 3, 4);
                        }
                    }
                }
                ref += src_stride_raw;
            }
        }
        else {
            __m128i s9, s10, s11, s12;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height >> 1; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s9 = s10 = s11 = s12 = _mm_setzero_si128();
                    for (; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm_adds_epu16(s9, s10), _mm_adds_epu16(s11, s12)));
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                            s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                            s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                            s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                            s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                            s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                            s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                            s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7));
                            s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6));
                            s1 = _mm_unpacklo_epi16(s9, _mm_setzero_si128());
                            s9 = _mm_unpackhi_epi16(s9, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s10, _mm_setzero_si128());
                            s10 = _mm_unpackhi_epi16(s10, _mm_setzero_si128());
                            s4 = _mm_unpacklo_epi16(s11, _mm_setzero_si128());
                            s11 = _mm_unpackhi_epi16(s11, _mm_setzero_si128());
                            s5 = _mm_unpacklo_epi16(s12, _mm_setzero_si128());
                            s12 = _mm_unpackhi_epi16(s12, _mm_setzero_si128());
                            s0 = _mm_add_epi32(s0, _mm_add_epi32(_mm_add_epi32(s1, s2), _mm_add_epi32(s4, s5)));
                            s3 = _mm_add_epi32(s3, _mm_add_epi32(_mm_add_epi32(s9, s10), _mm_add_epi32(s11, s12)));
                            UPDATE_BEST(s0, 0, 0);
                            UPDATE_BEST(s0, 1, 0);
                            UPDATE_BEST(s0, 2, 0);
                            UPDATE_BEST(s0, 3, 0);
                            UPDATE_BEST(s3, 0, 4);
                            UPDATE_BEST(s3, 1, 4);
                            UPDATE_BEST(s3, 2, 4);
                            UPDATE_BEST(s3, 3, 4);
                        }
                    }
                }

                ref += src_stride_raw;
            }
        }
        break;

    case 48:
        if (height <= 32) {
            __m128i s9, s10, s11, s12;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height >> 1; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 32));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 40));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 32));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s9 = s10 = s11 = s12 = _mm_setzero_si128();
                    for (; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 32));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 40));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 32));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm_adds_epu16(s9, s10), _mm_adds_epu16(s11, s12)));
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                            s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                            s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                            s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                            s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                            s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                            s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                            s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7));
                            s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6));
                            s1 = _mm_unpacklo_epi16(s9, _mm_setzero_si128());
                            s9 = _mm_unpackhi_epi16(s9, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s10, _mm_setzero_si128());
                            s10 = _mm_unpackhi_epi16(s10, _mm_setzero_si128());
                            s4 = _mm_unpacklo_epi16(s11, _mm_setzero_si128());
                            s11 = _mm_unpackhi_epi16(s11, _mm_setzero_si128());
                            s5 = _mm_unpacklo_epi16(s12, _mm_setzero_si128());
                            s12 = _mm_unpackhi_epi16(s12, _mm_setzero_si128());
                            s0 = _mm_add_epi32(s0, _mm_add_epi32(_mm_add_epi32(s1, s2), _mm_add_epi32(s4, s5)));
                            s3 = _mm_add_epi32(s3, _mm_add_epi32(_mm_add_epi32(s9, s10), _mm_add_epi32(s11, s12)));
                            UPDATE_BEST(s0, 0, 0);
                            UPDATE_BEST(s0, 1, 0);
                            UPDATE_BEST(s0, 2, 0);
                            UPDATE_BEST(s0, 3, 0);
                            UPDATE_BEST(s3, 0, 4);
                            UPDATE_BEST(s3, 1, 4);
                            UPDATE_BEST(s3, 2, 4);
                            UPDATE_BEST(s3, 3, 4);
                        }
                    }
                }

                ref += src_stride_raw;
            }
        }
        else {
            __m128i s9, s10;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s9 = s10 = _mm_setzero_si128();
                    k = 0;
                    while (k<height) {
                        s3 = s4 = s5 = s6 = _mm_setzero_si128();
                        for (l = 0; l<21 && k<height; k++, l++) {
                            s0 = _mm_loadu_si128((__m128i*)p_ref);
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                            s2 = _mm_loadu_si128((__m128i*)p_src);
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                            s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            s0 = _mm_loadu_si128((__m128i*)(p_ref + 32));
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 40));
                            s2 = _mm_loadu_si128((__m128i*)(p_src + 32));
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            p_src += src_stride;
                            p_ref += ref_stride;
                        }
                        s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                        s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                        s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                        s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                        s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                        s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                        s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                        s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                        s9 = _mm_add_epi32(s9, _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7)));
                        s10 = _mm_add_epi32(s10, _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6)));
                    }
                    s0 = _mm_packus_epi32(s9, s10);
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            UPDATE_BEST(s9, 0, 0);
                            UPDATE_BEST(s9, 1, 0);
                            UPDATE_BEST(s9, 2, 0);
                            UPDATE_BEST(s9, 3, 0);
                            UPDATE_BEST(s10, 0, 4);
                            UPDATE_BEST(s10, 1, 4);
                            UPDATE_BEST(s10, 2, 4);
                            UPDATE_BEST(s10, 3, 4);
                        }
                    }
                }

                ref += src_stride_raw;
            }
        }
        break;

    case 64:
        if (height <= 32) {
            __m128i s9, s10, s11, s12;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    for (k = 0; k<height >> 1; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 32));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 40));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 32));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 48));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 56));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 48));
                        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                        s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                        s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s9 = s10 = s11 = s12 = _mm_setzero_si128();
                    for (; k<height; k++) {
                        s0 = _mm_loadu_si128((__m128i*)p_ref);
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                        s2 = _mm_loadu_si128((__m128i*)p_src);
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 32));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 40));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 32));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        s0 = _mm_loadu_si128((__m128i*)(p_ref + 48));
                        s1 = _mm_loadu_si128((__m128i*)(p_ref + 56));
                        s2 = _mm_loadu_si128((__m128i*)(p_src + 48));
                        s9 = _mm_adds_epu16(s9, _mm_mpsadbw_epu8(s0, s2, 0));
                        s10 = _mm_adds_epu16(s10, _mm_mpsadbw_epu8(s0, s2, 5));
                        s11 = _mm_adds_epu16(s11, _mm_mpsadbw_epu8(s1, s2, 2));
                        s12 = _mm_adds_epu16(s12, _mm_mpsadbw_epu8(s1, s2, 7));
                        p_src += src_stride;
                        p_ref += ref_stride;
                    }
                    s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
                    s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm_adds_epu16(s9, s10), _mm_adds_epu16(s11, s12)));
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                            s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                            s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                            s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                            s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                            s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                            s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                            s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7));
                            s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6));
                            s1 = _mm_unpacklo_epi16(s9, _mm_setzero_si128());
                            s9 = _mm_unpackhi_epi16(s9, _mm_setzero_si128());
                            s2 = _mm_unpacklo_epi16(s10, _mm_setzero_si128());
                            s10 = _mm_unpackhi_epi16(s10, _mm_setzero_si128());
                            s4 = _mm_unpacklo_epi16(s11, _mm_setzero_si128());
                            s11 = _mm_unpackhi_epi16(s11, _mm_setzero_si128());
                            s5 = _mm_unpacklo_epi16(s12, _mm_setzero_si128());
                            s12 = _mm_unpackhi_epi16(s12, _mm_setzero_si128());
                            s0 = _mm_add_epi32(s0, _mm_add_epi32(_mm_add_epi32(s1, s2), _mm_add_epi32(s4, s5)));
                            s3 = _mm_add_epi32(s3, _mm_add_epi32(_mm_add_epi32(s9, s10), _mm_add_epi32(s11, s12)));
                            UPDATE_BEST(s0, 0, 0);
                            UPDATE_BEST(s0, 1, 0);
                            UPDATE_BEST(s0, 2, 0);
                            UPDATE_BEST(s0, 3, 0);
                            UPDATE_BEST(s3, 0, 4);
                            UPDATE_BEST(s3, 1, 4);
                            UPDATE_BEST(s3, 2, 4);
                            UPDATE_BEST(s3, 3, 4);
                        }
                    }
                }

                ref += src_stride_raw;
            }
        }
        else {
            __m128i s9, s10;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s9 = s10 = _mm_setzero_si128();
                    k = 0;
                    while (k<height) {
                        s3 = s4 = s5 = s6 = _mm_setzero_si128();
                        for (l = 0; l<16 && k<height; k++, l++) {
                            s0 = _mm_loadu_si128((__m128i*)p_ref);
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 8));
                            s2 = _mm_loadu_si128((__m128i*)p_src);
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            s0 = _mm_loadu_si128((__m128i*)(p_ref + 16));
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 24));
                            s2 = _mm_loadu_si128((__m128i*)(p_src + 16));
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            s0 = _mm_loadu_si128((__m128i*)(p_ref + 32));
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 40));
                            s2 = _mm_loadu_si128((__m128i*)(p_src + 32));
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            s0 = _mm_loadu_si128((__m128i*)(p_ref + 48));
                            s1 = _mm_loadu_si128((__m128i*)(p_ref + 56));
                            s2 = _mm_loadu_si128((__m128i*)(p_src + 48));
                            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
                            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
                            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
                            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
                            p_src += src_stride;
                            p_ref += ref_stride;
                        }
                        s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
                        s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
                        s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
                        s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
                        s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
                        s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
                        s7 = _mm_unpacklo_epi16(s6, _mm_setzero_si128());
                        s6 = _mm_unpackhi_epi16(s6, _mm_setzero_si128());
                        s9 = _mm_add_epi32(s9, _mm_add_epi32(_mm_add_epi32(s0, s1), _mm_add_epi32(s2, s7)));
                        s10 = _mm_add_epi32(s10, _mm_add_epi32(_mm_add_epi32(s3, s4), _mm_add_epi32(s5, s6)));
                    }
                    s0 = _mm_packus_epi32(s9, s10);
                    s0 = _mm_minpos_epu16(s0);
                    tem_sum = _mm_extract_epi16(s0, 0);
                    tem_sum &= 0x0000FFFF;
                    if (tem_sum < low_sum) {
                        if (tem_sum != 0xFFFF) { // no overflow
                            low_sum = tem_sum;
                            x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
                            y_best = i;
                        }
                        else {
                            UPDATE_BEST(s9, 0, 0);
                            UPDATE_BEST(s9, 1, 0);
                            UPDATE_BEST(s9, 2, 0);
                            UPDATE_BEST(s9, 3, 0);
                            UPDATE_BEST(s10, 0, 4);
                            UPDATE_BEST(s10, 1, 4);
                            UPDATE_BEST(s10, 2, 4);
                            UPDATE_BEST(s10, 3, 4);
                        }
                    }
                }

                ref += src_stride_raw;
            }
        }
        break;

    default:
        break;
    }

    *best_sad = low_sum;
    *x_search_center = x_best;
    *y_search_center = y_best;
}

/*******************************************
 * eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu
 *******************************************/
void eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu_sse41_intrin(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint16_t  *p_sad16x16 )
{

    int16_t x_mv,y_mv;
    const uint8_t *p_ref, *p_src;
    __m128i s0, s1, s2, s3, s4, s5;
     __m128i sad_0, sad_1, sad_2, sad_3;
    uint32_t tem_sum;

     /*
    -------------------------------------   -----------------------------------
    | 8x8_00 | 8x8_01 | 8x8_04 | 8x8_05 |   8x8_16 | 8x8_17 | 8x8_20 | 8x8_21 |
    -------------------------------------   -----------------------------------
    | 8x8_02 | 8x8_03 | 8x8_06 | 8x8_07 |   8x8_18 | 8x8_19 | 8x8_22 | 8x8_23 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_08 | 8x8_09 | 8x8_12 | 8x8_13 |   8x8_24 | 8x8_25 | 8x8_29 | 8x8_29 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_10 | 8x8_11 | 8x8_14 | 8x8_15 |   8x8_26 | 8x8_27 | 8x8_30 | 8x8_31 |
    -------------------------------------   -----------------------------------

    -------------------------------------   -----------------------------------
    | 8x8_32 | 8x8_33 | 8x8_36 | 8x8_37 |   8x8_48 | 8x8_49 | 8x8_52 | 8x8_53 |
    -------------------------------------   -----------------------------------
    | 8x8_34 | 8x8_35 | 8x8_38 | 8x8_39 |   8x8_50 | 8x8_51 | 8x8_54 | 8x8_55 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_40 | 8x8_41 | 8x8_44 | 8x8_45 |   8x8_56 | 8x8_57 | 8x8_60 | 8x8_61 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_42 | 8x8_43 | 8x8_46 | 8x8_48 |   8x8_58 | 8x8_59 | 8x8_62 | 8x8_63 |
    -------------------------------------   -----------------------------------
    */

    /*
    ----------------------    ----------------------
    |  16x16_0  |  16x16_1  |  16x16_4  |  16x16_5  |
    ----------------------    ----------------------
    |  16x16_2  |  16x16_3  |  16x16_6  |  16x16_7  |
    -----------------------   -----------------------
    |  16x16_8  |  16x16_9  |  16x16_12 |  16x16_13 |
    ----------------------    ----------------------
    |  16x16_10 |  16x16_11 |  16x16_14 |  16x16_15 |
    -----------------------   -----------------------
    */

    //8x8_0
    {
        p_src = src;
        p_ref = ref;
        s3 = s4 = _mm_setzero_si128();

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        p_src += src_stride *4;
        p_ref += ref_stride *4;

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        //final 8x4 SAD
        sad_0 = _mm_adds_epu16(s3, s4);

        //find the best for 8x8_0
        s3 = _mm_minpos_epu16(sad_0);
        tem_sum = _mm_extract_epi16(s3, 0);
        if (2*tem_sum <  p_best_sad8x8[0]) {
            p_best_sad8x8[0] = 2*tem_sum;
            x_mv = _MVXT(mv)  + (int16_t)(_mm_extract_epi16(s3, 1)*4) ;
            y_mv = _MVYT(mv);
            p_best_mv8x8[0]  = ((uint16_t)y_mv<<16) | ((uint16_t)x_mv);
        }
    }

    //8x8_1
    {
        p_src = src + 8;
        p_ref = ref + 8;
        s3 = s4 = _mm_setzero_si128();

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        p_src += src_stride *4;
        p_ref += ref_stride *4;

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        //final 8x4 SAD
        sad_1 = _mm_adds_epu16(s3, s4);

        //find the best for 8x8_1
        s3 = _mm_minpos_epu16(sad_1);
        tem_sum = _mm_extract_epi16(s3, 0);
        if (2*tem_sum <  p_best_sad8x8[1]) {
            p_best_sad8x8[1] = 2*tem_sum;
            x_mv = _MVXT(mv)  + (int16_t)(_mm_extract_epi16(s3, 1)*4) ;
            y_mv = _MVYT(mv);
            p_best_mv8x8[1]  = ((uint16_t)y_mv<<16) | ((uint16_t)x_mv);
        }
    }

    //8x8_2
    {
        p_src = src + 8*src_stride;
        p_ref = ref + 8*ref_stride;
        s3 = s4 = _mm_setzero_si128();

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        p_src += src_stride *4;
        p_ref += ref_stride *4;

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        //final 8x4 SAD
        sad_2 = _mm_adds_epu16(s3, s4);

        //find the best for 8x8_2
        s3 = _mm_minpos_epu16(sad_2);
        tem_sum = _mm_extract_epi16(s3, 0);
        if (2*tem_sum <  p_best_sad8x8[2]) {
            p_best_sad8x8[2] = 2*tem_sum;
            x_mv = _MVXT(mv)  + (int16_t)(_mm_extract_epi16(s3, 1)*4) ;
            y_mv = _MVYT(mv);
            p_best_mv8x8[2]  = ((uint16_t)y_mv<<16) | ((uint16_t)x_mv);
        }
    }

    //8x8_3
    {
        p_src = src + 8 + 8*src_stride;
        p_ref = ref + 8 + 8*ref_stride;
        s3 = s4 = _mm_setzero_si128();

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        p_src += src_stride *4;
        p_ref += ref_stride *4;

        s0 = _mm_loadu_si128((__m128i*)p_ref);
        s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride*2));
        s2 = _mm_loadl_epi64((__m128i*)p_src);
        s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride*2));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
        s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
        s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));

        //final 8x4 SAD
        sad_3 = _mm_adds_epu16(s3, s4);

        //find the best for 8x8_3
        s3 = _mm_minpos_epu16(sad_3);
        tem_sum = _mm_extract_epi16(s3, 0);
        if (2*tem_sum <  p_best_sad8x8[3]) {
            p_best_sad8x8[3] = 2*tem_sum;
            x_mv = _MVXT(mv)  + (int16_t)(_mm_extract_epi16(s3, 1)*4) ;
            y_mv = _MVYT(mv);
            p_best_mv8x8[3]  = ((uint16_t)y_mv<<16) | ((uint16_t)x_mv);
        }
    }

    //16x16
    {
        s0 = _mm_adds_epu16(sad_0, sad_1);
        s1 = _mm_adds_epu16(sad_2, sad_3);
        s3 = _mm_adds_epu16(s0   , s1   );
        //sotore the 8 SADs(16x8 SADs)
        _mm_store_si128( (__m128i*)p_sad16x16, s3);
        //find the best for 16x16
        s3 = _mm_minpos_epu16(s3);
        tem_sum = _mm_extract_epi16(s3, 0);
        if (2*tem_sum <  p_best_sad16x16[0]) {
            p_best_sad16x16[0] = 2*tem_sum;
            x_mv = _MVXT(mv)  + (int16_t)(_mm_extract_epi16(s3, 1)*4) ;
            y_mv = _MVYT(mv);
            p_best_mv16x16[0]  = ((uint16_t)y_mv<<16) | ((uint16_t)x_mv);
        }
    }

}
