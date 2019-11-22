/*
 *  Copyright (c) 2017 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>
#ifdef _WIN32
#include <intrin.h>
#endif
#include <immintrin.h>

#include "vpx_dsp_rtcd.h"
 //#include "vpx/vpx_integer.h"
 //#include "vpx_dsp/x86/bitdepth_conversion_sse2.h"
 //#include "vpx_dsp/x86/quantize_x86.h"

static INLINE void load_b_values(const int16_t *zbin_ptr, __m128i *zbin,
    const int16_t *round_ptr, __m128i *round,
    const int16_t *quant_ptr, __m128i *quant,
    const int16_t *dequant_ptr, __m128i *dequant,
    const int16_t *shift_ptr, __m128i *shift) {
    *zbin = _mm_load_si128((const __m128i *)zbin_ptr);
    *round = _mm_load_si128((const __m128i *)round_ptr);
    *quant = _mm_load_si128((const __m128i *)quant_ptr);
    *zbin = _mm_sub_epi16(*zbin, _mm_set1_epi16(1));
    *dequant = _mm_load_si128((const __m128i *)dequant_ptr);
    *shift = _mm_load_si128((const __m128i *)shift_ptr);
}
static INLINE void calculate_qcoeff(__m128i *coeff, const __m128i round,
    const __m128i quant, const __m128i shift) {
    __m128i tmp, qcoeff;
    qcoeff = _mm_adds_epi16(*coeff, round);
    tmp = _mm_mulhi_epi16(qcoeff, quant);
    qcoeff = _mm_add_epi16(tmp, qcoeff);
    *coeff = _mm_mulhi_epi16(qcoeff, shift);
}
static INLINE __m128i calculate_dqcoeff(__m128i qcoeff, __m128i dequant) {
    return _mm_mullo_epi16(qcoeff, dequant);
}

// Scan 16 values for eob reference in scan_ptr. Use masks (-1) from comparing
// to zbin to add 1 to the index in 'scan'.
static INLINE __m128i scan_for_eob(__m128i *coeff0, __m128i *coeff1,
    const __m128i zbin_mask0,
    const __m128i zbin_mask1,
    const int16_t *scan_ptr, const int index,
    const __m128i zero) {
    const __m128i zero_coeff0 = _mm_cmpeq_epi16(*coeff0, zero);
    const __m128i zero_coeff1 = _mm_cmpeq_epi16(*coeff1, zero);
    __m128i scan0 = _mm_load_si128((const __m128i *)(scan_ptr + index));
    __m128i scan1 = _mm_load_si128((const __m128i *)(scan_ptr + index + 8));
    __m128i eob0, eob1;
    // Add one to convert from indices to counts
    scan0 = _mm_sub_epi16(scan0, zbin_mask0);
    scan1 = _mm_sub_epi16(scan1, zbin_mask1);
    eob0 = _mm_andnot_si128(zero_coeff0, scan0);
    eob1 = _mm_andnot_si128(zero_coeff1, scan1);
    return _mm_max_epi16(eob0, eob1);
}

static INLINE int16_t accumulate_eob(__m128i eob) {
    __m128i eob_shuffled;
    eob_shuffled = _mm_shuffle_epi32(eob, 0xe);
    eob = _mm_max_epi16(eob, eob_shuffled);
    eob_shuffled = _mm_shufflelo_epi16(eob, 0xe);
    eob = _mm_max_epi16(eob, eob_shuffled);
    eob_shuffled = _mm_shufflelo_epi16(eob, 0x1);
    eob = _mm_max_epi16(eob, eob_shuffled);
    return _mm_extract_epi16(eob, 1);
}

// Load 8 16 bit values. If the source is 32 bits then pack down with
// saturation.
static INLINE __m128i load_tran_low(const tran_low_t *a) {
#if CONFIG_VP9_HIGHBITDEPTH
    const __m128i a_low = _mm_load_si128((const __m128i *)a);
    return _mm_packs_epi32(a_low, *(const __m128i *)(a + 4));
#else
    return _mm_load_si128((const __m128i *)a);
#endif
}

// Store 8 16 bit values. If the destination is 32 bits then sign extend the
// values by multiplying by 1.
static INLINE void store_tran_low(__m128i a, tran_low_t *b) {
#if CONFIG_VP9_HIGHBITDEPTH
    const __m128i one = _mm_set1_epi16(1);
    const __m128i a_hi = _mm_mulhi_epi16(a, one);
    const __m128i a_lo = _mm_mullo_epi16(a, one);
    const __m128i a_1 = _mm_unpacklo_epi16(a_lo, a_hi);
    const __m128i a_2 = _mm_unpackhi_epi16(a_lo, a_hi);
    _mm_store_si128((__m128i *)(b), a_1);
    _mm_store_si128((__m128i *)(b + 4), a_2);
#else
    _mm_store_si128((__m128i *)(b), a);
#endif
}

void eb_vp9_quantize_b_avx(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
    int skip_block, const int16_t *zbin_ptr,
    const int16_t *round_ptr, const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
    tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr,
    uint16_t *eob_ptr, const int16_t *scan_ptr,
    const int16_t *iscan_ptr) {
    const __m128i zero = _mm_setzero_si128();
    const __m256i big_zero = _mm256_setzero_si256();
    int index;

    __m128i zbin, round, quant, dequant, shift;
    __m128i coeff0, coeff1;
    __m128i qcoeff0, qcoeff1;
    __m128i cmp_mask0, cmp_mask1;
    __m128i all_zero;
    __m128i eob = zero, eob0;

    (void)scan_ptr;
    (void)skip_block;
    assert(!skip_block);

    *eob_ptr = 0;

    load_b_values(zbin_ptr, &zbin, round_ptr, &round, quant_ptr, &quant,
        dequant_ptr, &dequant, quant_shift_ptr, &shift);

    // Do DC and first 15 AC.
    coeff0 = load_tran_low(coeff_ptr);
    coeff1 = load_tran_low(coeff_ptr + 8);

    qcoeff0 = _mm_abs_epi16(coeff0);
    qcoeff1 = _mm_abs_epi16(coeff1);

    cmp_mask0 = _mm_cmpgt_epi16(qcoeff0, zbin);
    zbin = _mm_unpackhi_epi64(zbin, zbin);  // Switch DC to AC
    cmp_mask1 = _mm_cmpgt_epi16(qcoeff1, zbin);

    all_zero = _mm_or_si128(cmp_mask0, cmp_mask1);
    if (_mm_test_all_zeros(all_zero, all_zero)) {
        _mm256_storeu_si256((__m256i *)(qcoeff_ptr), big_zero);
        _mm256_storeu_si256((__m256i *)(dqcoeff_ptr), big_zero);
#if CONFIG_VP9_HIGHBITDEPTH
        _mm256_storeu_si256((__m256i *)(qcoeff_ptr + 8), big_zero);
        _mm256_storeu_si256((__m256i *)(dqcoeff_ptr + 8), big_zero);
#endif  // CONFIG_VP9_HIGHBITDEPTH

        if (n_coeffs == 16) return;

        round = _mm_unpackhi_epi64(round, round);
        quant = _mm_unpackhi_epi64(quant, quant);
        shift = _mm_unpackhi_epi64(shift, shift);
        dequant = _mm_unpackhi_epi64(dequant, dequant);
    }
    else {
        calculate_qcoeff(&qcoeff0, round, quant, shift);
        round = _mm_unpackhi_epi64(round, round);
        quant = _mm_unpackhi_epi64(quant, quant);
        shift = _mm_unpackhi_epi64(shift, shift);
        calculate_qcoeff(&qcoeff1, round, quant, shift);

        // Reinsert signs
        qcoeff0 = _mm_sign_epi16(qcoeff0, coeff0);
        qcoeff1 = _mm_sign_epi16(qcoeff1, coeff1);

        // Mask out zbin threshold coeffs
        qcoeff0 = _mm_and_si128(qcoeff0, cmp_mask0);
        qcoeff1 = _mm_and_si128(qcoeff1, cmp_mask1);

        store_tran_low(qcoeff0, qcoeff_ptr);
        store_tran_low(qcoeff1, qcoeff_ptr + 8);

        coeff0 = calculate_dqcoeff(qcoeff0, dequant);
        dequant = _mm_unpackhi_epi64(dequant, dequant);
        coeff1 = calculate_dqcoeff(qcoeff1, dequant);

        store_tran_low(coeff0, dqcoeff_ptr);
        store_tran_low(coeff1, dqcoeff_ptr + 8);

        eob = scan_for_eob(&coeff0, &coeff1, cmp_mask0, cmp_mask1, iscan_ptr, 0,
            zero);
    }

    // AC only loop.
    for (index = 16; index < n_coeffs; index += 16) {
        coeff0 = load_tran_low(coeff_ptr + index);
        coeff1 = load_tran_low(coeff_ptr + index + 8);

        qcoeff0 = _mm_abs_epi16(coeff0);
        qcoeff1 = _mm_abs_epi16(coeff1);

        cmp_mask0 = _mm_cmpgt_epi16(qcoeff0, zbin);
        cmp_mask1 = _mm_cmpgt_epi16(qcoeff1, zbin);

        all_zero = _mm_or_si128(cmp_mask0, cmp_mask1);
        if (_mm_test_all_zeros(all_zero, all_zero)) {
            _mm256_storeu_si256((__m256i *)(qcoeff_ptr + index), big_zero);
            _mm256_storeu_si256((__m256i *)(dqcoeff_ptr + index), big_zero);
#if CONFIG_VP9_HIGHBITDEPTH
            _mm256_storeu_si256((__m256i *)(qcoeff_ptr + index + 8), big_zero);
            _mm256_storeu_si256((__m256i *)(dqcoeff_ptr + index + 8), big_zero);
#endif  // CONFIG_VP9_HIGHBITDEPTH
            continue;
        }

        calculate_qcoeff(&qcoeff0, round, quant, shift);
        calculate_qcoeff(&qcoeff1, round, quant, shift);

        qcoeff0 = _mm_sign_epi16(qcoeff0, coeff0);
        qcoeff1 = _mm_sign_epi16(qcoeff1, coeff1);

        qcoeff0 = _mm_and_si128(qcoeff0, cmp_mask0);
        qcoeff1 = _mm_and_si128(qcoeff1, cmp_mask1);

        store_tran_low(qcoeff0, qcoeff_ptr + index);
        store_tran_low(qcoeff1, qcoeff_ptr + index + 8);

        coeff0 = calculate_dqcoeff(qcoeff0, dequant);
        coeff1 = calculate_dqcoeff(qcoeff1, dequant);

        store_tran_low(coeff0, dqcoeff_ptr + index);
        store_tran_low(coeff1, dqcoeff_ptr + index + 8);

        eob0 = scan_for_eob(&coeff0, &coeff1, cmp_mask0, cmp_mask1, iscan_ptr,
            index, zero);
        eob = _mm_max_epi16(eob, eob0);
    }

    *eob_ptr = accumulate_eob(eob);
}

void eb_vp9_quantize_b_32x32_avx(
    const tran_low_t *coeff_ptr, intptr_t n_coeffs, int skip_block,
    const int16_t *zbin_ptr, const int16_t *round_ptr, const int16_t *quant_ptr,
    const int16_t *quant_shift_ptr, tran_low_t *qcoeff_ptr,
    tran_low_t *dqcoeff_ptr, const int16_t *dequant_ptr, uint16_t *eob_ptr,
    const int16_t *scan_ptr, const int16_t *iscan_ptr) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i one = _mm_set1_epi16(1);
    const __m256i big_zero = _mm256_setzero_si256();
    int index;

    __m128i zbin, round, quant, dequant, shift;
    __m128i coeff0, coeff1;
    __m128i qcoeff0, qcoeff1;
    __m128i cmp_mask0, cmp_mask1;
    __m128i all_zero;
    __m128i eob = zero, eob0;

    (void)scan_ptr;
    (void)n_coeffs;
    (void)skip_block;
    assert(!skip_block);

    // Setup global values.
    // The 32x32 halves zbin and round.
    zbin = _mm_load_si128((const __m128i *)zbin_ptr);
    // Shift with rounding.
    zbin = _mm_add_epi16(zbin, one);
    zbin = _mm_srli_epi16(zbin, 1);
    // x86 has no "greater *or equal*" comparison. Subtract 1 from zbin so
    // it is a strict "greater" comparison.
    zbin = _mm_sub_epi16(zbin, one);

    round = _mm_load_si128((const __m128i *)round_ptr);
    round = _mm_add_epi16(round, one);
    round = _mm_srli_epi16(round, 1);

    quant = _mm_load_si128((const __m128i *)quant_ptr);
    dequant = _mm_load_si128((const __m128i *)dequant_ptr);
    shift = _mm_load_si128((const __m128i *)quant_shift_ptr);
    shift = _mm_slli_epi16(shift, 1);

    // Do DC and first 15 AC.
    coeff0 = load_tran_low(coeff_ptr);
    coeff1 = load_tran_low(coeff_ptr + 8);

    qcoeff0 = _mm_abs_epi16(coeff0);
    qcoeff1 = _mm_abs_epi16(coeff1);

    cmp_mask0 = _mm_cmpgt_epi16(qcoeff0, zbin);
    zbin = _mm_unpackhi_epi64(zbin, zbin);  // Switch DC to AC.
    cmp_mask1 = _mm_cmpgt_epi16(qcoeff1, zbin);

    all_zero = _mm_or_si128(cmp_mask0, cmp_mask1);
    if (_mm_test_all_zeros(all_zero, all_zero)) {
        _mm256_storeu_si256((__m256i *)(qcoeff_ptr), big_zero);
        _mm256_storeu_si256((__m256i *)(dqcoeff_ptr), big_zero);
#if CONFIG_VP9_HIGHBITDEPTH
        _mm256_storeu_si256((__m256i *)(qcoeff_ptr + 8), big_zero);
        _mm256_storeu_si256((__m256i *)(dqcoeff_ptr + 8), big_zero);
#endif  // CONFIG_VP9_HIGHBITDEPTH

        round = _mm_unpackhi_epi64(round, round);
        quant = _mm_unpackhi_epi64(quant, quant);
        shift = _mm_unpackhi_epi64(shift, shift);
        dequant = _mm_unpackhi_epi64(dequant, dequant);
    }
    else {
        calculate_qcoeff(&qcoeff0, round, quant, shift);
        round = _mm_unpackhi_epi64(round, round);
        quant = _mm_unpackhi_epi64(quant, quant);
        shift = _mm_unpackhi_epi64(shift, shift);
        calculate_qcoeff(&qcoeff1, round, quant, shift);

        // Reinsert signs.
        qcoeff0 = _mm_sign_epi16(qcoeff0, coeff0);
        qcoeff1 = _mm_sign_epi16(qcoeff1, coeff1);

        // Mask out zbin threshold coeffs.
        qcoeff0 = _mm_and_si128(qcoeff0, cmp_mask0);
        qcoeff1 = _mm_and_si128(qcoeff1, cmp_mask1);

        store_tran_low(qcoeff0, qcoeff_ptr);
        store_tran_low(qcoeff1, qcoeff_ptr + 8);

        // Un-sign to bias rounding like C.
        // dequant is almost always negative, so this is probably the backwards way
        // to handle the sign. However, it matches the previous assembly.
        coeff0 = _mm_abs_epi16(qcoeff0);
        coeff1 = _mm_abs_epi16(qcoeff1);

        coeff0 = calculate_dqcoeff(coeff0, dequant);
        dequant = _mm_unpackhi_epi64(dequant, dequant);
        coeff1 = calculate_dqcoeff(coeff1, dequant);

        // "Divide" by 2.
        coeff0 = _mm_srli_epi16(coeff0, 1);
        coeff1 = _mm_srli_epi16(coeff1, 1);

        coeff0 = _mm_sign_epi16(coeff0, qcoeff0);
        coeff1 = _mm_sign_epi16(coeff1, qcoeff1);

        store_tran_low(coeff0, dqcoeff_ptr);
        store_tran_low(coeff1, dqcoeff_ptr + 8);

        eob = scan_for_eob(&coeff0, &coeff1, cmp_mask0, cmp_mask1, iscan_ptr, 0,
            zero);
    }

    // AC only loop.
    for (index = 16; index < 32 * 32; index += 16) {
        coeff0 = load_tran_low(coeff_ptr + index);
        coeff1 = load_tran_low(coeff_ptr + index + 8);

        qcoeff0 = _mm_abs_epi16(coeff0);
        qcoeff1 = _mm_abs_epi16(coeff1);

        cmp_mask0 = _mm_cmpgt_epi16(qcoeff0, zbin);
        cmp_mask1 = _mm_cmpgt_epi16(qcoeff1, zbin);

        all_zero = _mm_or_si128(cmp_mask0, cmp_mask1);
        if (_mm_test_all_zeros(all_zero, all_zero)) {
            _mm256_storeu_si256((__m256i *)(qcoeff_ptr + index), big_zero);
            _mm256_storeu_si256((__m256i *)(dqcoeff_ptr + index), big_zero);
#if CONFIG_VP9_HIGHBITDEPTH
            _mm256_storeu_si256((__m256i *)(qcoeff_ptr + index + 8), big_zero);
            _mm256_storeu_si256((__m256i *)(dqcoeff_ptr + index + 8), big_zero);
#endif  // CONFIG_VP9_HIGHBITDEPTH
            continue;
        }

        calculate_qcoeff(&qcoeff0, round, quant, shift);
        calculate_qcoeff(&qcoeff1, round, quant, shift);

        qcoeff0 = _mm_sign_epi16(qcoeff0, coeff0);
        qcoeff1 = _mm_sign_epi16(qcoeff1, coeff1);

        qcoeff0 = _mm_and_si128(qcoeff0, cmp_mask0);
        qcoeff1 = _mm_and_si128(qcoeff1, cmp_mask1);

        store_tran_low(qcoeff0, qcoeff_ptr + index);
        store_tran_low(qcoeff1, qcoeff_ptr + index + 8);

        coeff0 = _mm_abs_epi16(qcoeff0);
        coeff1 = _mm_abs_epi16(qcoeff1);

        coeff0 = calculate_dqcoeff(coeff0, dequant);
        coeff1 = calculate_dqcoeff(coeff1, dequant);

        coeff0 = _mm_srli_epi16(coeff0, 1);
        coeff1 = _mm_srli_epi16(coeff1, 1);

        coeff0 = _mm_sign_epi16(coeff0, qcoeff0);
        coeff1 = _mm_sign_epi16(coeff1, qcoeff1);

        store_tran_low(coeff0, dqcoeff_ptr + index);
        store_tran_low(coeff1, dqcoeff_ptr + index + 8);

        eob0 = scan_for_eob(&coeff0, &coeff1, cmp_mask0, cmp_mask1, iscan_ptr,
            index, zero);
        eob = _mm_max_epi16(eob, eob0);
    }

    *eob_ptr = accumulate_eob(eob);
}
