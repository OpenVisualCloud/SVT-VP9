/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeSAD_AVX2.h"
#include "stdint.h"
#include "EbMemory_AVX2.h"
#include "immintrin.h"

#define UPDATE_BEST(s, k, offset) \
  tem_sum1 = _mm_extract_epi32(s, k); \
  if (tem_sum1 < low_sum) { \
    low_sum = tem_sum1; \
    x_best = j + offset + k; \
    y_best = i; \
  }

/*******************************************************************************
 * Requirement: width   = 4, 8, 16, 24, 32, 48 or 64
 * Requirement: height <= 64
 * Requirement: height % 2 = 0 when width = 4 or 8
*******************************************************************************/
void eb_vp9_sad_loop_kernel_avx2_intrin(
    uint8_t  *src,                             // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                             // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                          // input parameter, block height (M)
    uint32_t  width,                           // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t *x_search_center,
    int16_t *y_search_center,
    uint32_t  src_stride_raw,                  // input parameter, source stride (no line skipping)
    int16_t  search_area_width,
    int16_t  search_area_height)
{
  int16_t x_best = *x_search_center, y_best = *y_search_center;
  uint32_t low_sum = 0xffffff;
  uint32_t tem_sum1 = 0;
  int16_t i, j;
  uint32_t k, l;
  uint32_t leftover = search_area_width & 7;
  const uint8_t *p_ref, *p_src;
  __m128i s0, s1, s2, s3, s4, s5, s6, s8 = _mm_set1_epi32(-1);
  __m256i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7, ss8;

  if (leftover) {
    for (k=0; k<leftover; k++) {
      s8 = _mm_slli_si128(s8, 2);
    }
  }

  switch (width) {
  case 4:

    if (!(height % 4)) {
      uint32_t srcStrideT = 3 * src_stride;
      uint32_t refStrideT = 3 * ref_stride;
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss5 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 2 * ref_stride))), _mm_loadu_si128((__m128i*)p_ref), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + ref_stride))), _mm_loadu_si128((__m128i*)(p_ref + refStrideT)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_cvtsi32_si128(*(uint32_t *)p_src), _mm_cvtsi32_si128(*(uint32_t *)(p_src + src_stride)))), _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(uint32_t *)(p_src + 2 * src_stride)), _mm_cvtsi32_si128(*(uint32_t *)(p_src + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            p_src += src_stride << 2;
            p_ref += ref_stride << 2;
          }
          ss3 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss5 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + 2 * ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + ref_stride))), _mm_loadu_si128((__m128i*)(p_ref + refStrideT)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_cvtsi32_si128(*(uint32_t *)p_src), _mm_cvtsi32_si128(*(uint32_t *)(p_src + src_stride)))), _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(uint32_t *)(p_src + 2 * src_stride)), _mm_cvtsi32_si128(*(uint32_t *)(p_src + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            p_src += src_stride << 2;
            p_ref += ref_stride << 2;
          }
          ss3 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          s3 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)p_ref);
            s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride));
            s2 = _mm_cvtsi32_si128(*(uint32_t *)p_src);
            s5 = _mm_cvtsi32_si128(*(uint32_t *)(p_src+src_stride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            p_src += src_stride << 1;
            p_ref += ref_stride << 1;
          }
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          s3 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)p_ref);
            s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride));
            s2 = _mm_cvtsi32_si128(*(uint32_t *)p_src);
            s5 = _mm_cvtsi32_si128(*(uint32_t *)(p_src+src_stride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            p_src += src_stride << 1;
            p_ref += ref_stride << 1;
          }
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }

    break;

  case 8:
    if (!(height % 4)) {
      uint32_t srcStrideT = 3 * src_stride;
      uint32_t refStrideT = 3 * ref_stride;
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + 2 * ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + ref_stride))), _mm_loadu_si128((__m128i*)(p_ref + refStrideT)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)p_src), _mm_loadl_epi64((__m128i*)(p_src + src_stride)))), _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)(p_src + 2 * src_stride)), _mm_loadl_epi64((__m128i*)(p_src + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride << 2;
            p_ref += ref_stride << 2;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)),             _mm_loadu_si128((__m128i*)(p_ref+2*ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + ref_stride))), _mm_loadu_si128((__m128i*)(p_ref + refStrideT)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)p_src), _mm_loadl_epi64((__m128i*)(p_src + src_stride)))), _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)(p_src + 2 * src_stride)), _mm_loadl_epi64((__m128i*)(p_src + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride << 2;
            p_ref += ref_stride << 2;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          s3 = s4 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)p_ref);
            s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride));
            s2 = _mm_loadl_epi64((__m128i*)p_src);
            s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));
            p_src += src_stride << 1;
            p_ref += ref_stride << 1;
          }
          s3 = _mm_adds_epu16(s3, s4);
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          s3 = s4 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)p_ref);
            s1 = _mm_loadu_si128((__m128i*)(p_ref+ref_stride));
            s2 = _mm_loadl_epi64((__m128i*)p_src);
            s5 = _mm_loadl_epi64((__m128i*)(p_src+src_stride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));
            p_src += src_stride << 1;
            p_ref += ref_stride << 1;
          }
          s3 = _mm_adds_epu16(s3, s4);
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    break;

  case 16:
    if (height <= 16) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_src)), _mm_loadu_si128((__m128i*)(p_src + src_stride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += 2 * src_stride;
            p_ref += 2 * ref_stride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_src)), _mm_loadu_si128((__m128i*)(p_src + src_stride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += 2 * src_stride;
            p_ref += 2 * ref_stride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          tem_sum1 = _mm_extract_epi16(s3, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s3, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    else if (height <= 32) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_src)), _mm_loadu_si128((__m128i*)(p_src + src_stride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += 2 * src_stride;
            p_ref += 2 * ref_stride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
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
          tem_sum1  = _mm_extract_epi16(s5, 0);
          tem_sum1 += _mm_extract_epi16(s4, 0);
          tem_sum1 += _mm_extract_epi16(s6, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_src)), _mm_loadu_si128((__m128i*)(p_src + src_stride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += 2 * src_stride;
            p_ref += 2 * ref_stride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
          s3 = _mm_or_si128(s3, s8);
          s5 = _mm_or_si128(s5, s8);
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
          tem_sum1  = _mm_extract_epi16(s5, 0);
          tem_sum1 += _mm_extract_epi16(s4, 0);
          tem_sum1 += _mm_extract_epi16(s6, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_src)), _mm_loadu_si128((__m128i*)(p_src + src_stride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += 2 * src_stride;
            p_ref += 2 * ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss0 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
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

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
            ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
            ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_src)), _mm_loadu_si128((__m128i*)(p_src + src_stride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += 2 * src_stride;
            p_ref += 2 * ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss0 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    break;

  case 24:
    if (height <= 16) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s5 = _mm256_extracti128_si256(ss5, 0);
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
          tem_sum1  = _mm_extract_epi16(s5, 0);
          tem_sum1 += _mm_extract_epi16(s4, 0);
          tem_sum1 += _mm_extract_epi16(s6, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s5 = _mm256_extracti128_si256(ss5, 0);
          s3 = _mm_or_si128(s3, s8);
          s5 = _mm_or_si128(s5, s8);
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
          tem_sum1  = _mm_extract_epi16(s5, 0);
          tem_sum1 += _mm_extract_epi16(s4, 0);
          tem_sum1 += _mm_extract_epi16(s6, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm256_extracti128_si256(ss3, 0);
          s4 = _mm256_extracti128_si256(ss3, 1);
          s5 = _mm256_extracti128_si256(ss5, 0);
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), s5);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
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
              s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), s2);
              s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), s5);
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

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm256_extracti128_si256(ss3, 0);
          s4 = _mm256_extracti128_si256(ss3, 1);
          s5 = _mm256_extracti128_si256(ss5, 0);
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), s5);
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
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
              s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), s2);
              s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), s5);
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    break;

  case 32:
    if (height <= 16) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
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
          tem_sum1  = _mm_extract_epi16(s5, 0);
          tem_sum1 += _mm_extract_epi16(s4, 0);
          tem_sum1 += _mm_extract_epi16(s6, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
            y_best = i;
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
          s3 = _mm_or_si128(s3, s8);
          s5 = _mm_or_si128(s5, s8);
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
          tem_sum1  = _mm_extract_epi16(s5, 0);
          tem_sum1 += _mm_extract_epi16(s4, 0);
          tem_sum1 += _mm_extract_epi16(s6, 0);
          if (tem_sum1 < low_sum) {
            low_sum = tem_sum1;
            x_best = (int16_t)(j + _mm_extract_epi16(s5, 1));
            y_best = i;
          }
        }
        ref += src_stride_raw;
      }
    }
    else if (height <= 32) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm256_extracti128_si256(ss6, 0);
          s4 = _mm256_extracti128_si256(ss6, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
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

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm256_extracti128_si256(ss6, 0);
          s4 = _mm256_extracti128_si256(ss6, 1);
          s0 = _mm_adds_epu16(s3, s4);
          //s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
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

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    break;

  case 48:
    if (height <= 32) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
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
          s3 = _mm_adds_epu16(s3, s4);
          s5 = _mm_adds_epu16(s5, s6);
          s0 = _mm_adds_epu16(s3, s5);
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss6, 0), _mm256_extracti128_si256(ss6, 1)));
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s1, 0, 4);
              UPDATE_BEST(s1, 1, 4);
              UPDATE_BEST(s1, 2, 4);
              UPDATE_BEST(s1, 3, 4);
            }
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
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
          s3 = _mm_adds_epu16(s3, s4);
          s5 = _mm_adds_epu16(s5, s6);
          s0 = _mm_adds_epu16(s3, s5);
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss6, 0), _mm256_extracti128_si256(ss6, 1)));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s1;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
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
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
          ss7 = _mm256_adds_epu16(ss3, ss4);
          ss8 = _mm256_adds_epu16(ss5, ss6);
          ss7 = _mm256_adds_epu16(ss7, ss8);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss7, 0), _mm256_extracti128_si256(ss7, 1)));
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s4, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s6, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s4, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s6, _mm_setzero_si128()));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s1, 0, 4);
              UPDATE_BEST(s1, 1, 4);
              UPDATE_BEST(s1, 2, 4);
              UPDATE_BEST(s1, 3, 4);
            }
          }
        }

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
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
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
          ss7 = _mm256_adds_epu16(ss3, ss4);
          ss8 = _mm256_adds_epu16(ss5, ss6);
          ss7 = _mm256_adds_epu16(ss7, ss8);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss7, 0), _mm256_extracti128_si256(ss7, 1)));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s4, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s6, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s4, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s6, _mm_setzero_si128()));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s1;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    break;

  case 64:
    if (height <= 32) {
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(p_ref + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(p_src + 32));
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
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

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(p_ref + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(p_src + 32));
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += src_stride_raw;
      }
    }
    else {
      __m256i ss9, ss10;
      for (i=0; i<search_area_height; i++) {
        for (j=0; j<=search_area_width-8; j+=8) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = ss7 = ss8 = ss9 = ss10 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(p_ref + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(p_src + 32));
            ss7 = _mm256_adds_epu16(ss7, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss8 = _mm256_adds_epu16(ss8, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss9 = _mm256_adds_epu16(ss9, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss10 = _mm256_adds_epu16(ss10, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss0 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          ss0 = _mm256_adds_epu16(ss0, _mm256_adds_epu16(_mm256_adds_epu16(ss7, ss8), _mm256_adds_epu16(ss9, ss10)));
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss3, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss5, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256())));
              ss1 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss3, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss5, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256())));
              ss2 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss7, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss9, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss10, _mm256_setzero_si256())));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss7, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss9, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss10, _mm256_setzero_si256())));
              ss0 = _mm256_add_epi32(ss0, ss2);
              ss1 = _mm256_add_epi32(ss1, ss3);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss1, 0), _mm256_extracti128_si256(ss1, 1));
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

        if (leftover) {
          p_src = src;
          p_ref = ref + j;
          ss3 = ss4 = ss5 = ss6 = ss7 = ss8 = ss9 = ss10 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)p_ref);
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref+8));
            ss2 = _mm256_loadu_si256((__m256i *)p_src);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(p_ref + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(p_src + 32));
            ss7 = _mm256_adds_epu16(ss7, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss8 = _mm256_adds_epu16(ss8, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss9 = _mm256_adds_epu16(ss9, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss10 = _mm256_adds_epu16(ss10, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            p_src += src_stride;
            p_ref += ref_stride;
          }
          ss0 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          ss0 = _mm256_adds_epu16(ss0, _mm256_adds_epu16(_mm256_adds_epu16(ss7, ss8), _mm256_adds_epu16(ss9, ss10)));
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          tem_sum1 = _mm_extract_epi16(s0, 0);
          if (tem_sum1 < low_sum) {
            if (tem_sum1 != 0xFFFF) { // no overflow
              low_sum = tem_sum1;
              x_best = (int16_t)(j + _mm_extract_epi16(s0, 1));
              y_best = i;
            }
            else {
              ss0 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss3, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss5, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256())));
              ss1 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss3, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss5, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256())));
              ss2 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss7, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss9, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss10, _mm256_setzero_si256())));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss7, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss9, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss10, _mm256_setzero_si256())));
              ss0 = _mm256_add_epi32(ss0, ss2);
              ss1 = _mm256_add_epi32(ss1, ss3);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss1, 0), _mm256_extracti128_si256(ss1, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  tem_sum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (tem_sum1 < low_sum) {
                    low_sum = tem_sum1;
                    x_best = (int16_t)(j + leftover - k);
                    y_best = i;
                  }
                }
                s0 = s3;
              }
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

