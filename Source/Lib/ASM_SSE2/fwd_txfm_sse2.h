/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VPX_VPX_DSP_X86_FWD_TXFM_SSE2_H_
#define VPX_VPX_DSP_X86_FWD_TXFM_SSE2_H_

#ifdef __cplusplus
extern "C" {
#endif

#define pair_set_epi32(a, b) \
  _mm_set_epi32((int)(b), (int)(a), (int)(b), (int)(a))

    static INLINE __m128i k_madd_epi32(__m128i a, __m128i b) {
        __m128i buf0, buf1;
        buf0 = _mm_mul_epu32(a, b);
        a = _mm_srli_epi64(a, 32);
        b = _mm_srli_epi64(b, 32);
        buf1 = _mm_mul_epu32(a, b);
        return _mm_add_epi64(buf0, buf1);
    }

    static INLINE __m128i k_packs_epi64(__m128i a, __m128i b) {
        __m128i buf0 = _mm_shuffle_epi32(a, _MM_SHUFFLE(0, 0, 2, 0));
        __m128i buf1 = _mm_shuffle_epi32(b, _MM_SHUFFLE(0, 0, 2, 0));
        return _mm_unpacklo_epi64(buf0, buf1);
    }

    static INLINE int check_epi16_overflow_x2(const __m128i *preg0,
        const __m128i *preg1) {
        const __m128i max_overflow = _mm_set1_epi16(0x7fff);
        const __m128i min_overflow = _mm_set1_epi16((short)0x8000);
        __m128i cmp0 = _mm_or_si128(_mm_cmpeq_epi16(*preg0, max_overflow),
            _mm_cmpeq_epi16(*preg0, min_overflow));
        __m128i cmp1 = _mm_or_si128(_mm_cmpeq_epi16(*preg1, max_overflow),
            _mm_cmpeq_epi16(*preg1, min_overflow));
        cmp0 = _mm_or_si128(cmp0, cmp1);
        return _mm_movemask_epi8(cmp0);
    }

    static INLINE int check_epi16_overflow_x4(const __m128i *preg0,
        const __m128i *preg1,
        const __m128i *preg2,
        const __m128i *preg3) {
        const __m128i max_overflow = _mm_set1_epi16(0x7fff);
        const __m128i min_overflow = _mm_set1_epi16((short)0x8000);
        __m128i cmp0 = _mm_or_si128(_mm_cmpeq_epi16(*preg0, max_overflow),
            _mm_cmpeq_epi16(*preg0, min_overflow));
        __m128i cmp1 = _mm_or_si128(_mm_cmpeq_epi16(*preg1, max_overflow),
            _mm_cmpeq_epi16(*preg1, min_overflow));
        __m128i cmp2 = _mm_or_si128(_mm_cmpeq_epi16(*preg2, max_overflow),
            _mm_cmpeq_epi16(*preg2, min_overflow));
        __m128i cmp3 = _mm_or_si128(_mm_cmpeq_epi16(*preg3, max_overflow),
            _mm_cmpeq_epi16(*preg3, min_overflow));
        cmp0 = _mm_or_si128(_mm_or_si128(cmp0, cmp1), _mm_or_si128(cmp2, cmp3));
        return _mm_movemask_epi8(cmp0);
    }

    static INLINE int check_epi16_overflow_x8(const __m128i *const preg) {
        int res0, res1;
        res0 = check_epi16_overflow_x4(&preg[0], &preg[1], &preg[2], &preg[3]);
        res1 = check_epi16_overflow_x4(&preg[4], &preg[5], &preg[6], &preg[7]);
        return res0 + res1;
    }

    static INLINE int check_epi16_overflow_x12(
        const __m128i *preg0, const __m128i *preg1, const __m128i *preg2,
        const __m128i *preg3, const __m128i *preg4, const __m128i *preg5,
        const __m128i *preg6, const __m128i *preg7, const __m128i *preg8,
        const __m128i *preg9, const __m128i *preg10, const __m128i *preg11) {
        int res0, res1;
        res0 = check_epi16_overflow_x4(preg0, preg1, preg2, preg3);
        res1 = check_epi16_overflow_x4(preg4, preg5, preg6, preg7);
        if (!res0) res0 = check_epi16_overflow_x4(preg8, preg9, preg10, preg11);
        return res0 + res1;
    }

    static INLINE int check_epi16_overflow_x16(
        const __m128i *preg0, const __m128i *preg1, const __m128i *preg2,
        const __m128i *preg3, const __m128i *preg4, const __m128i *preg5,
        const __m128i *preg6, const __m128i *preg7, const __m128i *preg8,
        const __m128i *preg9, const __m128i *preg10, const __m128i *preg11,
        const __m128i *preg12, const __m128i *preg13, const __m128i *preg14,
        const __m128i *preg15) {
        int res0, res1;
        res0 = check_epi16_overflow_x4(preg0, preg1, preg2, preg3);
        res1 = check_epi16_overflow_x4(preg4, preg5, preg6, preg7);
        if (!res0) {
            res0 = check_epi16_overflow_x4(preg8, preg9, preg10, preg11);
            if (!res1) res1 = check_epi16_overflow_x4(preg12, preg13, preg14, preg15);
        }
        return res0 + res1;
    }

    static INLINE int check_epi16_overflow_x32(
        const __m128i *preg0, const __m128i *preg1, const __m128i *preg2,
        const __m128i *preg3, const __m128i *preg4, const __m128i *preg5,
        const __m128i *preg6, const __m128i *preg7, const __m128i *preg8,
        const __m128i *preg9, const __m128i *preg10, const __m128i *preg11,
        const __m128i *preg12, const __m128i *preg13, const __m128i *preg14,
        const __m128i *preg15, const __m128i *preg16, const __m128i *preg17,
        const __m128i *preg18, const __m128i *preg19, const __m128i *preg20,
        const __m128i *preg21, const __m128i *preg22, const __m128i *preg23,
        const __m128i *preg24, const __m128i *preg25, const __m128i *preg26,
        const __m128i *preg27, const __m128i *preg28, const __m128i *preg29,
        const __m128i *preg30, const __m128i *preg31) {
        int res0, res1;
        res0 = check_epi16_overflow_x4(preg0, preg1, preg2, preg3);
        res1 = check_epi16_overflow_x4(preg4, preg5, preg6, preg7);
        if (!res0) {
            res0 = check_epi16_overflow_x4(preg8, preg9, preg10, preg11);
            if (!res1) {
                res1 = check_epi16_overflow_x4(preg12, preg13, preg14, preg15);
                if (!res0) {
                    res0 = check_epi16_overflow_x4(preg16, preg17, preg18, preg19);
                    if (!res1) {
                        res1 = check_epi16_overflow_x4(preg20, preg21, preg22, preg23);
                        if (!res0) {
                            res0 = check_epi16_overflow_x4(preg24, preg25, preg26, preg27);
                            if (!res1)
                                res1 = check_epi16_overflow_x4(preg28, preg29, preg30, preg31);
                        }
                    }
                }
            }
        }
        return res0 + res1;
    }

    static INLINE int k_check_epi32_overflow_4(const __m128i *preg0,
        const __m128i *preg1,
        const __m128i *preg2,
        const __m128i *preg3,
        const __m128i *zero) {
        __m128i minus_one = _mm_set1_epi32(-1);
        // Check for overflows
        __m128i reg0_shifted = _mm_slli_epi64(*preg0, 1);
        __m128i reg1_shifted = _mm_slli_epi64(*preg1, 1);
        __m128i reg2_shifted = _mm_slli_epi64(*preg2, 1);
        __m128i reg3_shifted = _mm_slli_epi64(*preg3, 1);
        __m128i reg0_top_dwords =
            _mm_shuffle_epi32(reg0_shifted, _MM_SHUFFLE(0, 0, 3, 1));
        __m128i reg1_top_dwords =
            _mm_shuffle_epi32(reg1_shifted, _MM_SHUFFLE(0, 0, 3, 1));
        __m128i reg2_top_dwords =
            _mm_shuffle_epi32(reg2_shifted, _MM_SHUFFLE(0, 0, 3, 1));
        __m128i reg3_top_dwords =
            _mm_shuffle_epi32(reg3_shifted, _MM_SHUFFLE(0, 0, 3, 1));
        __m128i top_dwords_01 = _mm_unpacklo_epi64(reg0_top_dwords, reg1_top_dwords);
        __m128i top_dwords_23 = _mm_unpacklo_epi64(reg2_top_dwords, reg3_top_dwords);
        __m128i valid_positve_01 = _mm_cmpeq_epi32(top_dwords_01, *zero);
        __m128i valid_positve_23 = _mm_cmpeq_epi32(top_dwords_23, *zero);
        __m128i valid_negative_01 = _mm_cmpeq_epi32(top_dwords_01, minus_one);
        __m128i valid_negative_23 = _mm_cmpeq_epi32(top_dwords_23, minus_one);
        int overflow_01 =
            _mm_movemask_epi8(_mm_cmpeq_epi32(valid_positve_01, valid_negative_01));
        int overflow_23 =
            _mm_movemask_epi8(_mm_cmpeq_epi32(valid_positve_23, valid_negative_23));
        return (overflow_01 + overflow_23);
    }

    static INLINE int k_check_epi32_overflow_8(
        const __m128i *preg0, const __m128i *preg1, const __m128i *preg2,
        const __m128i *preg3, const __m128i *preg4, const __m128i *preg5,
        const __m128i *preg6, const __m128i *preg7, const __m128i *zero) {
        int overflow = k_check_epi32_overflow_4(preg0, preg1, preg2, preg3, zero);
        if (!overflow) {
            overflow = k_check_epi32_overflow_4(preg4, preg5, preg6, preg7, zero);
        }
        return overflow;
    }

    static INLINE int k_check_epi32_overflow_16(
        const __m128i *preg0, const __m128i *preg1, const __m128i *preg2,
        const __m128i *preg3, const __m128i *preg4, const __m128i *preg5,
        const __m128i *preg6, const __m128i *preg7, const __m128i *preg8,
        const __m128i *preg9, const __m128i *preg10, const __m128i *preg11,
        const __m128i *preg12, const __m128i *preg13, const __m128i *preg14,
        const __m128i *preg15, const __m128i *zero) {
        int overflow = k_check_epi32_overflow_4(preg0, preg1, preg2, preg3, zero);
        if (!overflow) {
            overflow = k_check_epi32_overflow_4(preg4, preg5, preg6, preg7, zero);
            if (!overflow) {
                overflow = k_check_epi32_overflow_4(preg8, preg9, preg10, preg11, zero);
                if (!overflow) {
                    overflow =
                        k_check_epi32_overflow_4(preg12, preg13, preg14, preg15, zero);
                }
            }
        }
        return overflow;
    }

    static INLINE int k_check_epi32_overflow_32(
        const __m128i *preg0, const __m128i *preg1, const __m128i *preg2,
        const __m128i *preg3, const __m128i *preg4, const __m128i *preg5,
        const __m128i *preg6, const __m128i *preg7, const __m128i *preg8,
        const __m128i *preg9, const __m128i *preg10, const __m128i *preg11,
        const __m128i *preg12, const __m128i *preg13, const __m128i *preg14,
        const __m128i *preg15, const __m128i *preg16, const __m128i *preg17,
        const __m128i *preg18, const __m128i *preg19, const __m128i *preg20,
        const __m128i *preg21, const __m128i *preg22, const __m128i *preg23,
        const __m128i *preg24, const __m128i *preg25, const __m128i *preg26,
        const __m128i *preg27, const __m128i *preg28, const __m128i *preg29,
        const __m128i *preg30, const __m128i *preg31, const __m128i *zero) {
        int overflow = k_check_epi32_overflow_4(preg0, preg1, preg2, preg3, zero);
        if (!overflow) {
            overflow = k_check_epi32_overflow_4(preg4, preg5, preg6, preg7, zero);
            if (!overflow) {
                overflow = k_check_epi32_overflow_4(preg8, preg9, preg10, preg11, zero);
                if (!overflow) {
                    overflow =
                        k_check_epi32_overflow_4(preg12, preg13, preg14, preg15, zero);
                    if (!overflow) {
                        overflow =
                            k_check_epi32_overflow_4(preg16, preg17, preg18, preg19, zero);
                        if (!overflow) {
                            overflow =
                                k_check_epi32_overflow_4(preg20, preg21, preg22, preg23, zero);
                            if (!overflow) {
                                overflow = k_check_epi32_overflow_4(preg24, preg25, preg26,
                                    preg27, zero);
                                if (!overflow) {
                                    overflow = k_check_epi32_overflow_4(preg28, preg29, preg30,
                                        preg31, zero);
                                }
                            }
                        }
                    }
                }
            }
        }
        return overflow;
    }

    static INLINE void store_output(const __m128i output, tran_low_t *dst_ptr) {
#if CONFIG_VP9_HIGHBITDEPTH
        const __m128i zero = _mm_setzero_si128();
        const __m128i sign_bits = _mm_cmplt_epi16(output, zero);
        __m128i out0 = _mm_unpacklo_epi16(output, sign_bits);
        __m128i out1 = _mm_unpackhi_epi16(output, sign_bits);
        _mm_store_si128((__m128i *)(dst_ptr), out0);
        _mm_store_si128((__m128i *)(dst_ptr + 4), out1);
#else
        _mm_store_si128((__m128i *)(dst_ptr), output);
#endif  // CONFIG_VP9_HIGHBITDEPTH
    }

    static INLINE void storeu_output(const __m128i output, tran_low_t *dst_ptr) {
#if CONFIG_VP9_HIGHBITDEPTH
        const __m128i zero = _mm_setzero_si128();
        const __m128i sign_bits = _mm_cmplt_epi16(output, zero);
        __m128i out0 = _mm_unpacklo_epi16(output, sign_bits);
        __m128i out1 = _mm_unpackhi_epi16(output, sign_bits);
        _mm_storeu_si128((__m128i *)(dst_ptr), out0);
        _mm_storeu_si128((__m128i *)(dst_ptr + 4), out1);
#else
        _mm_storeu_si128((__m128i *)(dst_ptr), output);
#endif  // CONFIG_VP9_HIGHBITDEPTH
    }

    static INLINE __m128i mult_round_shift(const __m128i *pin0, const __m128i *pin1,
        const __m128i *pmultiplier,
        const __m128i *prounding,
        const int shift) {
        const __m128i u0 = _mm_madd_epi16(*pin0, *pmultiplier);
        const __m128i u1 = _mm_madd_epi16(*pin1, *pmultiplier);
        const __m128i v0 = _mm_add_epi32(u0, *prounding);
        const __m128i v1 = _mm_add_epi32(u1, *prounding);
        const __m128i w0 = _mm_srai_epi32(v0, shift);
        const __m128i w1 = _mm_srai_epi32(v1, shift);
        return _mm_packs_epi32(w0, w1);
    }

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VPX_DSP_X86_FWD_TXFM_SSE2_H_
