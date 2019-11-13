/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <immintrin.h> /* AVX2 */

#include "vpx_dsp_rtcd.h"
#include "mem.h"
#include "EbTranspose_AVX2.h"
#include "mem_sse2.h"

void eb_vp9_lpf_horizontal_16_avx2(unsigned char *s, int p,
                                const unsigned char *_blimit,
                                const unsigned char *_limit,
                                const unsigned char *_thresh) {
  __m128i mask, hev, flat, flat2;
  const __m128i zero = _mm_set1_epi16(0);
  const __m128i one = _mm_set1_epi8(1);
  __m128i q7p7, q6p6, q5p5, q4p4, q3p3, q2p2, q1p1, q0p0, p0q0, p1q1;
  __m128i abs_p1p0;

  const __m128i thresh =
      _mm_broadcastb_epi8(_mm_cvtsi32_si128((int)_thresh[0]));
  const __m128i limit = _mm_broadcastb_epi8(_mm_cvtsi32_si128((int)_limit[0]));
  const __m128i blimit =
      _mm_broadcastb_epi8(_mm_cvtsi32_si128((int)_blimit[0]));

  q4p4 = _mm_loadl_epi64((__m128i *)(s - 5 * p));
  q4p4 = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(q4p4), (__m64 *)(s + 4 * p)));
  q3p3 = _mm_loadl_epi64((__m128i *)(s - 4 * p));
  q3p3 = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(q3p3), (__m64 *)(s + 3 * p)));
  q2p2 = _mm_loadl_epi64((__m128i *)(s - 3 * p));
  q2p2 = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(q2p2), (__m64 *)(s + 2 * p)));
  q1p1 = _mm_loadl_epi64((__m128i *)(s - 2 * p));
  q1p1 = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(q1p1), (__m64 *)(s + 1 * p)));
  p1q1 = _mm_shuffle_epi32(q1p1, 78);
  q0p0 = _mm_loadl_epi64((__m128i *)(s - 1 * p));
  q0p0 = _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(q0p0), (__m64 *)(s - 0 * p)));
  p0q0 = _mm_shuffle_epi32(q0p0, 78);

  {
    __m128i abs_p1q1, abs_p0q0, abs_q1q0, fe, ff, work;
    abs_p1p0 =
        _mm_or_si128(_mm_subs_epu8(q1p1, q0p0), _mm_subs_epu8(q0p0, q1p1));
    abs_q1q0 = _mm_srli_si128(abs_p1p0, 8);
    fe = _mm_set1_epi8((char)0xfe);
    ff = _mm_cmpeq_epi8(abs_p1p0, abs_p1p0);
    abs_p0q0 =
        _mm_or_si128(_mm_subs_epu8(q0p0, p0q0), _mm_subs_epu8(p0q0, q0p0));
    abs_p1q1 =
        _mm_or_si128(_mm_subs_epu8(q1p1, p1q1), _mm_subs_epu8(p1q1, q1p1));
    flat = _mm_max_epu8(abs_p1p0, abs_q1q0);
    hev = _mm_subs_epu8(flat, thresh);
    hev = _mm_xor_si128(_mm_cmpeq_epi8(hev, zero), ff);

    abs_p0q0 = _mm_adds_epu8(abs_p0q0, abs_p0q0);
    abs_p1q1 = _mm_srli_epi16(_mm_and_si128(abs_p1q1, fe), 1);
    mask = _mm_subs_epu8(_mm_adds_epu8(abs_p0q0, abs_p1q1), blimit);
    mask = _mm_xor_si128(_mm_cmpeq_epi8(mask, zero), ff);
    // mask |= (abs(p0 - q0) * 2 + abs(p1 - q1) / 2  > blimit) * -1;
    mask = _mm_max_epu8(abs_p1p0, mask);
    // mask |= (abs(p1 - p0) > limit) * -1;
    // mask |= (abs(q1 - q0) > limit) * -1;

    work = _mm_max_epu8(
        _mm_or_si128(_mm_subs_epu8(q2p2, q1p1), _mm_subs_epu8(q1p1, q2p2)),
        _mm_or_si128(_mm_subs_epu8(q3p3, q2p2), _mm_subs_epu8(q2p2, q3p3)));
    mask = _mm_max_epu8(work, mask);
    mask = _mm_max_epu8(mask, _mm_srli_si128(mask, 8));
    mask = _mm_subs_epu8(mask, limit);
    mask = _mm_cmpeq_epi8(mask, zero);
  }

  // lp filter
  {
    const __m128i t4 = _mm_set1_epi8(4);
    const __m128i t3 = _mm_set1_epi8(3);
    const __m128i t80 = _mm_set1_epi8((char)0x80);
    const __m128i t1 = _mm_set1_epi16(0x1);
    __m128i qs1ps1 = _mm_xor_si128(q1p1, t80);
    __m128i qs0ps0 = _mm_xor_si128(q0p0, t80);
    __m128i qs0 = _mm_xor_si128(p0q0, t80);
    __m128i qs1 = _mm_xor_si128(p1q1, t80);
    __m128i filt;
    __m128i work_a;
    __m128i filter1, filter2;
    __m128i flat2_q6p6, flat2_q5p5, flat2_q4p4, flat2_q3p3, flat2_q2p2;
    __m128i flat2_q1p1, flat2_q0p0, flat_q2p2, flat_q1p1, flat_q0p0;

    filt = _mm_and_si128(_mm_subs_epi8(qs1ps1, qs1), hev);
    work_a = _mm_subs_epi8(qs0, qs0ps0);
    filt = _mm_adds_epi8(filt, work_a);
    filt = _mm_adds_epi8(filt, work_a);
    filt = _mm_adds_epi8(filt, work_a);
    /* (vpx_filter + 3 * (qs0 - ps0)) & mask */
    filt = _mm_and_si128(filt, mask);

    filter1 = _mm_adds_epi8(filt, t4);
    filter2 = _mm_adds_epi8(filt, t3);

    filter1 = _mm_unpacklo_epi8(zero, filter1);
    filter1 = _mm_srai_epi16(filter1, 0xB);
    filter2 = _mm_unpacklo_epi8(zero, filter2);
    filter2 = _mm_srai_epi16(filter2, 0xB);

    /* Filter1 >> 3 */
    filt = _mm_packs_epi16(filter2, _mm_subs_epi16(zero, filter1));
    qs0ps0 = _mm_xor_si128(_mm_adds_epi8(qs0ps0, filt), t80);

    /* filt >> 1 */
    filt = _mm_adds_epi16(filter1, t1);
    filt = _mm_srai_epi16(filt, 1);
    filt = _mm_andnot_si128(_mm_srai_epi16(_mm_unpacklo_epi8(zero, hev), 0x8),
                            filt);
    filt = _mm_packs_epi16(filt, _mm_subs_epi16(zero, filt));
    qs1ps1 = _mm_xor_si128(_mm_adds_epi8(qs1ps1, filt), t80);
    // loop_filter done

    {
      __m128i work;
      flat = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(q2p2, q0p0), _mm_subs_epu8(q0p0, q2p2)),
          _mm_or_si128(_mm_subs_epu8(q3p3, q0p0), _mm_subs_epu8(q0p0, q3p3)));
      flat = _mm_max_epu8(abs_p1p0, flat);
      flat = _mm_max_epu8(flat, _mm_srli_si128(flat, 8));
      flat = _mm_subs_epu8(flat, one);
      flat = _mm_cmpeq_epi8(flat, zero);
      flat = _mm_and_si128(flat, mask);

      q5p5 = _mm_loadl_epi64((__m128i *)(s - 6 * p));
      q5p5 = _mm_castps_si128(
          _mm_loadh_pi(_mm_castsi128_ps(q5p5), (__m64 *)(s + 5 * p)));

      q6p6 = _mm_loadl_epi64((__m128i *)(s - 7 * p));
      q6p6 = _mm_castps_si128(
          _mm_loadh_pi(_mm_castsi128_ps(q6p6), (__m64 *)(s + 6 * p)));

      flat2 = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(q4p4, q0p0), _mm_subs_epu8(q0p0, q4p4)),
          _mm_or_si128(_mm_subs_epu8(q5p5, q0p0), _mm_subs_epu8(q0p0, q5p5)));

      q7p7 = _mm_loadl_epi64((__m128i *)(s - 8 * p));
      q7p7 = _mm_castps_si128(
          _mm_loadh_pi(_mm_castsi128_ps(q7p7), (__m64 *)(s + 7 * p)));

      work = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(q6p6, q0p0), _mm_subs_epu8(q0p0, q6p6)),
          _mm_or_si128(_mm_subs_epu8(q7p7, q0p0), _mm_subs_epu8(q0p0, q7p7)));

      flat2 = _mm_max_epu8(work, flat2);
      flat2 = _mm_max_epu8(flat2, _mm_srli_si128(flat2, 8));
      flat2 = _mm_subs_epu8(flat2, one);
      flat2 = _mm_cmpeq_epi8(flat2, zero);
      flat2 = _mm_and_si128(flat2, flat);  // flat2 & flat & mask
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // flat and wide flat calculations
    {
      const __m128i eight = _mm_set1_epi16(8);
      const __m128i four = _mm_set1_epi16(4);
      __m128i p7_16, p6_16, p5_16, p4_16, p3_16, p2_16, p1_16, p0_16;
      __m128i q7_16, q6_16, q5_16, q4_16, q3_16, q2_16, q1_16, q0_16;
      __m128i pixelFilter_p, pixelFilter_q;
      __m128i pixetFilter_p2p1p0, pixetFilter_q2q1q0;
      __m128i sum_p7, sum_q7, sum_p3, sum_q3, res_p, res_q;

      p7_16 = _mm_unpacklo_epi8(q7p7, zero);
      p6_16 = _mm_unpacklo_epi8(q6p6, zero);
      p5_16 = _mm_unpacklo_epi8(q5p5, zero);
      p4_16 = _mm_unpacklo_epi8(q4p4, zero);
      p3_16 = _mm_unpacklo_epi8(q3p3, zero);
      p2_16 = _mm_unpacklo_epi8(q2p2, zero);
      p1_16 = _mm_unpacklo_epi8(q1p1, zero);
      p0_16 = _mm_unpacklo_epi8(q0p0, zero);
      q0_16 = _mm_unpackhi_epi8(q0p0, zero);
      q1_16 = _mm_unpackhi_epi8(q1p1, zero);
      q2_16 = _mm_unpackhi_epi8(q2p2, zero);
      q3_16 = _mm_unpackhi_epi8(q3p3, zero);
      q4_16 = _mm_unpackhi_epi8(q4p4, zero);
      q5_16 = _mm_unpackhi_epi8(q5p5, zero);
      q6_16 = _mm_unpackhi_epi8(q6p6, zero);
      q7_16 = _mm_unpackhi_epi8(q7p7, zero);

      pixelFilter_p = _mm_add_epi16(_mm_add_epi16(p6_16, p5_16),
                                    _mm_add_epi16(p4_16, p3_16));
      pixelFilter_q = _mm_add_epi16(_mm_add_epi16(q6_16, q5_16),
                                    _mm_add_epi16(q4_16, q3_16));

      pixetFilter_p2p1p0 = _mm_add_epi16(p0_16, _mm_add_epi16(p2_16, p1_16));
      pixelFilter_p = _mm_add_epi16(pixelFilter_p, pixetFilter_p2p1p0);

      pixetFilter_q2q1q0 = _mm_add_epi16(q0_16, _mm_add_epi16(q2_16, q1_16));
      pixelFilter_q = _mm_add_epi16(pixelFilter_q, pixetFilter_q2q1q0);
      pixelFilter_p =
          _mm_add_epi16(eight, _mm_add_epi16(pixelFilter_p, pixelFilter_q));
      pixetFilter_p2p1p0 = _mm_add_epi16(
          four, _mm_add_epi16(pixetFilter_p2p1p0, pixetFilter_q2q1q0));
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(p7_16, p0_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(q7_16, q0_16)), 4);
      flat2_q0p0 = _mm_packus_epi16(res_p, res_q);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(p3_16, p0_16)), 3);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(q3_16, q0_16)), 3);

      flat_q0p0 = _mm_packus_epi16(res_p, res_q);

      sum_p7 = _mm_add_epi16(p7_16, p7_16);
      sum_q7 = _mm_add_epi16(q7_16, q7_16);
      sum_p3 = _mm_add_epi16(p3_16, p3_16);
      sum_q3 = _mm_add_epi16(q3_16, q3_16);

      pixelFilter_q = _mm_sub_epi16(pixelFilter_p, p6_16);
      pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q6_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(sum_p7, p1_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_q, _mm_add_epi16(sum_q7, q1_16)), 4);
      flat2_q1p1 = _mm_packus_epi16(res_p, res_q);

      pixetFilter_q2q1q0 = _mm_sub_epi16(pixetFilter_p2p1p0, p2_16);
      pixetFilter_p2p1p0 = _mm_sub_epi16(pixetFilter_p2p1p0, q2_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(sum_p3, p1_16)), 3);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixetFilter_q2q1q0, _mm_add_epi16(sum_q3, q1_16)), 3);
      flat_q1p1 = _mm_packus_epi16(res_p, res_q);

      sum_p7 = _mm_add_epi16(sum_p7, p7_16);
      sum_q7 = _mm_add_epi16(sum_q7, q7_16);
      sum_p3 = _mm_add_epi16(sum_p3, p3_16);
      sum_q3 = _mm_add_epi16(sum_q3, q3_16);

      pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q5_16);
      pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p5_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(sum_p7, p2_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_q, _mm_add_epi16(sum_q7, q2_16)), 4);
      flat2_q2p2 = _mm_packus_epi16(res_p, res_q);

      pixetFilter_p2p1p0 = _mm_sub_epi16(pixetFilter_p2p1p0, q1_16);
      pixetFilter_q2q1q0 = _mm_sub_epi16(pixetFilter_q2q1q0, p1_16);

      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixetFilter_p2p1p0, _mm_add_epi16(sum_p3, p2_16)), 3);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixetFilter_q2q1q0, _mm_add_epi16(sum_q3, q2_16)), 3);
      flat_q2p2 = _mm_packus_epi16(res_p, res_q);

      sum_p7 = _mm_add_epi16(sum_p7, p7_16);
      sum_q7 = _mm_add_epi16(sum_q7, q7_16);
      pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q4_16);
      pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p4_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(sum_p7, p3_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_q, _mm_add_epi16(sum_q7, q3_16)), 4);
      flat2_q3p3 = _mm_packus_epi16(res_p, res_q);

      sum_p7 = _mm_add_epi16(sum_p7, p7_16);
      sum_q7 = _mm_add_epi16(sum_q7, q7_16);
      pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q3_16);
      pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p3_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(sum_p7, p4_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_q, _mm_add_epi16(sum_q7, q4_16)), 4);
      flat2_q4p4 = _mm_packus_epi16(res_p, res_q);

      sum_p7 = _mm_add_epi16(sum_p7, p7_16);
      sum_q7 = _mm_add_epi16(sum_q7, q7_16);
      pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q2_16);
      pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p2_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(sum_p7, p5_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_q, _mm_add_epi16(sum_q7, q5_16)), 4);
      flat2_q5p5 = _mm_packus_epi16(res_p, res_q);

      sum_p7 = _mm_add_epi16(sum_p7, p7_16);
      sum_q7 = _mm_add_epi16(sum_q7, q7_16);
      pixelFilter_p = _mm_sub_epi16(pixelFilter_p, q1_16);
      pixelFilter_q = _mm_sub_epi16(pixelFilter_q, p1_16);
      res_p = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_p, _mm_add_epi16(sum_p7, p6_16)), 4);
      res_q = _mm_srli_epi16(
          _mm_add_epi16(pixelFilter_q, _mm_add_epi16(sum_q7, q6_16)), 4);
      flat2_q6p6 = _mm_packus_epi16(res_p, res_q);
    }
    // wide flat
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    flat = _mm_shuffle_epi32(flat, 68);
    flat2 = _mm_shuffle_epi32(flat2, 68);

    q2p2 = _mm_andnot_si128(flat, q2p2);
    flat_q2p2 = _mm_and_si128(flat, flat_q2p2);
    q2p2 = _mm_or_si128(q2p2, flat_q2p2);

    qs1ps1 = _mm_andnot_si128(flat, qs1ps1);
    flat_q1p1 = _mm_and_si128(flat, flat_q1p1);
    q1p1 = _mm_or_si128(qs1ps1, flat_q1p1);

    qs0ps0 = _mm_andnot_si128(flat, qs0ps0);
    flat_q0p0 = _mm_and_si128(flat, flat_q0p0);
    q0p0 = _mm_or_si128(qs0ps0, flat_q0p0);

    q6p6 = _mm_andnot_si128(flat2, q6p6);
    flat2_q6p6 = _mm_and_si128(flat2, flat2_q6p6);
    q6p6 = _mm_or_si128(q6p6, flat2_q6p6);
    _mm_storel_epi64((__m128i *)(s - 7 * p), q6p6);
    _mm_storeh_pi((__m64 *)(s + 6 * p), _mm_castsi128_ps(q6p6));

    q5p5 = _mm_andnot_si128(flat2, q5p5);
    flat2_q5p5 = _mm_and_si128(flat2, flat2_q5p5);
    q5p5 = _mm_or_si128(q5p5, flat2_q5p5);
    _mm_storel_epi64((__m128i *)(s - 6 * p), q5p5);
    _mm_storeh_pi((__m64 *)(s + 5 * p), _mm_castsi128_ps(q5p5));

    q4p4 = _mm_andnot_si128(flat2, q4p4);
    flat2_q4p4 = _mm_and_si128(flat2, flat2_q4p4);
    q4p4 = _mm_or_si128(q4p4, flat2_q4p4);
    _mm_storel_epi64((__m128i *)(s - 5 * p), q4p4);
    _mm_storeh_pi((__m64 *)(s + 4 * p), _mm_castsi128_ps(q4p4));

    q3p3 = _mm_andnot_si128(flat2, q3p3);
    flat2_q3p3 = _mm_and_si128(flat2, flat2_q3p3);
    q3p3 = _mm_or_si128(q3p3, flat2_q3p3);
    _mm_storel_epi64((__m128i *)(s - 4 * p), q3p3);
    _mm_storeh_pi((__m64 *)(s + 3 * p), _mm_castsi128_ps(q3p3));

    q2p2 = _mm_andnot_si128(flat2, q2p2);
    flat2_q2p2 = _mm_and_si128(flat2, flat2_q2p2);
    q2p2 = _mm_or_si128(q2p2, flat2_q2p2);
    _mm_storel_epi64((__m128i *)(s - 3 * p), q2p2);
    _mm_storeh_pi((__m64 *)(s + 2 * p), _mm_castsi128_ps(q2p2));

    q1p1 = _mm_andnot_si128(flat2, q1p1);
    flat2_q1p1 = _mm_and_si128(flat2, flat2_q1p1);
    q1p1 = _mm_or_si128(q1p1, flat2_q1p1);
    _mm_storel_epi64((__m128i *)(s - 2 * p), q1p1);
    _mm_storeh_pi((__m64 *)(s + 1 * p), _mm_castsi128_ps(q1p1));

    q0p0 = _mm_andnot_si128(flat2, q0p0);
    flat2_q0p0 = _mm_and_si128(flat2, flat2_q0p0);
    q0p0 = _mm_or_si128(q0p0, flat2_q0p0);
    _mm_storel_epi64((__m128i *)(s - 1 * p), q0p0);
    _mm_storeh_pi((__m64 *)(s - 0 * p), _mm_castsi128_ps(q0p0));
  }
}

