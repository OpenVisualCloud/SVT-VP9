/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "EbComputeSAD_SadLoopKernel_AVX512.h"
#include "stdint.h"

#define UPDATE_BEST(s, k, offset) \
  tem_sum1 = _mm_extract_epi32(s, k); \
  if (tem_sum1 < low_sum) { \
    low_sum = tem_sum1; \
    x_best = j + offset + k; \
    y_best = i; \
  }

#ifdef __cplusplus
extern "C" {
#endif

    void eb_vp9_sad_loop_kernel(
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
        int16_t  search_area_height);

#ifdef __cplusplus
}
#endif

/*******************************************************************************
* Requirement: width   = 4, 8, 16, 24, 32, 48 or 64
* Requirement: height <= 64
* Requirement: height % 2 = 0 when width = 4 or 8
*******************************************************************************/
#if (defined(__GNUC__) && (__GNUC__ < 7)) // 512-bit ver of eb_vp9_sad_loop_kernel_avx512_hme_l0_intrin is crashing -O0 GCC builds before GCC 7.x so forcing opt level
__attribute__((optimize(2)))
#endif
#ifndef DISABLE_AVX512
AVX512_FUNC_TARGET
void eb_vp9_sad_loop_kernel_avx512_hme_l0_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width,                          // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t *x_search_center,
    int16_t *y_search_center,
    uint32_t  src_stride_raw,                   // input parameter, source stride (no line skipping)
    int16_t  search_area_width,
    int16_t  search_area_height)
{
    int16_t x_best = *x_search_center, y_best = *y_search_center;
    uint32_t low_sum = 0xffffff;
    uint32_t tem_sum1 = 0;
    int32_t i, j;
    uint32_t k;
    const uint8_t *p_ref, *p_src;
    __m128i s0, s1, s2, s3, s4, s5, s6;
    __m256i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7, ss8;

    switch (width) {
    case 4:

        if (!(height % 4)) {
            uint32_t srcStrideT = 3 * src_stride;
            uint32_t refStrideT = 3 * ref_stride;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss5 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 4) {
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
                ref += src_stride_raw;
            }
        }
        else {
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 4) {
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
                ref += src_stride_raw;
            }
        }
        else {
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
    {
                if (height <= 16 && search_area_width <= 128)
                {
                    for (i = 0; i<search_area_height; i++)
                    {
                        const int n = search_area_width >> 4; // search_area_width / 16;

                        __m512i ss3sum1_aaray[8] = { _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(),
                                                     _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512() };
                        __m512i ss7sum1_aaray[8] = { _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(),
                                                     _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512() };

                        const uint8_t *pRef1 = ref;

                        ss3 = ss7 = _mm256_setzero_si256();
                        p_src = src;
                        p_ref = ref;

                        for (k = 0; k < height; k += 2)
                        {
                            __m256i ref0temp, ref1temp, ref4temp, ref3temp;

                            __m256i temp = _mm256_loadu_si256((__m256i*) (p_src));
                            ref0temp = _mm256_permutevar8x32_epi32(temp, _mm256_setr_epi64x(               0x0,                0x0, 0x0004000400040004, 0x0004000400040004));
                            ref1temp = _mm256_permutevar8x32_epi32(temp, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0005000500050005, 0x0005000500050005));
                            ref3temp = _mm256_permutevar8x32_epi32(temp, _mm256_setr_epi64x(0x0002000200020002, 0x0002000200020002, 0x0006000600060006, 0x0006000600060006));
                            ref4temp = _mm256_permutevar8x32_epi32(temp, _mm256_setr_epi64x(0x0003000300030003, 0x0003000300030003, 0x0007000700070007, 0x0007000700070007));

                            __m512i ref1ftemp = _mm512_broadcast_i64x4(ref0temp);
                            __m512i ref2ftemp = _mm512_broadcast_i64x4(ref1temp);
                            __m512i ref3ftemp = _mm512_broadcast_i64x4(ref3temp);
                            __m512i ref4ftemp = _mm512_broadcast_i64x4(ref4temp);

                            for (j = 0, p_ref = pRef1; j < n; j++, p_ref += 16)
                            {
                                __m256i ss0 = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref     ))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride     )), 0x1);
                                __m256i ss1 = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8 ))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8 )), 0x1);
                                        ss2 = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 16))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 16)), 0x1);

                                __m512i ss1ftemp = _mm512_inserti64x4(_mm512_castsi256_si512(ss0), ss1, 0x1);
                                __m512i ss2ftemp = _mm512_inserti64x4(_mm512_castsi256_si512(ss1), ss2, 0x1);

                                ss3sum1_aaray[j] = _mm512_adds_epu16(ss3sum1_aaray[j], _mm512_dbsad_epu8(ref1ftemp, ss1ftemp, 0x94));
                                ss3sum1_aaray[j] = _mm512_adds_epu16(ss3sum1_aaray[j], _mm512_dbsad_epu8(ref2ftemp, ss1ftemp, 0xE9));
                                ss7sum1_aaray[j] = _mm512_adds_epu16(ss7sum1_aaray[j], _mm512_dbsad_epu8(ref3ftemp, ss2ftemp, 0x94));
                                ss7sum1_aaray[j] = _mm512_adds_epu16(ss7sum1_aaray[j], _mm512_dbsad_epu8(ref4ftemp, ss2ftemp, 0xE9));

                            }

                            p_src += 2 * src_stride;
                            pRef1 += 2 * ref_stride;

                        }

                        for (j = 0; j < n; j++)
                        {
                            //Code performing better then original code
                            __m512i result1 = _mm512_adds_epu16(ss3sum1_aaray[j], ss7sum1_aaray[j]);
                            __m256i ss5 = _mm512_castsi512_si256(result1);
                            __m256i ss6 = _mm512_extracti64x4_epi64(result1, 1);
                            __m128i s3cum = _mm_adds_epu16(_mm256_castsi256_si128(ss5), _mm256_extracti128_si256(ss5, 1));
                            __m128i s7cum = _mm_adds_epu16(_mm256_castsi256_si128(ss6), _mm256_extracti128_si256(ss6, 1));
                            s3cum = _mm_minpos_epu16(s3cum);
                            s7cum = _mm_minpos_epu16(s7cum);
                            tem_sum1 = _mm_extract_epi16(s3cum, 0);
                            if (tem_sum1 < low_sum) {
                                low_sum = tem_sum1;
                                x_best = (int16_t)((16 * j) + _mm_extract_epi16(s3cum, 1));
                                y_best = i;
                            }

                            tem_sum1 = _mm_extract_epi16(s7cum, 0);
                            if (tem_sum1 < low_sum) {
                                low_sum = tem_sum1;
                                x_best = (int16_t)((16 * j) + 8 + _mm_extract_epi16(s7cum, 1));
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
                            ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                            for (k = 0; k<height; k += 2) {
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
                            tem_sum1 = _mm_extract_epi16(s5, 0);
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
                    for (i = 0; i<search_area_height; i++) {
                        for (j = 0; j <= search_area_width - 8; j += 8) {
                            p_src = src;
                            p_ref = ref + j;
                            ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                            for (k = 0; k<height; k += 2) {
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
                        ref += src_stride_raw;
                    }
                }
                break;
    }
    case 24:
        if (height <= 16) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                    tem_sum1 = _mm_extract_epi16(s5, 0);
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                    tem_sum1 = _mm_extract_epi16(s5, 0);
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        else {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        break;

    case 48:
        if (height <= 32) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        else {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        break;

    case 64:
        if (height <= 32) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        else {
            __m256i ss9, ss10;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = ss7 = ss8 = ss9 = ss10 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
#else
void eb_vp9_sad_loop_kernel_avx2_hme_l0_intrin(
    uint8_t  *src,                            // input parameter, source samples Ptr
    uint32_t  src_stride,                      // input parameter, source stride
    uint8_t  *ref,                            // input parameter, reference samples Ptr
    uint32_t  ref_stride,                      // input parameter, reference stride
    uint32_t  height,                         // input parameter, block height (M)
    uint32_t  width,                          // input parameter, block width (N)
    uint64_t *best_sad,
    int16_t *x_search_center,
    int16_t *y_search_center,
    uint32_t  src_stride_raw,                   // input parameter, source stride (no line skipping)
    int16_t search_area_width,
    int16_t search_area_height)
{
    int16_t x_best = *x_search_center, y_best = *y_search_center;
    uint32_t low_sum = 0xffffff;
    uint32_t tem_sum1 = 0;
    int16_t i, j;
    uint32_t k;
    const uint8_t *p_ref, *p_src;
    __m128i s0, s1, s2, s3, s4, s5, s6, s7 = _mm_set1_epi32(-1);
    __m256i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7, ss8, ss9, ss10, ss11;

    switch (width) {
    case 4:

        if (!(height % 4)) {
            uint32_t srcStrideT = 3 * src_stride;
            uint32_t refStrideT = 3 * ref_stride;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss5 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 4) {
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
                ref += src_stride_raw;
            }
        }
        else {
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 4) {
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
                ref += src_stride_raw;
            }
        }
        else {
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 16; j += 16) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    ss7 = ss9 = ss10 = ss11 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 2) {
                        ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)p_ref)), _mm_loadu_si128((__m128i*)(p_ref + ref_stride)), 0x1);
                        ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 8))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 8)), 0x1);
                        ss2 = _mm256_load_si256((__m256i*)p_src);
                        ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
                        ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
                        ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
                        ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111

                        ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(p_ref + 16))), _mm_loadu_si128((__m128i*)(p_ref + ref_stride + 16)), 0x1);
                        ss7 = _mm256_adds_epu16(ss7, _mm256_mpsadbw_epu8(ss1, ss2, 0));
                        ss11 = _mm256_adds_epu16(ss11, _mm256_mpsadbw_epu8(ss1, ss2, 45));
                        ss9 = _mm256_adds_epu16(ss9, _mm256_mpsadbw_epu8(ss0, ss2, 18));
                        ss10 = _mm256_adds_epu16(ss10, _mm256_mpsadbw_epu8(ss0, ss2, 63));

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

                    ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss7, ss11), _mm256_adds_epu16(ss9, ss10));
                    s7 = _mm_adds_epu16(_mm256_extracti128_si256(ss7, 0), _mm256_extracti128_si256(ss7, 1));
                    s7 = _mm_minpos_epu16(s7);
                    tem_sum1 = _mm_extract_epi16(s7, 0);
                    if (tem_sum1 < low_sum) {
                        low_sum = tem_sum1;
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
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 2) {
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
                    tem_sum1 = _mm_extract_epi16(s5, 0);
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k += 2) {
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
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                    tem_sum1 = _mm_extract_epi16(s5, 0);
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                    tem_sum1 = _mm_extract_epi16(s5, 0);
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
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        else {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        break;

    case 48:
        if (height <= 32) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        else {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    s3 = s4 = s5 = s6 = _mm_setzero_si128();
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        break;

    case 64:
        if (height <= 32) {
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
                ref += src_stride_raw;
            }
        }
        else {
            __m256i ss9, ss10;
            for (i = 0; i<search_area_height; i++) {
                for (j = 0; j <= search_area_width - 8; j += 8) {
                    p_src = src;
                    p_ref = ref + j;
                    ss3 = ss4 = ss5 = ss6 = ss7 = ss8 = ss9 = ss10 = _mm256_setzero_si256();
                    for (k = 0; k<height; k++) {
                        ss0 = _mm256_loadu_si256((__m256i*)p_ref);
                        ss1 = _mm256_loadu_si256((__m256i*)(p_ref + 8));
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
#endif

// AVX512VL version

/*******************************************************************************
* Requirement: width   = 4, 8, 16, 24, 32, 48 or 64
* Requirement: height <= 64
* Requirement: height % 2 = 0 when width = 4 or 8
*******************************************************************************/
#ifndef DISABLE_AVX512
#if !defined(disable_GetEightHorizontalSearchPointResultsAll85PUs_AVX512_INTRIN)
FORCE_INLINE
#endif
AVX512_FUNC_TARGET
void  GetEightHorizontalSearchPointResults_8x8_16x16_PU_AVX512_INTRIN(
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
    uint32_t tem_sum;
    __m256i ref0temp, ref1temp, ss0temp, ss1temp, ss2temp, ss3temp;
    __m256i ref2temp, ref3temp, ss4temp, ss5temp, ss6temp, ss7temp ;

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
    __m256i result1, result2, result3, result4;
    __m256i temp, temp1, temp2, temp3;
    __m128i sumsad01, sumsad23;
    result1 = result2 = result3 = result4 = _mm256_setzero_si256();

    ref0temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref                   )))  , _mm_loadu_si128((__m128i*)(ref                   + 8 )), 0x1);
    ref1temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref + (ref_stride  * 8 )))) , _mm_loadu_si128((__m128i*)(ref + (ref_stride * 8) + 8 )), 0x1);

    temp  = _mm256_broadcastsi128_si256(*(__m128i*) (src                   ));
    temp1 = _mm256_broadcastsi128_si256(*(__m128i*) (src + (src_stride * 8) ));

    ss0temp = _mm256_permutevar8x32_epi32(temp , _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss2temp = _mm256_permutevar8x32_epi32(temp , _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));
    ss1temp = _mm256_permutevar8x32_epi32(temp1, _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss3temp = _mm256_permutevar8x32_epi32(temp1, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));

    result1  = _mm256_dbsad_epu8(ss0temp, ref0temp, 0x94);
    result2  = _mm256_dbsad_epu8(ss1temp, ref1temp, 0x94);
    result3  = _mm256_dbsad_epu8(ss2temp, ref0temp, 0xE9);
    result4  = _mm256_dbsad_epu8(ss3temp, ref1temp, 0xE9);

    src += src_stride * 2;
    ref += ref_stride * 2;

    ref2temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref                   )))  , _mm_loadu_si128((__m128i*)(ref                   + 8 )), 0x1);
    ref3temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref + (ref_stride  * 8 )))) , _mm_loadu_si128((__m128i*)(ref + (ref_stride * 8) + 8 )), 0x1);

    temp2 = _mm256_broadcastsi128_si256(*(__m128i*) (src));
    temp3 = _mm256_broadcastsi128_si256(*(__m128i*) (src + (src_stride * 8)));

    ss4temp = _mm256_permutevar8x32_epi32(temp2, _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss6temp = _mm256_permutevar8x32_epi32(temp2, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));
    ss5temp = _mm256_permutevar8x32_epi32(temp3, _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss7temp = _mm256_permutevar8x32_epi32(temp3, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));

    result1  = _mm256_adds_epu16(result1, _mm256_dbsad_epu8(ss4temp, ref2temp, 0x94));
    result2  = _mm256_adds_epu16(result2, _mm256_dbsad_epu8(ss5temp, ref3temp, 0x94));
    result3  = _mm256_adds_epu16(result3, _mm256_dbsad_epu8(ss6temp, ref2temp, 0xE9));
    result4  = _mm256_adds_epu16(result4, _mm256_dbsad_epu8(ss7temp, ref3temp, 0xE9));

    src += src_stride * 2;
    ref += ref_stride * 2;

    ref0temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref                   )))  , _mm_loadu_si128((__m128i*)(ref                   + 8 )), 0x1);
    ref1temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref + (ref_stride  * 8 )))) , _mm_loadu_si128((__m128i*)(ref + (ref_stride * 8) + 8 )), 0x1);

    temp    = _mm256_broadcastsi128_si256(*(__m128i*) (src                   ));
    temp1   = _mm256_broadcastsi128_si256(*(__m128i*) (src + (src_stride * 8) ));

    ss0temp = _mm256_permutevar8x32_epi32(temp , _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss2temp = _mm256_permutevar8x32_epi32(temp , _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));
    ss1temp = _mm256_permutevar8x32_epi32(temp1, _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss3temp = _mm256_permutevar8x32_epi32(temp1, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));

    result1  = _mm256_adds_epu16(result1, _mm256_dbsad_epu8(ss0temp, ref0temp, 0x94));
    result2  = _mm256_adds_epu16(result2, _mm256_dbsad_epu8(ss1temp, ref1temp, 0x94));
    result3  = _mm256_adds_epu16(result3, _mm256_dbsad_epu8(ss2temp, ref0temp, 0xE9));
    result4  = _mm256_adds_epu16(result4, _mm256_dbsad_epu8(ss3temp, ref1temp, 0xE9));

    src += src_stride * 2;
    ref += ref_stride * 2;

    ref2temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref                   )))  , _mm_loadu_si128((__m128i*)(ref                   + 8 )), 0x1);
    ref3temp = _mm256_inserti128_si256( _mm256_castsi128_si256(_mm_loadu_si128((__m128i*    ) (ref + (ref_stride  * 8 )))) , _mm_loadu_si128((__m128i*)(ref + (ref_stride * 8) + 8 )), 0x1);

    temp2 = _mm256_broadcastsi128_si256(*(__m128i*) (src));
    temp3 = _mm256_broadcastsi128_si256(*(__m128i*) (src + (src_stride * 8)));

    ss4temp = _mm256_permutevar8x32_epi32(temp2, _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss6temp = _mm256_permutevar8x32_epi32(temp2, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));
    ss5temp = _mm256_permutevar8x32_epi32(temp3, _mm256_setr_epi64x(0x0               , 0x0               , 0x0006000600060006, 0x0006000600060006));
    ss7temp = _mm256_permutevar8x32_epi32(temp3, _mm256_setr_epi64x(0x0001000100010001, 0x0001000100010001, 0x0007000700070007, 0x0007000700070007));

    result1  = _mm256_adds_epu16(result1, _mm256_dbsad_epu8(ss4temp, ref2temp, 0x94));
    result2  = _mm256_adds_epu16(result2, _mm256_dbsad_epu8(ss5temp, ref3temp, 0x94));
    result3  = _mm256_adds_epu16(result3, _mm256_dbsad_epu8(ss6temp, ref2temp, 0xE9));
    result4  = _mm256_adds_epu16(result4, _mm256_dbsad_epu8(ss7temp, ref3temp, 0xE9));

    result1 = _mm256_adds_epu16(result1, result3);
    result2 = _mm256_adds_epu16(result2, result4);

    sad_0 = _mm256_castsi256_si128  (result1   );
    sad_1 = _mm256_extracti128_si256(result1, 1);
    sad_2 = _mm256_castsi256_si128  (result2   );
    sad_3 = _mm256_extracti128_si256(result2, 1);

    sumsad01 = _mm_adds_epu16(sad_0, sad_1);
    sumsad23 = _mm_adds_epu16(sad_2, sad_3);
    s3 = _mm_adds_epu16(sumsad01, sumsad23);

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
#endif
/*******************************************
* GetEightHorizontalSearchPointResultsAll85CUs
*******************************************/