/*******************************************************************************
* Requirement: height % 4 = 0
*******************************************************************************/
uint32_t compute4x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0;
    __m256i ymm = _mm256_setzero_si256();
    uint32_t y;
    (void)width;

    for (y = 0; y < height; y += 4) {
        const __m256i src0123 = load8bit_4x4_avx2(src, src_stride);
        const __m256i ref0123 = load8bit_4x4_avx2(ref, ref_stride);
        ymm = _mm256_add_epi32(ymm, _mm256_sad_epu8(src0123, ref0123));
        src += src_stride << 2;
        ref += ref_stride << 2;
    }

    xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm),
        _mm256_extracti128_si256(ymm, 1));

    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

/*******************************************************************************
* Requirement: height % 4 = 0
*******************************************************************************/
uint32_t compute8x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0;
    __m256i ymm = _mm256_setzero_si256();
    uint32_t y;
    (void)width;

    for (y = 0; y < height; y += 4) {
        const __m256i src0123 = load8bit_8x4_avx2(src, src_stride);
        const __m256i ref0123 = load8bit_8x4_avx2(ref, ref_stride);
        ymm = _mm256_add_epi32(ymm, _mm256_sad_epu8(src0123, ref0123));
        src += src_stride << 2;
        ref += ref_stride << 2;
    }

    xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm),
                         _mm256_extracti128_si256(ymm, 1));
    xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));

    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