DECLARE_ALIGNED(32, static const uint8_t, filt_loopfilter_avx2[32]) = {
  0, 128, 1, 128, 2,  128, 3,  128, 4,  128, 5,  128, 6,  128, 7,  128,
  8, 128, 9, 128, 10, 128, 11, 128, 12, 128, 13, 128, 14, 128, 15, 128
};

static INLINE __m128i abs_diff(__m128i a, __m128i b) {
    return _mm_or_si128(_mm_subs_epu8(a, b), _mm_subs_epu8(b, a));
}

static INLINE __m256i filter_add2_sub2_avx2(const __m256i total,
    const __m256i a1, const __m256i a2,
    const __m256i s1,
    const __m256i s2) {
    __m256i x = _mm256_add_epi16(a1, total);
    x = _mm256_add_epi16(_mm256_sub_epi16(x, _mm256_add_epi16(s1, s2)), a2);
    return x;
}

static INLINE __m256i unpack_8bit_avx2(const __m128i in) {
    const __m256i mask = _mm256_load_si256((__m256i const *)filt_loopfilter_avx2);
    const __m256i d = _mm256_inserti128_si256(_mm256_castsi128_si256(in), in, 1);
    return _mm256_shuffle_epi8(d, mask);
}

static INLINE __m128i filter8_mask_avx2(const __m128i flat,
    const __m128i other_filt,
    const __m256i f) {
    const __m256i ff = _mm256_srli_epi16(f, 3);
    const __m128i f8 = _mm_packus_epi16(_mm256_extracti128_si256(ff, 0),
        _mm256_extracti128_si256(ff, 1));
    const __m128i result = _mm_and_si128(flat, f8);
    return _mm_or_si128(_mm_andnot_si128(flat, other_filt), result);
}