#ifndef DISABLE_AVX512
/*******************************************
Calcualte SAD for 32x32,64x64 from 16x16
and check if there is improvement, if yes keep
the best SAD+MV
*******************************************/
AVX512_FUNC_TARGET
FORCE_INLINE static
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

    s0     = _mm_setzero_si128();
    s1     = _mm_setzero_si128();
    s2     = _mm_setzero_si128();
    s3     = _mm_setzero_si128();
    s4     = _mm_setzero_si128();
    s5     = _mm_setzero_si128();
    s6     = _mm_setzero_si128();
    s7     = _mm_setzero_si128();
    sad_0  = _mm_setzero_si128();
    sad_1  = _mm_setzero_si128();

    sad_00 = _mm_setzero_si128();
    sad_01 = _mm_setzero_si128();
    sad_10 = _mm_setzero_si128();
    sad_11 = _mm_setzero_si128();
    sad_20 = _mm_setzero_si128();
    sad_21 = _mm_setzero_si128();
    sad_30 = _mm_setzero_si128();
    sad_31 = _mm_setzero_si128();

    ss0    = _mm256_setzero_si256();
    ss1    = _mm256_setzero_si256();
    ss2    = _mm256_setzero_si256();
    ss3    = _mm256_setzero_si256();
    ss4    = _mm256_setzero_si256();
    ss5    = _mm256_setzero_si256();
    ss6    = _mm256_setzero_si256();
    ss7    = _mm256_setzero_si256();

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
    s0            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 0 * 8));
    s1            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 1 * 8));
    s2            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 2 * 8));
    s3            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 3 * 8));

    s4            = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0            = _mm_add_epi32(s4, s6);
    s1            = _mm_add_epi32(s5, s7);

    s4            = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2            = _mm_add_epi32(s4, s6);
    s3            = _mm_add_epi32(s5, s7);

    sad_01        = _mm_add_epi32(s0, s2);
    sad_00        = _mm_add_epi32(s1, s3);

    //32x32_1
    s0            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 4 * 8));
    s1            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 5 * 8));
    s2            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 6 * 8));
    s3            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 7 * 8));

    s4            = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0            = _mm_add_epi32(s4, s6);
    s1            = _mm_add_epi32(s5, s7);

    s4            = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2            = _mm_add_epi32(s4, s6);
    s3            = _mm_add_epi32(s5, s7);

    sad_11        = _mm_add_epi32(s0, s2);
    sad_10        = _mm_add_epi32(s1, s3);

    //32x32_2
    s0            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 8 * 8));
    s1            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 9 * 8));
    s2            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 10 * 8));
    s3            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 11 * 8));

    s4            = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0            = _mm_add_epi32(s4, s6);
    s1            = _mm_add_epi32(s5, s7);

    s4            = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2            = _mm_add_epi32(s4, s6);
    s3            = _mm_add_epi32(s5, s7);

    sad_21        = _mm_add_epi32(s0, s2);
    sad_20        = _mm_add_epi32(s1, s3);

    //32x32_3
    s0            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 12 * 8));
    s1            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 13 * 8));
    s2            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 14 * 8));
    s3            = _mm_loadu_si128((__m128i*)(p_sad16x16 + 15 * 8));

    s4            = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0            = _mm_add_epi32(s4, s6);
    s1            = _mm_add_epi32(s5, s7);

    s4            = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5            = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6            = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7            = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2            = _mm_add_epi32(s4, s6);
    s3            = _mm_add_epi32(s5, s7);

    sad_31        = _mm_add_epi32(s0, s2);
    sad_30        = _mm_add_epi32(s1, s3);

    sad_0         = _mm_add_epi32(_mm_add_epi32(sad_00, sad_10), _mm_add_epi32(sad_20, sad_30));
    sad_1         = _mm_add_epi32(_mm_add_epi32(sad_01, sad_11), _mm_add_epi32(sad_21, sad_31));
    sad_0         = _mm_slli_epi32(sad_0, 1);
    sad_1         = _mm_slli_epi32(sad_1, 1);

    best_sad64x64 = p_best_sad64x64[0];
    best_mv_64x64 = 0;
    //sad_0
    tem_sum = _mm_extract_epi32(sad_0, 0);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
    }
    tem_sum = _mm_extract_epi32(sad_0, 1);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 1 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_0, 2);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 2 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_0, 3);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 3 * 4;
    }

    //sad_1
    tem_sum = _mm_extract_epi32(sad_1, 0);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 4 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_1, 1);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 5 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_1, 2);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 6 * 4;
    }
    tem_sum = _mm_extract_epi32(sad_1, 3);
    if (tem_sum < best_sad64x64){
        best_sad64x64 = tem_sum;
        best_mv_64x64 = 7 * 4;
    }
    if (p_best_sad64x64[0] != best_sad64x64) {
        p_best_sad64x64[0] = best_sad64x64;
        x_mv = _MVXT(mv) + (int16_t) best_mv_64x64;  y_mv = _MVYT(mv);
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

AVX512_FUNC_TARGET
void eb_vp9_get_eight_horizontal_search_point_results_all85_p_us_avx512_intrin(
    MeContext               *context_ptr,
    uint32_t                   list_index,
    uint32_t                   search_region_index,
    uint32_t                   x_search_index,
    uint32_t                   y_search_index
)
{
    uint8_t  *src_ptr                 = context_ptr->sb_src_ptr;
    uint8_t  *ref_ptr                  = context_ptr->integer_buffer_ptr[list_index][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * context_ptr->interpolated_full_stride[list_index][0]);
    uint32_t refluma_stride           = context_ptr->interpolated_full_stride[list_index][0];

    uint32_t search_position_tl_index = search_region_index;

    uint32_t src_next16x16_offset     = (MAX_SB_SIZE << 4);
    uint32_t ref_next16x16_offset     = (refluma_stride << 4);

    uint32_t curr_mv_y                = (((uint16_t)y_search_index) << 18);
    uint16_t curr_mv_x                = (((uint16_t)x_search_index << 2));
    uint32_t curr_mv                  = curr_mv_y | curr_mv_x;

    uint32_t  *p_best_sad8x8          = context_ptr->p_best_sad8x8;
    uint32_t  *p_best_sad16x16        = context_ptr->p_best_sad16x16;
    uint32_t  *p_best_sad32x32        = context_ptr->p_best_sad32x32;
    uint32_t  *p_best_sad64x64        = context_ptr->p_best_sad64x64;

    uint32_t  *p_best_mv8x8           = context_ptr->p_best_mv8x8;
    uint32_t  *p_best_mv16x16         = context_ptr->p_best_mv16x16;
    uint32_t  *p_best_mv32x32         = context_ptr->p_best_mv32x32;
    uint32_t  *p_best_mv64x64         = context_ptr->p_best_mv64x64;

    uint16_t  *p_sad16x16             = context_ptr->p_eight_pos_sad16x16;

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

    const uint32_t  src_stride        = context_ptr->sb_src_stride;
    const char c_blknum_tbl[16]     = { 0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15 };

    uint32_t search_position_index      = search_position_tl_index;
    uint32_t block_index               = 0;

    src_next16x16_offset            = src_stride << 4;

    for (int j = 0; j < 16; j += 4)
    {
        int i = j, blockIndex_l = block_index, searchPositionIndex_l = search_position_index;
        for ( ; i < (j+4); ++i, blockIndex_l += 16, searchPositionIndex_l += 16)
        {
            int blck = c_blknum_tbl[i];
            // for best performance this function is inlined
            GetEightHorizontalSearchPointResults_8x8_16x16_PU_AVX512_INTRIN(src_ptr + blockIndex_l,          src_stride,
                                                                            ref_ptr + searchPositionIndex_l, refluma_stride,
                                                                            &p_best_sad8x8[4 * blck],
                                                                            &p_best_mv8x8[4 * blck],
                                                                            &p_best_sad16x16[blck],
                                                                            &p_best_mv16x16[blck],
                                                                            curr_mv,
                                                                            &p_sad16x16[8 * blck]);
        }
        block_index          += src_next16x16_offset;
        search_position_index += ref_next16x16_offset;
    }

    ////32x32 and 64x64
    eb_vp9_get_eight_horizontal_search_point_results_32x32_64x64_pu_avx2_intrin(p_sad16x16, p_best_sad32x32, p_best_sad64x64, p_best_mv32x32, p_best_mv64x64, curr_mv);
}
#endif