static __m256i Compute16x2Sad_Kernel(const uint8_t *const src,
    const uint32_t src_stride, const uint8_t *const ref,
    const uint32_t ref_stride, const __m256i ymm)
{
    const __m256i src01 = load8bit_16x2_unaligned_avx2(src, src_stride);
    const __m256i ref01 = load8bit_16x2_unaligned_avx2(ref, ref_stride);
    return _mm256_add_epi32(ymm, _mm256_sad_epu8(src01, ref01));
}

/*******************************************************************************
* Requirement: height % 4 = 0
*******************************************************************************/
uint32_t compute16x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0;
    __m256i ymm = _mm256_setzero_si256();
    uint32_t y;
    (void)width;

    for (y = 0; y < height; y += 4) {
        ymm = Compute16x2Sad_Kernel(src + 0 * src_stride, src_stride,
            ref + 0 * ref_stride, ref_stride, ymm);
        ymm = Compute16x2Sad_Kernel(src + 2 * src_stride, src_stride,
            ref + 2 * ref_stride, ref_stride, ymm);
        src += src_stride << 2;
        ref += ref_stride << 2;
    }

    xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm),
                         _mm256_extracti128_si256(ymm, 1));
    xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));

    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
uint32_t compute24x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0, xmm1;
    __m256i ymm0, ymm1;
    uint32_t y;
    (void)width;

    ymm0 = ymm1 = _mm256_setzero_si256();
    for (y = 0; y < height; y += 2) {
        ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i *)ref)));
        ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + src_stride)), _mm256_loadu_si256((__m256i *)(ref + ref_stride))));
        src += src_stride << 1;
        ref += ref_stride << 1;
    }
    xmm0 = _mm_add_epi32(_mm256_extracti128_si256(ymm0, 0), _mm256_extracti128_si256(ymm1, 0));
    xmm1 = _mm_add_epi32(_mm256_extracti128_si256(ymm0, 1), _mm256_extracti128_si256(ymm1, 1));
    xmm0 = _mm_add_epi32(_mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8)), xmm1);
    return (uint32_t)_mm_cvtsi128_si32(xmm0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
uint32_t compute32x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0;
    __m256i ymm0, ymm1;
    uint32_t y;
    (void)width;

    ymm0 = ymm1 = _mm256_setzero_si256();
    for (y = 0; y < height; y += 2) {
        ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i *)ref)));
        ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + src_stride)), _mm256_loadu_si256((__m256i *)(ref + ref_stride))));
        src += src_stride << 1;
        ref += ref_stride << 1;
    }
    ymm0 = _mm256_add_epi32(ymm0, ymm1);
    xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm0), _mm256_extracti128_si256(ymm0, 1));
    xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
    return (uint32_t) /*xmm0.m128i_i64[0];*/ _mm_cvtsi128_si32(xmm0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
uint32_t compute48x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0, xmm1;
    __m256i ymm0, ymm1;
    uint32_t y;
    (void)width;

    ymm0 = ymm1 = _mm256_setzero_si256();
    xmm0 = xmm1 = _mm_setzero_si128();
    for (y = 0; y < height; y += 2) {
        ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i*)ref)));
        xmm0 = _mm_add_epi32(xmm0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 32)), _mm_loadu_si128((__m128i*)(ref + 32))));
        ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + src_stride)), _mm256_loadu_si256((__m256i*)(ref + ref_stride))));
        xmm1 = _mm_add_epi32(xmm1, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + src_stride + 32)), _mm_loadu_si128((__m128i*)(ref + ref_stride + 32))));
        src += src_stride << 1;
        ref += ref_stride << 1;
    }
    ymm0 = _mm256_add_epi32(ymm0, ymm1);
    xmm0 = _mm_add_epi32(xmm0, xmm1);
    xmm0 = _mm_add_epi32(xmm0, _mm_add_epi32(_mm256_extracti128_si256(ymm0, 0), _mm256_extracti128_si256(ymm0, 1)));
    xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
    return _mm_extract_epi32(xmm0, 0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
uint32_t compute64x_m_sad_avx2_intrin(
    uint8_t  *src,         // input parameter, source samples Ptr
    uint32_t  src_stride,  // input parameter, source stride
    uint8_t  *ref,         // input parameter, reference samples Ptr
    uint32_t  ref_stride,  // input parameter, reference stride
    uint32_t  height,      // input parameter, block height (M)
    uint32_t  width)       // input parameter, block width (N)
{
    __m128i xmm0;
    __m256i ymm0, ymm1, ymm2, ymm3;
    uint32_t y;
    (void)width;

    ymm0 = ymm1 = ymm2 = ymm3 = _mm256_setzero_si256();
    for (y = 0; y < height; y += 2) {
        ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i*)ref)));
        ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + 32)), _mm256_loadu_si256((__m256i*)(ref + 32))));
        ymm2 = _mm256_add_epi32(ymm2, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + src_stride)), _mm256_loadu_si256((__m256i*)(ref + ref_stride))));
        ymm3 = _mm256_add_epi32(ymm3, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + src_stride + 32)), _mm256_loadu_si256((__m256i*)(ref + ref_stride + 32))));
        src += src_stride << 1;
        ref += ref_stride << 1;
    }
    ymm0 = _mm256_add_epi32(_mm256_add_epi32(ymm0, ymm1), _mm256_add_epi32(ymm2, ymm3));
    xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm0), _mm256_extracti128_si256(ymm0, 1));
    xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
    return _mm_extract_epi32(xmm0, 0);
}