static INLINE __m128i filter16_mask_avx2(const __m128i flat,
    const __m128i other_filt,
    const __m256i f) {
    const __m256i ff = _mm256_srli_epi16(f, 4);
    const __m128i f16 = _mm_packus_epi16(_mm256_extracti128_si256(ff, 0),
        _mm256_extracti128_si256(ff, 1));
    const __m128i result = _mm_and_si128(flat, f16);
    return _mm_or_si128(_mm_andnot_si128(flat, other_filt), result);
}

static INLINE __m128i lpf_filter8_avx2(const __m128i o, const __m128i flat2,
    const __m256i a0, const __m256i a1,
    const __m256i s0, const __m256i s1,
    __m256i *const total) {
    *total = filter_add2_sub2_avx2(*total, a0, a1, s0, s1);
    return filter8_mask_avx2(flat2, o, *total);
}

static INLINE __m128i lpf_filter16_avx2(const __m128i o, const __m128i flat2,
    const __m256i a0, const __m256i a1,
    const __m256i s0, const __m256i s1,
    __m256i *const total) {
    *total = filter_add2_sub2_avx2(*total, a0, a1, s0, s1);
    return filter16_mask_avx2(flat2, o, *total);
}

static INLINE void lpf_horizontal_16_dual_avx2(const unsigned char *_blimit,
    const unsigned char *_limit,
    const unsigned char *_thresh,
    __m128i *const io) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i one = _mm_set1_epi8(1);
    const __m128i blimit = _mm_load_si128((const __m128i *)_blimit);
    const __m128i limit = _mm_load_si128((const __m128i *)_limit);
    const __m128i thresh = _mm_load_si128((const __m128i *)_thresh);
    __m128i mask, hev, flat, flat2;
    __m128i p[8], q[8];
    __m128i op2, op1, op0, oq0, oq1, oq2;
    __m128i max_abs_p1p0q1q0;
    __m256i p0, p1, p2, p3, p4, p5, p6, p7;
    __m256i q0, q1, q2, q3, q4, q5, q6, q7;

    p[7] = io[0];
    p[6] = io[1];
    p[5] = io[2];
    p[4] = io[3];
    p[3] = io[4];
    p[2] = io[5];
    p[1] = io[6];
    p[0] = io[7];
    q[0] = io[8];
    q[1] = io[9];
    q[2] = io[10];
    q[3] = io[11];
    q[4] = io[12];
    q[5] = io[13];
    q[6] = io[14];
    q[7] = io[15];

    p7 = unpack_8bit_avx2(p[7]);
    p6 = unpack_8bit_avx2(p[6]);
    p5 = unpack_8bit_avx2(p[5]);
    p4 = unpack_8bit_avx2(p[4]);
    p3 = unpack_8bit_avx2(p[3]);
    p2 = unpack_8bit_avx2(p[2]);
    p1 = unpack_8bit_avx2(p[1]);
    p0 = unpack_8bit_avx2(p[0]);
    q0 = unpack_8bit_avx2(q[0]);
    q1 = unpack_8bit_avx2(q[1]);
    q2 = unpack_8bit_avx2(q[2]);
    q3 = unpack_8bit_avx2(q[3]);
    q4 = unpack_8bit_avx2(q[4]);
    q5 = unpack_8bit_avx2(q[5]);
    q6 = unpack_8bit_avx2(q[6]);
    q7 = unpack_8bit_avx2(q[7]);

    {
        const __m128i abs_p1p0 = abs_diff(p[1], p[0]);
        const __m128i abs_q1q0 = abs_diff(q[1], q[0]);
        const __m128i fe = _mm_set1_epi8((char)0xfe);
        const __m128i ff = _mm_cmpeq_epi8(zero, zero);
        __m128i abs_p0q0 = abs_diff(p[0], q[0]);
        __m128i abs_p1q1 = abs_diff(p[1], q[1]);
        __m128i work;
        max_abs_p1p0q1q0 = _mm_max_epu8(abs_p1p0, abs_q1q0);

        abs_p0q0 = _mm_adds_epu8(abs_p0q0, abs_p0q0);
        abs_p1q1 = _mm_srli_epi16(_mm_and_si128(abs_p1q1, fe), 1);
        mask = _mm_subs_epu8(_mm_adds_epu8(abs_p0q0, abs_p1q1), blimit);
        mask = _mm_xor_si128(_mm_cmpeq_epi8(mask, zero), ff);
        // mask |= (abs(p0 - q0) * 2 + abs(p1 - q1) / 2  > blimit) * -1;
        mask = _mm_max_epu8(max_abs_p1p0q1q0, mask);
        // mask |= (abs(p1 - p0) > limit) * -1;
        // mask |= (abs(q1 - q0) > limit) * -1;
        work = _mm_max_epu8(abs_diff(p[2], p[1]), abs_diff(p[3], p[2]));
        mask = _mm_max_epu8(work, mask);
        work = _mm_max_epu8(abs_diff(q[2], q[1]), abs_diff(q[3], q[2]));
        mask = _mm_max_epu8(work, mask);
        mask = _mm_subs_epu8(mask, limit);
        mask = _mm_cmpeq_epi8(mask, zero);
    }

    {
        __m128i work;
        work = _mm_max_epu8(abs_diff(p[2], p[0]), abs_diff(q[2], q[0]));
        flat = _mm_max_epu8(work, max_abs_p1p0q1q0);
        work = _mm_max_epu8(abs_diff(p[3], p[0]), abs_diff(q[3], q[0]));
        flat = _mm_max_epu8(work, flat);
        work = _mm_max_epu8(abs_diff(p[4], p[0]), abs_diff(q[4], q[0]));
        flat = _mm_subs_epu8(flat, one);
        flat = _mm_cmpeq_epi8(flat, zero);
        flat = _mm_and_si128(flat, mask);
        flat2 = _mm_max_epu8(abs_diff(p[5], p[0]), abs_diff(q[5], q[0]));
        flat2 = _mm_max_epu8(work, flat2);
        work = _mm_max_epu8(abs_diff(p[6], p[0]), abs_diff(q[6], q[0]));
        flat2 = _mm_max_epu8(work, flat2);
        work = _mm_max_epu8(abs_diff(p[7], p[0]), abs_diff(q[7], q[0]));
        flat2 = _mm_max_epu8(work, flat2);
        flat2 = _mm_subs_epu8(flat2, one);
        flat2 = _mm_cmpeq_epi8(flat2, zero);
        flat2 = _mm_and_si128(flat2, flat); // flat2 & flat & mask
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // filter4
    {
        const __m128i t4 = _mm_set1_epi8(4);
        const __m128i t3 = _mm_set1_epi8(3);
        const __m128i t80 = _mm_set1_epi8((char)0x80);
        const __m128i te0 = _mm_set1_epi8((char)0xe0);
        const __m128i t1f = _mm_set1_epi8(0x1f);
        const __m128i t1 = _mm_set1_epi8(0x1);
        const __m128i t7f = _mm_set1_epi8(0x7f);
        const __m128i ff = _mm_cmpeq_epi8(t4, t4);
        __m128i filt;
        __m128i work_a;
        __m128i filter1, filter2;

        op1 = _mm_xor_si128(p[1], t80);
        op0 = _mm_xor_si128(p[0], t80);
        oq0 = _mm_xor_si128(q[0], t80);
        oq1 = _mm_xor_si128(q[1], t80);

        hev = _mm_subs_epu8(max_abs_p1p0q1q0, thresh);
        hev = _mm_xor_si128(_mm_cmpeq_epi8(hev, zero), ff);
        filt = _mm_and_si128(_mm_subs_epi8(op1, oq1), hev);

        work_a = _mm_subs_epi8(oq0, op0);
        filt = _mm_adds_epi8(filt, work_a);
        filt = _mm_adds_epi8(filt, work_a);
        filt = _mm_adds_epi8(filt, work_a);
        // (vpx_filter + 3 * (qs0 - ps0)) & mask
        filt = _mm_and_si128(filt, mask);
        filter1 = _mm_adds_epi8(filt, t4);
        filter2 = _mm_adds_epi8(filt, t3);

        // Filter1 >> 3
        work_a = _mm_cmpgt_epi8(zero, filter1);
        filter1 = _mm_srli_epi16(filter1, 3);
        work_a = _mm_and_si128(work_a, te0);
        filter1 = _mm_and_si128(filter1, t1f);
        filter1 = _mm_or_si128(filter1, work_a);
        oq0 = _mm_xor_si128(_mm_subs_epi8(oq0, filter1), t80);

        // Filter2 >> 3
        work_a = _mm_cmpgt_epi8(zero, filter2);
        filter2 = _mm_srli_epi16(filter2, 3);
        work_a = _mm_and_si128(work_a, te0);
        filter2 = _mm_and_si128(filter2, t1f);
        filter2 = _mm_or_si128(filter2, work_a);
        op0 = _mm_xor_si128(_mm_adds_epi8(op0, filter2), t80);

        // filt >> 1
        filt = _mm_adds_epi8(filter1, t1);
        work_a = _mm_cmpgt_epi8(zero, filt);
        filt = _mm_srli_epi16(filt, 1);
        work_a = _mm_and_si128(work_a, t80);
        filt = _mm_and_si128(filt, t7f);
        filt = _mm_or_si128(filt, work_a);
        filt = _mm_andnot_si128(hev, filt);
        op1 = _mm_xor_si128(_mm_adds_epi8(op1, filt), t80);
        oq1 = _mm_xor_si128(_mm_subs_epi8(oq1, filt), t80);
        // loop_filter done

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // filter8
#if 1
        {
            const __m256i four = _mm256_set1_epi16(4);
            __m256i f8;

            f8 = _mm256_add_epi16(_mm256_add_epi16(p3, four),
                _mm256_add_epi16(p3, p2));
            f8 = _mm256_add_epi16(_mm256_add_epi16(p3, f8), _mm256_add_epi16(p2, p1));
            f8 = _mm256_add_epi16(_mm256_add_epi16(p0, q0), f8);

            op2 = filter8_mask_avx2(flat, p[2], f8);
            op1 = lpf_filter8_avx2(op1, flat, q1, p1, p2, p3, &f8);
            op0 = lpf_filter8_avx2(op0, flat, q2, p0, p1, p3, &f8);
            oq0 = lpf_filter8_avx2(oq0, flat, q3, q0, p0, p3, &f8);
            oq1 = lpf_filter8_avx2(oq1, flat, q3, q1, q0, p2, &f8);
            oq2 = lpf_filter8_avx2(q[2], flat, q3, q2, q1, p1, &f8);
        }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // wide flat calculations
        {
            const __m256i eight = _mm256_set1_epi16(8);
            __m256i f;

            f = _mm256_sub_epi16(_mm256_slli_epi16(p7, 3), p7); // p7 * 7
            f = _mm256_add_epi16(_mm256_slli_epi16(p6, 1), _mm256_add_epi16(p4, f));
            f = _mm256_add_epi16(_mm256_add_epi16(p3, f), _mm256_add_epi16(p2, p1));
            f = _mm256_add_epi16(_mm256_add_epi16(p0, q0), f);
            f = _mm256_add_epi16(_mm256_add_epi16(p5, eight), f);

            io[1] = filter16_mask_avx2(flat2, p[6], f);
            io[2] = lpf_filter16_avx2(p[5], flat2, q1, p5, p6, p7, &f);
            io[3] = lpf_filter16_avx2(p[4], flat2, q2, p4, p5, p7, &f);
            io[4] = lpf_filter16_avx2(p[3], flat2, q3, p3, p4, p7, &f);
            io[5] = lpf_filter16_avx2(op2, flat2, q4, p2, p3, p7, &f);
            io[6] = lpf_filter16_avx2(op1, flat2, q5, p1, p2, p7, &f);
            io[7] = lpf_filter16_avx2(op0, flat2, q6, p0, p1, p7, &f);
            io[8] = lpf_filter16_avx2(oq0, flat2, q7, q0, p0, p7, &f);
            io[9] = lpf_filter16_avx2(oq1, flat2, q7, q1, p6, q0, &f);
            io[10] = lpf_filter16_avx2(oq2, flat2, q7, q2, p5, q1, &f);
            io[11] = lpf_filter16_avx2(q[3], flat2, q7, q3, p4, q2, &f);
            io[12] = lpf_filter16_avx2(q[4], flat2, q7, q4, p3, q3, &f);
            io[13] = lpf_filter16_avx2(q[5], flat2, q7, q5, p2, q4, &f);
            io[14] = lpf_filter16_avx2(q[6], flat2, q7, q6, p1, q5, &f);
        }
        // wide flat
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#else
    // Note: this is even slower. Keep the code so don't try the same idea.
        __m256i op2_op1, op0_oq0, oq1_oq2;
        {
            const __m256i four = _mm256_set1_epi16(4);
            const __m256i flatx = _mm256_setr_m128i(flat, flat);
            __m256i f0, f1;

            f0 = _mm256_add_epi16(_mm256_add_epi16(p3, four),
                _mm256_add_epi16(p3, p2));
            f0 = _mm256_add_epi16(_mm256_add_epi16(p3, f0), _mm256_add_epi16(p2, p1));
            f0 = _mm256_add_epi16(_mm256_add_epi16(p0, q0), f0);

            f1 = filter_add2_sub2_avx2(f0, q1, p1, p2, p3);
            op2_op1 =
                dual_filter8_mask_avx2(flatx, _mm256_setr_m128i(p[2], op1), f0, f1);
            op0_oq0 = dual_filter8_avx2(q2, p0, p1, p3, q3, q0, p0, p3, flatx,
                _mm256_setr_m128i(op0, oq0), &f1);
            oq1_oq2 = dual_filter8_avx2(q3, q1, q0, p2, q3, q2, q1, p1, flatx,
                _mm256_setr_m128i(oq1, q[2]), &f1);
        }

        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // wide flat calculations
        {
            const __m256i eight = _mm256_set1_epi16(8);
            const __m256i flat2x = _mm256_setr_m128i(flat2, flat2);
            __m256i f0, f1, xx;

            f0 = _mm256_sub_epi16(_mm256_slli_epi16(p7, 3), p7); // p7 * 7
            f0 = _mm256_add_epi16(_mm256_slli_epi16(p6, 1), _mm256_add_epi16(p4, f0));
            f0 = _mm256_add_epi16(_mm256_add_epi16(p3, f0), _mm256_add_epi16(p2, p1));
            f0 = _mm256_add_epi16(_mm256_add_epi16(p0, q0), f0);
            f0 = _mm256_add_epi16(_mm256_add_epi16(p5, eight), f0);

            f1 = filter_add2_sub2_avx2(f0, q1, p5, p6, p7);
            xx = dual_filter16_mask_avx2(flat2x, _mm256_setr_m128i(p[6], p[5]), f0,
                f1);
            io[1] = _mm256_extracti128_si256(xx, 0);
            io[2] = _mm256_extracti128_si256(xx, 1);

            xx = dual_filter16_avx2(q2, p4, p5, p7, q3, p3, p4, p7, flat2x,
                _mm256_setr_m128i(p[4], p[3]), &f1);
            io[3] = _mm256_extracti128_si256(xx, 0);
            io[4] = _mm256_extracti128_si256(xx, 1);

            xx = dual_filter16_avx2(q4, p2, p3, p7, q5, p1, p2, p7, flat2x, op2_op1,
                &f1);
            io[5] = _mm256_extracti128_si256(xx, 0);
            io[6] = _mm256_extracti128_si256(xx, 1);

            xx = dual_filter16_avx2(q6, p0, p1, p7, q7, q0, p0, p7, flat2x, op0_oq0,
                &f1);
            io[7] = _mm256_extracti128_si256(xx, 0);
            io[8] = _mm256_extracti128_si256(xx, 1);

            xx = dual_filter16_avx2(q7, q1, p6, q0, q7, q2, p5, q1, flat2x, oq1_oq2,
                &f1);
            io[9] = _mm256_extracti128_si256(xx, 0);
            io[10] = _mm256_extracti128_si256(xx, 1);

            xx = dual_filter16_avx2(q7, q3, p4, q2, q7, q4, p3, q3, flat2x,
                _mm256_setr_m128i(q[3], q[4]), &f1);
            io[11] = _mm256_extracti128_si256(xx, 0);
            io[12] = _mm256_extracti128_si256(xx, 1);

            xx = dual_filter16_avx2(q7, q5, p2, q4, q7, q6, p1, q5, flat2x,
                _mm256_setr_m128i(q[5], q[6]), &f1);
            io[13] = _mm256_extracti128_si256(xx, 0);
            io[14] = _mm256_extracti128_si256(xx, 1);
        }
        // wide flat
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#endif
    }
}
void eb_vp9_lpf_horizontal_16_dual_avx2(unsigned char *s, int p,
                                     const unsigned char *_blimit,
                                     const unsigned char *_limit,
                                     const unsigned char *_thresh) {
  __m128i mask, hev, flat, flat2;
  const __m128i zero = _mm_set1_epi16(0);
  const __m128i one = _mm_set1_epi8(1);
  __m128i p7, p6, p5;
  __m128i p4, p3, p2, p1, p0, q0, q1, q2, q3, q4;
  __m128i q5, q6, q7;
  __m256i p256_7, q256_7, p256_6, q256_6, p256_5, q256_5, p256_4, q256_4,
      p256_3, q256_3, p256_2, q256_2, p256_1, q256_1, p256_0, q256_0;

  const __m128i thresh =
      _mm_broadcastb_epi8(_mm_cvtsi32_si128((int)_thresh[0]));
  const __m128i limit = _mm_broadcastb_epi8(_mm_cvtsi32_si128((int)_limit[0]));
  const __m128i blimit =
      _mm_broadcastb_epi8(_mm_cvtsi32_si128((int)_blimit[0]));

  p256_4 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s - 5 * p)));
  p256_3 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s - 4 * p)));
  p256_2 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s - 3 * p)));
  p256_1 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s - 2 * p)));
  p256_0 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s - 1 * p)));
  q256_0 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s - 0 * p)));
  q256_1 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s + 1 * p)));
  q256_2 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s + 2 * p)));
  q256_3 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s + 3 * p)));
  q256_4 =
      _mm256_castpd_si256(_mm256_broadcast_pd((__m128d const *)(s + 4 * p)));

  p4 = _mm256_castsi256_si128(p256_4);
  p3 = _mm256_castsi256_si128(p256_3);
  p2 = _mm256_castsi256_si128(p256_2);
  p1 = _mm256_castsi256_si128(p256_1);
  p0 = _mm256_castsi256_si128(p256_0);
  q0 = _mm256_castsi256_si128(q256_0);
  q1 = _mm256_castsi256_si128(q256_1);
  q2 = _mm256_castsi256_si128(q256_2);
  q3 = _mm256_castsi256_si128(q256_3);
  q4 = _mm256_castsi256_si128(q256_4);

  {
    const __m128i abs_p1p0 =
        _mm_or_si128(_mm_subs_epu8(p1, p0), _mm_subs_epu8(p0, p1));
    const __m128i abs_q1q0 =
        _mm_or_si128(_mm_subs_epu8(q1, q0), _mm_subs_epu8(q0, q1));
    const __m128i fe = _mm_set1_epi8((char)0xfe);
    const __m128i ff = _mm_cmpeq_epi8(abs_p1p0, abs_p1p0);
    __m128i abs_p0q0 =
        _mm_or_si128(_mm_subs_epu8(p0, q0), _mm_subs_epu8(q0, p0));
    __m128i abs_p1q1 =
        _mm_or_si128(_mm_subs_epu8(p1, q1), _mm_subs_epu8(q1, p1));
    __m128i work;
    flat = _mm_max_epu8(abs_p1p0, abs_q1q0);
    hev = _mm_subs_epu8(flat, thresh);
    hev = _mm_xor_si128(_mm_cmpeq_epi8(hev, zero), ff);

    abs_p0q0 = _mm_adds_epu8(abs_p0q0, abs_p0q0);
    abs_p1q1 = _mm_srli_epi16(_mm_and_si128(abs_p1q1, fe), 1);
    mask = _mm_subs_epu8(_mm_adds_epu8(abs_p0q0, abs_p1q1), blimit);
    mask = _mm_xor_si128(_mm_cmpeq_epi8(mask, zero), ff);
    // mask |= (abs(p0 - q0) * 2 + abs(p1 - q1) / 2  > blimit) * -1;
    mask = _mm_max_epu8(flat, mask);
    // mask |= (abs(p1 - p0) > limit) * -1;
    // mask |= (abs(q1 - q0) > limit) * -1;
    work = _mm_max_epu8(
        _mm_or_si128(_mm_subs_epu8(p2, p1), _mm_subs_epu8(p1, p2)),
        _mm_or_si128(_mm_subs_epu8(p3, p2), _mm_subs_epu8(p2, p3)));
    mask = _mm_max_epu8(work, mask);
    work = _mm_max_epu8(
        _mm_or_si128(_mm_subs_epu8(q2, q1), _mm_subs_epu8(q1, q2)),
        _mm_or_si128(_mm_subs_epu8(q3, q2), _mm_subs_epu8(q2, q3)));
    mask = _mm_max_epu8(work, mask);
    mask = _mm_subs_epu8(mask, limit);
    mask = _mm_cmpeq_epi8(mask, zero);
  }

  // lp filter
  {
    const __m128i t4 = _mm_set1_epi8(4);
    const __m128i t3 = _mm_set1_epi8(3);
    const __m128i t80 = _mm_set1_epi8((char)0x80);
    const __m128i te0 = _mm_set1_epi8((char)0xe0);
    const __m128i t1f = _mm_set1_epi8(0x1f);
    const __m128i t1 = _mm_set1_epi8(0x1);
    const __m128i t7f = _mm_set1_epi8(0x7f);

    __m128i ps1 = _mm_xor_si128(p1, t80);
    __m128i ps0 = _mm_xor_si128(p0, t80);
    __m128i qs0 = _mm_xor_si128(q0, t80);
    __m128i qs1 = _mm_xor_si128(q1, t80);
    __m128i filt;
    __m128i work_a;
    __m128i filter1, filter2;
    __m128i flat2_p6, flat2_p5, flat2_p4, flat2_p3, flat2_p2, flat2_p1,
        flat2_p0, flat2_q0, flat2_q1, flat2_q2, flat2_q3, flat2_q4, flat2_q5,
        flat2_q6, flat_p2, flat_p1, flat_p0, flat_q0, flat_q1, flat_q2;

    filt = _mm_and_si128(_mm_subs_epi8(ps1, qs1), hev);
    work_a = _mm_subs_epi8(qs0, ps0);
    filt = _mm_adds_epi8(filt, work_a);
    filt = _mm_adds_epi8(filt, work_a);
    filt = _mm_adds_epi8(filt, work_a);
    /* (vpx_filter + 3 * (qs0 - ps0)) & mask */
    filt = _mm_and_si128(filt, mask);

    filter1 = _mm_adds_epi8(filt, t4);
    filter2 = _mm_adds_epi8(filt, t3);

    /* Filter1 >> 3 */
    work_a = _mm_cmpgt_epi8(zero, filter1);
    filter1 = _mm_srli_epi16(filter1, 3);
    work_a = _mm_and_si128(work_a, te0);
    filter1 = _mm_and_si128(filter1, t1f);
    filter1 = _mm_or_si128(filter1, work_a);
    qs0 = _mm_xor_si128(_mm_subs_epi8(qs0, filter1), t80);

    /* Filter2 >> 3 */
    work_a = _mm_cmpgt_epi8(zero, filter2);
    filter2 = _mm_srli_epi16(filter2, 3);
    work_a = _mm_and_si128(work_a, te0);
    filter2 = _mm_and_si128(filter2, t1f);
    filter2 = _mm_or_si128(filter2, work_a);
    ps0 = _mm_xor_si128(_mm_adds_epi8(ps0, filter2), t80);

    /* filt >> 1 */
    filt = _mm_adds_epi8(filter1, t1);
    work_a = _mm_cmpgt_epi8(zero, filt);
    filt = _mm_srli_epi16(filt, 1);
    work_a = _mm_and_si128(work_a, t80);
    filt = _mm_and_si128(filt, t7f);
    filt = _mm_or_si128(filt, work_a);
    filt = _mm_andnot_si128(hev, filt);
    ps1 = _mm_xor_si128(_mm_adds_epi8(ps1, filt), t80);
    qs1 = _mm_xor_si128(_mm_subs_epi8(qs1, filt), t80);
    // loop_filter done

    {
      __m128i work;
      work = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(p2, p0), _mm_subs_epu8(p0, p2)),
          _mm_or_si128(_mm_subs_epu8(q2, q0), _mm_subs_epu8(q0, q2)));
      flat = _mm_max_epu8(work, flat);
      work = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(p3, p0), _mm_subs_epu8(p0, p3)),
          _mm_or_si128(_mm_subs_epu8(q3, q0), _mm_subs_epu8(q0, q3)));
      flat = _mm_max_epu8(work, flat);
      work = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(p4, p0), _mm_subs_epu8(p0, p4)),
          _mm_or_si128(_mm_subs_epu8(q4, q0), _mm_subs_epu8(q0, q4)));
      flat = _mm_subs_epu8(flat, one);
      flat = _mm_cmpeq_epi8(flat, zero);
      flat = _mm_and_si128(flat, mask);

      p256_5 = _mm256_castpd_si256(
          _mm256_broadcast_pd((__m128d const *)(s - 6 * p)));
      q256_5 = _mm256_castpd_si256(
          _mm256_broadcast_pd((__m128d const *)(s + 5 * p)));
      p5 = _mm256_castsi256_si128(p256_5);
      q5 = _mm256_castsi256_si128(q256_5);
      flat2 = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(p5, p0), _mm_subs_epu8(p0, p5)),
          _mm_or_si128(_mm_subs_epu8(q5, q0), _mm_subs_epu8(q0, q5)));

      flat2 = _mm_max_epu8(work, flat2);
      p256_6 = _mm256_castpd_si256(
          _mm256_broadcast_pd((__m128d const *)(s - 7 * p)));
      q256_6 = _mm256_castpd_si256(
          _mm256_broadcast_pd((__m128d const *)(s + 6 * p)));
      p6 = _mm256_castsi256_si128(p256_6);
      q6 = _mm256_castsi256_si128(q256_6);
      work = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(p6, p0), _mm_subs_epu8(p0, p6)),
          _mm_or_si128(_mm_subs_epu8(q6, q0), _mm_subs_epu8(q0, q6)));

      flat2 = _mm_max_epu8(work, flat2);

      p256_7 = _mm256_castpd_si256(
          _mm256_broadcast_pd((__m128d const *)(s - 8 * p)));
      q256_7 = _mm256_castpd_si256(
          _mm256_broadcast_pd((__m128d const *)(s + 7 * p)));
      p7 = _mm256_castsi256_si128(p256_7);
      q7 = _mm256_castsi256_si128(q256_7);
      work = _mm_max_epu8(
          _mm_or_si128(_mm_subs_epu8(p7, p0), _mm_subs_epu8(p0, p7)),
          _mm_or_si128(_mm_subs_epu8(q7, q0), _mm_subs_epu8(q0, q7)));

      flat2 = _mm_max_epu8(work, flat2);
      flat2 = _mm_subs_epu8(flat2, one);
      flat2 = _mm_cmpeq_epi8(flat2, zero);
      flat2 = _mm_and_si128(flat2, flat);  // flat2 & flat & mask
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // flat and wide flat calculations
    {
      const __m256i eight = _mm256_set1_epi16(8);
      const __m256i four = _mm256_set1_epi16(4);
      __m256i pixelFilter_p, pixelFilter_q, pixetFilter_p2p1p0,
          pixetFilter_q2q1q0, sum_p7, sum_q7, sum_p3, sum_q3, res_p, res_q;

      const __m256i filter =
          _mm256_load_si256((__m256i const *)filt_loopfilter_avx2);
      p256_7 = _mm256_shuffle_epi8(p256_7, filter);
      p256_6 = _mm256_shuffle_epi8(p256_6, filter);
      p256_5 = _mm256_shuffle_epi8(p256_5, filter);
      p256_4 = _mm256_shuffle_epi8(p256_4, filter);
      p256_3 = _mm256_shuffle_epi8(p256_3, filter);
      p256_2 = _mm256_shuffle_epi8(p256_2, filter);
      p256_1 = _mm256_shuffle_epi8(p256_1, filter);
      p256_0 = _mm256_shuffle_epi8(p256_0, filter);
      q256_0 = _mm256_shuffle_epi8(q256_0, filter);
      q256_1 = _mm256_shuffle_epi8(q256_1, filter);
      q256_2 = _mm256_shuffle_epi8(q256_2, filter);
      q256_3 = _mm256_shuffle_epi8(q256_3, filter);
      q256_4 = _mm256_shuffle_epi8(q256_4, filter);
      q256_5 = _mm256_shuffle_epi8(q256_5, filter);
      q256_6 = _mm256_shuffle_epi8(q256_6, filter);
      q256_7 = _mm256_shuffle_epi8(q256_7, filter);

      pixelFilter_p = _mm256_add_epi16(_mm256_add_epi16(p256_6, p256_5),
                                       _mm256_add_epi16(p256_4, p256_3));
      pixelFilter_q = _mm256_add_epi16(_mm256_add_epi16(q256_6, q256_5),
                                       _mm256_add_epi16(q256_4, q256_3));

      pixetFilter_p2p1p0 =
          _mm256_add_epi16(p256_0, _mm256_add_epi16(p256_2, p256_1));
      pixelFilter_p = _mm256_add_epi16(pixelFilter_p, pixetFilter_p2p1p0);

      pixetFilter_q2q1q0 =
          _mm256_add_epi16(q256_0, _mm256_add_epi16(q256_2, q256_1));
      pixelFilter_q = _mm256_add_epi16(pixelFilter_q, pixetFilter_q2q1q0);

      pixelFilter_p = _mm256_add_epi16(
          eight, _mm256_add_epi16(pixelFilter_p, pixelFilter_q));

      pixetFilter_p2p1p0 = _mm256_add_epi16(
          four, _mm256_add_epi16(pixetFilter_p2p1p0, pixetFilter_q2q1q0));

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(p256_7, p256_0)), 4);

      flat2_p0 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(q256_7, q256_0)), 4);

      flat2_q0 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      res_p =
          _mm256_srli_epi16(_mm256_add_epi16(pixetFilter_p2p1p0,
                                             _mm256_add_epi16(p256_3, p256_0)),
                            3);

      flat_p0 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q =
          _mm256_srli_epi16(_mm256_add_epi16(pixetFilter_p2p1p0,
                                             _mm256_add_epi16(q256_3, q256_0)),
                            3);

      flat_q0 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      sum_p7 = _mm256_add_epi16(p256_7, p256_7);

      sum_q7 = _mm256_add_epi16(q256_7, q256_7);

      sum_p3 = _mm256_add_epi16(p256_3, p256_3);

      sum_q3 = _mm256_add_epi16(q256_3, q256_3);

      pixelFilter_q = _mm256_sub_epi16(pixelFilter_p, p256_6);

      pixelFilter_p = _mm256_sub_epi16(pixelFilter_p, q256_6);

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(sum_p7, p256_1)), 4);

      flat2_p1 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_q, _mm256_add_epi16(sum_q7, q256_1)), 4);

      flat2_q1 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      pixetFilter_q2q1q0 = _mm256_sub_epi16(pixetFilter_p2p1p0, p256_2);

      pixetFilter_p2p1p0 = _mm256_sub_epi16(pixetFilter_p2p1p0, q256_2);

      res_p =
          _mm256_srli_epi16(_mm256_add_epi16(pixetFilter_p2p1p0,
                                             _mm256_add_epi16(sum_p3, p256_1)),
                            3);

      flat_p1 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q =
          _mm256_srli_epi16(_mm256_add_epi16(pixetFilter_q2q1q0,
                                             _mm256_add_epi16(sum_q3, q256_1)),
                            3);

      flat_q1 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      sum_p7 = _mm256_add_epi16(sum_p7, p256_7);

      sum_q7 = _mm256_add_epi16(sum_q7, q256_7);

      sum_p3 = _mm256_add_epi16(sum_p3, p256_3);

      sum_q3 = _mm256_add_epi16(sum_q3, q256_3);

      pixelFilter_p = _mm256_sub_epi16(pixelFilter_p, q256_5);

      pixelFilter_q = _mm256_sub_epi16(pixelFilter_q, p256_5);

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(sum_p7, p256_2)), 4);

      flat2_p2 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_q, _mm256_add_epi16(sum_q7, q256_2)), 4);

      flat2_q2 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      pixetFilter_p2p1p0 = _mm256_sub_epi16(pixetFilter_p2p1p0, q256_1);

      pixetFilter_q2q1q0 = _mm256_sub_epi16(pixetFilter_q2q1q0, p256_1);

      res_p =
          _mm256_srli_epi16(_mm256_add_epi16(pixetFilter_p2p1p0,
                                             _mm256_add_epi16(sum_p3, p256_2)),
                            3);

      flat_p2 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q =
          _mm256_srli_epi16(_mm256_add_epi16(pixetFilter_q2q1q0,
                                             _mm256_add_epi16(sum_q3, q256_2)),
                            3);

      flat_q2 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      sum_p7 = _mm256_add_epi16(sum_p7, p256_7);

      sum_q7 = _mm256_add_epi16(sum_q7, q256_7);

      pixelFilter_p = _mm256_sub_epi16(pixelFilter_p, q256_4);

      pixelFilter_q = _mm256_sub_epi16(pixelFilter_q, p256_4);

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(sum_p7, p256_3)), 4);

      flat2_p3 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_q, _mm256_add_epi16(sum_q7, q256_3)), 4);

      flat2_q3 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      sum_p7 = _mm256_add_epi16(sum_p7, p256_7);

      sum_q7 = _mm256_add_epi16(sum_q7, q256_7);

      pixelFilter_p = _mm256_sub_epi16(pixelFilter_p, q256_3);

      pixelFilter_q = _mm256_sub_epi16(pixelFilter_q, p256_3);

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(sum_p7, p256_4)), 4);

      flat2_p4 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_q, _mm256_add_epi16(sum_q7, q256_4)), 4);

      flat2_q4 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      sum_p7 = _mm256_add_epi16(sum_p7, p256_7);

      sum_q7 = _mm256_add_epi16(sum_q7, q256_7);

      pixelFilter_p = _mm256_sub_epi16(pixelFilter_p, q256_2);

      pixelFilter_q = _mm256_sub_epi16(pixelFilter_q, p256_2);

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(sum_p7, p256_5)), 4);

      flat2_p5 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_q, _mm256_add_epi16(sum_q7, q256_5)), 4);

      flat2_q5 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));

      sum_p7 = _mm256_add_epi16(sum_p7, p256_7);

      sum_q7 = _mm256_add_epi16(sum_q7, q256_7);

      pixelFilter_p = _mm256_sub_epi16(pixelFilter_p, q256_1);

      pixelFilter_q = _mm256_sub_epi16(pixelFilter_q, p256_1);

      res_p = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_p, _mm256_add_epi16(sum_p7, p256_6)), 4);

      flat2_p6 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_p, res_p), 168));

      res_q = _mm256_srli_epi16(
          _mm256_add_epi16(pixelFilter_q, _mm256_add_epi16(sum_q7, q256_6)), 4);

      flat2_q6 = _mm256_castsi256_si128(
          _mm256_permute4x64_epi64(_mm256_packus_epi16(res_q, res_q), 168));
    }

    // wide flat
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    p2 = _mm_andnot_si128(flat, p2);
    flat_p2 = _mm_and_si128(flat, flat_p2);
    p2 = _mm_or_si128(flat_p2, p2);

    p1 = _mm_andnot_si128(flat, ps1);
    flat_p1 = _mm_and_si128(flat, flat_p1);
    p1 = _mm_or_si128(flat_p1, p1);

    p0 = _mm_andnot_si128(flat, ps0);
    flat_p0 = _mm_and_si128(flat, flat_p0);
    p0 = _mm_or_si128(flat_p0, p0);

    q0 = _mm_andnot_si128(flat, qs0);
    flat_q0 = _mm_and_si128(flat, flat_q0);
    q0 = _mm_or_si128(flat_q0, q0);

    q1 = _mm_andnot_si128(flat, qs1);
    flat_q1 = _mm_and_si128(flat, flat_q1);
    q1 = _mm_or_si128(flat_q1, q1);

    q2 = _mm_andnot_si128(flat, q2);
    flat_q2 = _mm_and_si128(flat, flat_q2);
    q2 = _mm_or_si128(flat_q2, q2);

    p6 = _mm_andnot_si128(flat2, p6);
    flat2_p6 = _mm_and_si128(flat2, flat2_p6);
    p6 = _mm_or_si128(flat2_p6, p6);
    _mm_storeu_si128((__m128i *)(s - 7 * p), p6);

    p5 = _mm_andnot_si128(flat2, p5);
    flat2_p5 = _mm_and_si128(flat2, flat2_p5);
    p5 = _mm_or_si128(flat2_p5, p5);
    _mm_storeu_si128((__m128i *)(s - 6 * p), p5);

    p4 = _mm_andnot_si128(flat2, p4);
    flat2_p4 = _mm_and_si128(flat2, flat2_p4);
    p4 = _mm_or_si128(flat2_p4, p4);
    _mm_storeu_si128((__m128i *)(s - 5 * p), p4);

    p3 = _mm_andnot_si128(flat2, p3);
    flat2_p3 = _mm_and_si128(flat2, flat2_p3);
    p3 = _mm_or_si128(flat2_p3, p3);
    _mm_storeu_si128((__m128i *)(s - 4 * p), p3);

    p2 = _mm_andnot_si128(flat2, p2);
    flat2_p2 = _mm_and_si128(flat2, flat2_p2);
    p2 = _mm_or_si128(flat2_p2, p2);
    _mm_storeu_si128((__m128i *)(s - 3 * p), p2);

    p1 = _mm_andnot_si128(flat2, p1);
    flat2_p1 = _mm_and_si128(flat2, flat2_p1);
    p1 = _mm_or_si128(flat2_p1, p1);
    _mm_storeu_si128((__m128i *)(s - 2 * p), p1);

    p0 = _mm_andnot_si128(flat2, p0);
    flat2_p0 = _mm_and_si128(flat2, flat2_p0);
    p0 = _mm_or_si128(flat2_p0, p0);
    _mm_storeu_si128((__m128i *)(s - 1 * p), p0);

    q0 = _mm_andnot_si128(flat2, q0);
    flat2_q0 = _mm_and_si128(flat2, flat2_q0);
    q0 = _mm_or_si128(flat2_q0, q0);
    _mm_storeu_si128((__m128i *)(s - 0 * p), q0);

    q1 = _mm_andnot_si128(flat2, q1);
    flat2_q1 = _mm_and_si128(flat2, flat2_q1);
    q1 = _mm_or_si128(flat2_q1, q1);
    _mm_storeu_si128((__m128i *)(s + 1 * p), q1);

    q2 = _mm_andnot_si128(flat2, q2);
    flat2_q2 = _mm_and_si128(flat2, flat2_q2);
    q2 = _mm_or_si128(flat2_q2, q2);
    _mm_storeu_si128((__m128i *)(s + 2 * p), q2);

    q3 = _mm_andnot_si128(flat2, q3);
    flat2_q3 = _mm_and_si128(flat2, flat2_q3);
    q3 = _mm_or_si128(flat2_q3, q3);
    _mm_storeu_si128((__m128i *)(s + 3 * p), q3);

    q4 = _mm_andnot_si128(flat2, q4);
    flat2_q4 = _mm_and_si128(flat2, flat2_q4);
    q4 = _mm_or_si128(flat2_q4, q4);
    _mm_storeu_si128((__m128i *)(s + 4 * p), q4);

    q5 = _mm_andnot_si128(flat2, q5);
    flat2_q5 = _mm_and_si128(flat2, flat2_q5);
    q5 = _mm_or_si128(flat2_q5, q5);
    _mm_storeu_si128((__m128i *)(s + 5 * p), q5);

    q6 = _mm_andnot_si128(flat2, q6);
    flat2_q6 = _mm_and_si128(flat2, flat2_q6);
    q6 = _mm_or_si128(flat2_q6, q6);
    _mm_storeu_si128((__m128i *)(s + 6 * p), q6);
  }
}
void vpx_lpf_vertical_16_dual_avx2(unsigned char *s, int p,
    const uint8_t *blimit, const uint8_t *limit,
    const uint8_t *thresh) {
    __m128i io[16];

    // Transpose 16x16
    loadu_8bit_16x16(s - 8, p, io);
    transpose_8bit_16x16_avx2(io, io);

    // Loop filtering
    lpf_horizontal_16_dual_avx2(blimit, limit, thresh, io);

    // Transpose back
    transpose_8bit_16x16_avx2(io, io);
    storeu_8bit_16x16(io, s - 8, p);
}