#ifdef DISABLE_AVX512

void eb_vp9_get_eight_horizontal_search_point_results_8x8_16x16_pu_avx2_intrin(
    uint8_t   *src,
    uint32_t   src_stride,
    uint8_t   *ref,
    uint32_t   ref_stride,
    uint32_t  *p_best_sad8x8,
    uint32_t  *p_best_mv8x8,
    uint32_t  *p_best_sad16x16,
    uint32_t  *p_best_mv16x16,
    uint32_t   mv,
    uint16_t  *p_sad16x16)
{

    int16_t x_mv, y_mv;
    __m128i s3;
    __m128i sad_0, sad_1, sad_2, sad_3;
    __m256i ss0, ss1, ss2, ss3, ss4;
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

    //8x8_0 & 8x8_1
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * src_stride)));
    ss3 = _mm256_mpsadbw_epu8(ss0, ss2, 0);
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_mpsadbw_epu8(ss1, ss2, 18);                         // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
    src += src_stride * 4;
    ref += ref_stride * 4;
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * src_stride)));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
    sad_0 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
    sad_1 = _mm_adds_epu16(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));

    //8x8_2 & 8x8_3
    src += src_stride * 4;
    ref += ref_stride * 4;
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * src_stride)));
    ss3 = _mm256_mpsadbw_epu8(ss0, ss2, 0);
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_mpsadbw_epu8(ss1, ss2, 18);                         // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111

    src += src_stride * 4;
    ref += ref_stride * 4;
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * ref_stride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * src_stride)));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
    sad_2 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
    sad_3 = _mm_adds_epu16(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));

    //16x16
    s3 = _mm_adds_epu16(_mm_adds_epu16(sad_0, sad_1), _mm_adds_epu16(sad_2, sad_3));
    //sotore the 8 SADs(16x8 SADs)
    _mm_store_si128((__m128i*)p_sad16x16, s3);
    //find the best for 16x16
    s3 = _mm_minpos_epu16(s3);
    tem_sum = _mm_extract_epi16(s3, 0) << 1;
    if (tem_sum <  p_best_sad16x16[0]) {
        p_best_sad16x16[0] = tem_sum;
        x_mv = _MVXT(mv) + (int16_t)(_mm_extract_epi16(s3, 1) * 4);
        y_mv = _MVYT(mv);
        p_best_mv16x16[0] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
    }

    //find the best for 8x8_0, 8x8_1, 8x8_2 & 8x8_3
    sad_0 = _mm_minpos_epu16(sad_0);
    sad_1 = _mm_minpos_epu16(sad_1);
    sad_2 = _mm_minpos_epu16(sad_2);
    sad_3 = _mm_minpos_epu16(sad_3);
    sad_0 = _mm_unpacklo_epi16(sad_0, sad_1);
    sad_2 = _mm_unpacklo_epi16(sad_2, sad_3);
    sad_0 = _mm_unpacklo_epi32(sad_0, sad_2);
    sad_1 = _mm_unpackhi_epi16(sad_0, _mm_setzero_si128());
    sad_0 = _mm_unpacklo_epi16(sad_0, _mm_setzero_si128());
    sad_0 = _mm_slli_epi32(sad_0, 1);
    sad_1 = _mm_slli_epi16(sad_1, 2);
    sad_2 = _mm_loadu_si128((__m128i*)p_best_sad8x8);
    s3 = _mm_cmpgt_epi32(sad_2, sad_0);
    sad_0 = _mm_min_epu32(sad_0, sad_2);
    _mm_storeu_si128((__m128i*)p_best_sad8x8, sad_0);
    sad_3 = _mm_loadu_si128((__m128i*)p_best_mv8x8);
    sad_3 = _mm_andnot_si128(s3, sad_3);
    sad_2 = _mm_set1_epi32(mv);
    sad_2 = _mm_add_epi16(sad_2, sad_1);
    sad_2 = _mm_and_si128(sad_2, s3);
    sad_2 = _mm_or_si128(sad_2, sad_3);
    _mm_storeu_si128((__m128i*)p_best_mv8x8, sad_2);

}
void eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64_pu_avx2_intrin(
    uint16_t  *p_sad16x16,
    uint32_t  *p_best_sad32x32,
    uint32_t  *p_best_sad64x64,
    uint32_t  *p_best_mv32x32,
    uint32_t  *p_best_mv64x64,
    uint32_t   mv)
{
    int16_t x_mv, y_mv;
    uint32_t tem_sum, best_sad64x64, best_mv_64x64;
    __m128i s0, s1, s2, s3, s4, s5, s6, s7, sad_0, sad_1;
    __m128i sad_00, sad_01, sad_10, sad_11, sad_20, sad_21, sad_30, sad_31;
    __m256i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7;

    s0 = _mm_setzero_si128();
    s1 = _mm_setzero_si128();
    s2 = _mm_setzero_si128();
    s3 = _mm_setzero_si128();
    s4 = _mm_setzero_si128();
    s5 = _mm_setzero_si128();
    s6 = _mm_setzero_si128();
    s7 = _mm_setzero_si128();
    sad_0 = _mm_setzero_si128();
    sad_1 = _mm_setzero_si128();

    sad_00 = _mm_setzero_si128();
    sad_01 = _mm_setzero_si128();
    sad_10 = _mm_setzero_si128();
    sad_11 = _mm_setzero_si128();
    sad_20 = _mm_setzero_si128();
    sad_21 = _mm_setzero_si128();
    sad_30 = _mm_setzero_si128();
    sad_31 = _mm_setzero_si128();

    ss0 = _mm256_setzero_si256();
    ss1 = _mm256_setzero_si256();
    ss2 = _mm256_setzero_si256();
    ss3 = _mm256_setzero_si256();
    ss4 = _mm256_setzero_si256();
    ss5 = _mm256_setzero_si256();
    ss6 = _mm256_setzero_si256();
    ss7 = _mm256_setzero_si256();

    /*--------------------
    |  32x32_0  |  32x32_1
    ----------------------
    |  32x32_2  |  32x32_3
    ----------------------*/

    /*  data ordering in p_sad16x16 buffer

    Search    Search            Search
    Point 0   Point 1           Point 7
    ---------------------------------------
    16x16_0    |    x    |    x    | ...... |    x    |
    ---------------------------------------
    16x16_1    |    x    |    x    | ...... |    x    |

    16x16_n    |    x    |    x    | ...... |    x    |

    ---------------------------------------
    16x16_15   |    x    |    x    | ...... |    x    |
    ---------------------------------------
    */

    //    __m128i Zero = _mm_setzero_si128();

    //32x32_0
    s0 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 0 * 8));
    s1 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 1 * 8));
    s2 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 2 * 8));
    s3 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 3 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_01 = _mm_add_epi32(s0, s2);
    sad_00 = _mm_add_epi32(s1, s3);

    //32x32_1
    s0 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 4 * 8));
    s1 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 5 * 8));
    s2 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 6 * 8));
    s3 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 7 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_11 = _mm_add_epi32(s0, s2);
    sad_10 = _mm_add_epi32(s1, s3);

    //32x32_2
    s0 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 8 * 8));
    s1 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 9 * 8));
    s2 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 10 * 8));
    s3 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 11 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_21 = _mm_add_epi32(s0, s2);
    sad_20 = _mm_add_epi32(s1, s3);

    //32x32_3
    s0 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 12 * 8));
    s1 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 13 * 8));
    s2 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 14 * 8));
    s3 = _mm_loadu_si128((__m128i*)(p_sad16x16 + 15 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_31 = _mm_add_epi32(s0, s2);
    sad_30 = _mm_add_epi32(s1, s3);

    sad_0 = _mm_add_epi32(_mm_add_epi32(sad_00, sad_10), _mm_add_epi32(sad_20, sad_30));
    sad_1 = _mm_add_epi32(_mm_add_epi32(sad_01, sad_11), _mm_add_epi32(sad_21, sad_31));
    sad_0 = _mm_slli_epi32(sad_0, 1);
    sad_1 = _mm_slli_epi32(sad_1, 1);

    best_sad64x64 = p_best_sad64x64[0];
    best_mv_64x64 = 0;
    //sad_0
    tem_sum = _mm_extract_epi32(sad_0, 0);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
    }
    tem_sum = _mm_extract_epi32(sad_0, 1);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 1 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_0, 2);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 2 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_0, 3);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 3 * 4;
    }

    //sad_1
    tem_sum = _mm_extract_epi32(sad_1, 0);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 4 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_1, 1);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 5 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_1, 2);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 6 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_1, 3);
    if (tem_sum < best_sad64x64) {
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 7 * 4;
    }
    if (p_best_sad64x64[0] != best_sad64x64) {
        p_best_sad64x64[0] = best_sad64x64;
        x_mv = _MVXT(mv) + (int16_t)best_mv_64x64;  y_mv = _MVYT(mv);
        p_best_mv64x64[0] = ((uint16_t)y_mv << 16) | ((uint16_t)x_mv);
    }

    // ****CODE PAST HERE IS BUGGY FOR GCC****

    // XY
    // X: 32x32 block [0..3]
    // Y: Search position [0..7]
    ss0 = _mm256_setr_m128i(sad_00, sad_01); // 07 06 05 04  03 02 01 00
    ss1 = _mm256_setr_m128i(sad_10, sad_11); // 17 16 15 14  13 12 11 10
    ss2 = _mm256_setr_m128i(sad_20, sad_21); // 27 26 25 24  23 22 21 20
    ss3 = _mm256_setr_m128i(sad_30, sad_31); // 37 36 35 34  33 32 31 30
    ss4 = _mm256_unpacklo_epi32(ss0, ss1);   // 15 05 14 04  11 01 10 00
    ss5 = _mm256_unpacklo_epi32(ss2, ss3);   // 35 25 34 24  31 21 30 20
    ss6 = _mm256_unpackhi_epi32(ss0, ss1);   // 17 07 16 06  13 03 12 02
    ss7 = _mm256_unpackhi_epi32(ss2, ss3);   // 37 27 36 26  33 23 32 22
    ss0 = _mm256_unpacklo_epi64(ss4, ss5);   // 34 24 14 04  30 20 10 00
    ss1 = _mm256_unpackhi_epi64(ss4, ss5);   // 35 25 15 05  31 21 11 01
    ss2 = _mm256_unpacklo_epi64(ss6, ss7);   // 36 26 16 06  32 22 12 02
    ss3 = _mm256_unpackhi_epi64(ss6, ss7);   // 37 27 17 07  33 23 13 03

                                             //   ss4   |  ss5-2  |                ss6        |
                                             // a0 : a1 | a2 : a3 | min(a0, a1) : min(a2, a3) |    | (ss4 & !ss6) | ((ss5-2) & ss6) | ((ss4 & !ss6) | ((ss5-2) & ss6)) |
                                             // > (-1)  | >  (-3) |         >     (-1)        | a3 |       0      |       -3        |              -3                  |
                                             // > (-1)  | >  (-3) |         <=     (0)        | a1 |      -1      |        0        |              -1                  |
                                             // > (-1)  | <= (-2) |         >     (-1)        | a2 |       0      |       -2        |              -2                  |
                                             // > (-1)  | <= (-2) |         <=     (0)        | a1 |      -1      |        0        |              -1                  |
                                             // <= (0)  | >  (-3) |         >     (-1)        | a3 |       0      |       -3        |              -3                  |
                                             // <= (0)  | >  (-3) |         <=     (0)        | a0 |       0      |        0        |               0                  |
                                             // <= (0)  | <= (-2) |         >     (-1)        | a2 |       0      |       -2        |              -2                  |
                                             // <= (0)  | <= (-2) |         <=     (0)        | a0 |       0      |        0        |               0                  |

                                             // *** 8 search points per position ***

                                             // ss0: Search Pos 0,4 for blocks 0,1,2,3
                                             // ss1: Search Pos 1,5 for blocks 0,1,2,3
                                             // ss2: Search Pos 2,6 for blocks 0,1,2,3
                                             // ss3: Search Pos 3,7 for blocks 0,1,2,3

    ss4 = _mm256_cmpgt_epi32(ss0, ss1);
    // not different SVT_LOG("%d\n", _mm_extract_epi32(_mm256_extracti128_si256(ss4, 0), 0)); // DEBUG
    //ss4 = _mm256_or_si256(_mm256_cmpgt_epi32(ss0, ss1), _mm256_cmpeq_epi32(ss0, ss1));
    ss0 = _mm256_min_epi32(ss0, ss1);
    ss5 = _mm256_cmpgt_epi32(ss2, ss3);
    //ss5 = _mm256_or_si256(_mm256_cmpgt_epi32(ss2, ss3), _mm256_cmpeq_epi32(ss2, ss3));
    ss2 = _mm256_min_epi32(ss2, ss3);
    ss5 = _mm256_sub_epi32(ss5, _mm256_set1_epi32(2)); // ss5-2

                                                       // *** 4 search points per position ***
    ss6 = _mm256_cmpgt_epi32(ss0, ss2);
    //ss6 = _mm256_or_si256(_mm256_cmpgt_epi32(ss0, ss2), _mm256_cmpeq_epi32(ss0, ss2));
    ss0 = _mm256_min_epi32(ss0, ss2);
    ss5 = _mm256_and_si256(ss5, ss6); // (ss5-2) & ss6
    ss4 = _mm256_andnot_si256(ss6, ss4); // ss4 & !ss6
    ss4 = _mm256_or_si256(ss4, ss5); // (ss4 & !ss6) | ((ss5-2) & ss6)

                                     // *** 2 search points per position ***
    s0 = _mm_setzero_si128();
    s1 = _mm_setzero_si128();
    s2 = _mm_setzero_si128();
    s3 = _mm_setzero_si128();
    s4 = _mm_setzero_si128();
    s5 = _mm_setzero_si128();
    s6 = _mm_setzero_si128();
    s7 = _mm_setzero_si128();

    // ss0 = 8 SADs, two search points for each 32x32
    // ss4 = 8 MVs, two search points for each 32x32
    //
    // XY
    // X: 32x32 block [0..3]
    // Y: search position [0..1]
    // Format: 00 10 20 30  01 11 21 31

    // Each 128 bits contains 4 32x32 32bit block results
#ifdef __GNUC__
    // SAD
    s0 = _mm256_extracti128_si256(ss0, 1);
    s1 = _mm256_extracti128_si256(ss0, 0);
    // MV
    s2 = _mm256_extracti128_si256(ss4, 1);
    s3 = _mm256_extracti128_si256(ss4, 0);
#else
    // SAD
    s0 = _mm256_extracti128_si256(ss0, 0);
    s1 = _mm256_extracti128_si256(ss0, 1);
    // MV
    s2 = _mm256_extracti128_si256(ss4, 0);
    s3 = _mm256_extracti128_si256(ss4, 1);
#endif

    //// Should be fine
    //SVT_LOG("sad0 %d, %d, %d, %d\n", _mm_extract_epi32(s0, 0), _mm_extract_epi32(s0, 1), _mm_extract_epi32(s0, 2), _mm_extract_epi32(s0, 3)); // DEBUG
    //SVT_LOG("sad1 %d, %d, %d, %d\n", _mm_extract_epi32(s1, 0), _mm_extract_epi32(s1, 1), _mm_extract_epi32(s1, 2), _mm_extract_epi32(s1, 3)); // DEBUG
    //SVT_LOG("mv0 %d, %d, %d, %d\n", _mm_extract_epi32(s2, 0), _mm_extract_epi32(s2, 1), _mm_extract_epi32(s2, 2), _mm_extract_epi32(s2, 3)); // DEBUG
    //SVT_LOG("mv1 %d, %d, %d, %d\n", _mm_extract_epi32(s3, 0), _mm_extract_epi32(s3, 1), _mm_extract_epi32(s3, 2), _mm_extract_epi32(s3, 3)); // DEBUG

    // Choose the best MV out of the two, use s4 to hold results of min
    s4 = _mm_cmpgt_epi32(s0, s1);

    // DIFFERENT BETWEEN VS AND GCC
    // SVT_LOG("%d, %d, %d, %d\n", _mm_extract_epi32(s4, 0), _mm_extract_epi32(s4, 1), _mm_extract_epi32(s4, 2), _mm_extract_epi32(s4, 3)); // DEBUG

    //s4 = _mm_or_si128(_mm_cmpgt_epi32(s0, s1), _mm_cmpeq_epi32(s0, s1));
    s0 = _mm_min_epi32(s0, s1);

    // Extract MV's based on the blocks to s2
    s3 = _mm_sub_epi32(s3, _mm_set1_epi32(4)); // s3-4
                                               // Remove the MV's are not used from s2
    s2 = _mm_andnot_si128(s4, s2);
    // Remove the MV's that are not used from s3 (inverse from s2 above operation)
    s3 = _mm_and_si128(s4, s3);
    // Combine the remaining candidates into s2
    s2 = _mm_or_si128(s2, s3);
    // Convert MV's into encoders format
    s2 = _mm_sub_epi32(_mm_setzero_si128(), s2);
    s2 = _mm_slli_epi32(s2, 2); // mv info

                                // ***SAD***
                                // s0: current SAD candidates for each 32x32
                                // s1: best SAD's for 32x32

                                // << 1 to compensate for every other line
    s0 = _mm_slli_epi32(s0, 1); // best sad info
                                // Load best SAD's
    s1 = _mm_loadu_si128((__m128i*)p_best_sad32x32);

    // Determine which candidates are better than the current best SAD's.
    // s4 is used to determine the MV's of the new best SAD's
    s4 = _mm_cmpgt_epi32(s1, s0);
    // not different SVT_LOG("%d, %d, %d, %d\n", _mm_extract_epi32(s4, 0), _mm_extract_epi32(s4, 1), _mm_extract_epi32(s4, 2), _mm_extract_epi32(s4, 3)); // DEBUG
    //s4 = _mm_or_si128(_mm_cmpgt_epi32(s1, s0), _mm_cmpeq_epi32(s1, s0));
    // Combine old and new min SAD's
    s0 = _mm_min_epu32(s0, s1);
    // Store new best SAD's back to memory
    _mm_storeu_si128((__m128i*)p_best_sad32x32, s0);

    // ***Motion Vectors***
    // Load best MV's
    // s3: candidate MV's
    // s4: results of comparing SAD's
    // s5: previous best MV's

    // Load previous best MV's
    s5 = _mm_loadu_si128((__m128i*)p_best_mv32x32);
    // Remove the MV's that are being replaced
    s5 = _mm_andnot_si128(s4, s5);
    // Set s3 to the base MV
    s3 = _mm_set1_epi32(mv);
    // Add candidate MV's to base MV
    s3 = _mm_add_epi16(s3, s2);
    // Remove non-candidate's
    s3 = _mm_and_si128(s3, s4);
    // Combine remaining candidates with remaining best MVs
    s3 = _mm_or_si128(s3, s5);
    // Store back to memory
    _mm_storeu_si128((__m128i*)p_best_mv32x32, s3);
}

#endif
