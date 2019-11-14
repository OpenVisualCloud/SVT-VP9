/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPictureOperators_SSE2.h"
#include <emmintrin.h>
#include "EbDefinitions.h"

#if 0
static __m128i _mm_loadh_epi64(__m128i x, __m128i *p)
{
  return _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(x), (double *)p));
}
#endif

// Note: maximum energy within 1 TU considered to be 2^30-e
// All functions can accumulate up to 4 TUs to stay within 32-bit unsigned range

void full_distortion_kernel_8x8_32bit_bt_sse2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
  int32_t row_count;

  __m128i sum = _mm_setzero_si128();

  row_count = 8;
  do
  {
    __m128i x0;
    __m128i y0;

    x0 = _mm_loadu_si128((__m128i *)(coeff + 0x00));
    y0 = _mm_loadu_si128((__m128i *)(recon_coeff + 0x00));
    coeff += coeff_stride;
    recon_coeff += recon_coeff_stride;

    x0 = _mm_sub_epi16(x0, y0);

    x0 = _mm_madd_epi16(x0, x0);

    sum = _mm_add_epi32(sum, x0);
  }
  while (--row_count);

  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
  sum = _mm_unpacklo_epi32(sum, sum);
  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
  _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(sum, _mm_setzero_si128()));

  (void)area_width;
  (void)area_height;
}

void full_distortion_kernel_intra_16_mxn_32_bit_bt_sse2(
    int16_t   *coeff,
    uint32_t   coeff_stride,
    int16_t   *recon_coeff,
    uint32_t   recon_coeff_stride,
    uint64_t   distortion_result[2],
    uint32_t   area_width,
    uint32_t   area_height)
{
  int32_t row_count, col_count;
  __m128i sum = _mm_setzero_si128();

  col_count = area_width;
  do
  {
    int16_t *coeff_temp = coeff;
    int16_t *reconCoeffTemp = recon_coeff;

    row_count = area_height;
    do
    {
      __m128i x0, x1;
      __m128i y0, y1;

      x0 = _mm_loadu_si128((__m128i *)(coeff_temp + 0x00));
      x1 = _mm_loadu_si128((__m128i *)(coeff_temp + 0x08));
      y0 = _mm_loadu_si128((__m128i *)(reconCoeffTemp + 0x00));
      y1 = _mm_loadu_si128((__m128i *)(reconCoeffTemp + 0x08));
      coeff_temp += coeff_stride;
      reconCoeffTemp += recon_coeff_stride;

      x0 = _mm_sub_epi16(x0, y0);
      x1 = _mm_sub_epi16(x1, y1);

      x0 = _mm_madd_epi16(x0, x0);
      x1 = _mm_madd_epi16(x1, x1);

      sum = _mm_add_epi32(sum, x0);
      sum = _mm_add_epi32(sum, x1);
    }
    while (--row_count);

    coeff += 16;
    recon_coeff += 16;
    col_count -= 16;
  }
  while (col_count > 0);

  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
  sum = _mm_unpacklo_epi32(sum, sum);
  sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, 0x4e)); // 01001110
  _mm_storeu_si128((__m128i *)distortion_result, _mm_unpacklo_epi32(sum, _mm_setzero_si128()));
}

/*******************************************************************************
                         PictureCopyKernel_INTRIN
*******************************************************************************/
void eb_vp9_picture_copy_kernel4x4_sse_intrin(
    EbByte   src,
    uint32_t src_stride,
    EbByte   dst,
    uint32_t dst_stride,
    uint32_t area_width,
    uint32_t area_height)
{
    *(uint32_t *)dst = *(uint32_t *)src;
    *(uint32_t *)(dst + dst_stride) = *(uint32_t *)(src + src_stride);
    *(uint32_t *)(dst + (dst_stride << 1)) = *(uint32_t *)(src + (src_stride << 1));
    *(uint32_t *)(dst + (dst_stride * 3)) = *(uint32_t *)(src + (src_stride * 3));

    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel8x8_sse2_intrin(
    EbByte    src,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{
    _mm_storel_epi64((__m128i*)dst, _mm_cvtsi64_si128(*(uint64_t *)src));
    _mm_storel_epi64((__m128i*)(dst + dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + src_stride)));
    _mm_storel_epi64((__m128i*)(dst + (dst_stride << 1)), _mm_cvtsi64_si128(*(uint64_t *)(src + (src_stride << 1))));
    _mm_storel_epi64((__m128i*)(dst + 3* dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + 3*src_stride)));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

    _mm_storel_epi64((__m128i*)dst, _mm_cvtsi64_si128(*(uint64_t *)src));
    _mm_storel_epi64((__m128i*)(dst + dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + src_stride)));
    _mm_storel_epi64((__m128i*)(dst + (dst_stride << 1)), _mm_cvtsi64_si128(*(uint64_t *)(src + (src_stride << 1))));
    _mm_storel_epi64((__m128i*)(dst + 3* dst_stride), _mm_cvtsi64_si128(*(uint64_t *)(src + 3*src_stride)));

    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel16x16_sse2_intrin(
    EbByte    src,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{
    _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
    _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i*)(src + (src_stride * 3))));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

       _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
    _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i*)(src + (src_stride * 3))));

       src += (src_stride << 2);
    dst += (dst_stride << 2);

    _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
    _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i*)(src + (src_stride * 3))));

    src += (src_stride << 2);
    dst += (dst_stride << 2);

       _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
    _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
    _mm_storeu_si128((__m128i*)(dst + (dst_stride * 3)), _mm_loadu_si128((__m128i*)(src + (src_stride * 3))));

       src += (src_stride << 2);
    dst += (dst_stride << 2);

    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel32x32_sse2_intrin(
    EbByte    src,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{
    uint32_t y;

    for (y = 0; y < 4; ++y){

        _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
        _mm_storeu_si128((__m128i*)(dst + 16), _mm_loadu_si128((__m128i*)(src + 16)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 16), _mm_loadu_si128((__m128i*)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 16), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i*)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 16), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 16)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);

        _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
        _mm_storeu_si128((__m128i*)(dst + 16), _mm_loadu_si128((__m128i*)(src + 16)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 16), _mm_loadu_si128((__m128i*)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 16), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i*)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 16), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 16)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
    (void)area_width;
    (void)area_height;

    return;
}

void eb_vp9_picture_copy_kernel64x64_sse2_intrin(
    EbByte    src,
    uint32_t  src_stride,
    EbByte    dst,
    uint32_t  dst_stride,
    uint32_t  area_width,
    uint32_t  area_height)
{
    uint32_t y;

    for (y = 0; y < 8; ++y){

        _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
        _mm_storeu_si128((__m128i*)(dst + 16), _mm_loadu_si128((__m128i*)(src + 16)));
        _mm_storeu_si128((__m128i*)(dst + 32), _mm_loadu_si128((__m128i*)(src + 32)));
        _mm_storeu_si128((__m128i*)(dst + 48), _mm_loadu_si128((__m128i*)(src + 48)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 16), _mm_loadu_si128((__m128i*)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 32), _mm_loadu_si128((__m128i*)(src + src_stride + 32)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 48), _mm_loadu_si128((__m128i*)(src + src_stride + 48)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 16), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 32), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 32)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 48), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 48)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i*)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 16), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 16)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 32), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 32)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 48), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 48)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);

        _mm_storeu_si128((__m128i*)dst, _mm_loadu_si128((__m128i*)src));
        _mm_storeu_si128((__m128i*)(dst + 16), _mm_loadu_si128((__m128i*)(src + 16)));
        _mm_storeu_si128((__m128i*)(dst + 32), _mm_loadu_si128((__m128i*)(src + 32)));
        _mm_storeu_si128((__m128i*)(dst + 48), _mm_loadu_si128((__m128i*)(src + 48)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride), _mm_loadu_si128((__m128i*)(src + src_stride)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 16), _mm_loadu_si128((__m128i*)(src + src_stride + 16)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 32), _mm_loadu_si128((__m128i*)(src + src_stride + 32)));
        _mm_storeu_si128((__m128i*)(dst + dst_stride + 48), _mm_loadu_si128((__m128i*)(src + src_stride + 48)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1)), _mm_loadu_si128((__m128i*)(src + (src_stride << 1))));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 16), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 16)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 32), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 32)));
        _mm_storeu_si128((__m128i*)(dst + (dst_stride << 1) + 48), _mm_loadu_si128((__m128i*)(src + (src_stride << 1) + 48)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride), _mm_loadu_si128((__m128i*)(src + 3 * src_stride)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 16), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 16)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 32), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 32)));
        _mm_storeu_si128((__m128i*)(dst + 3 * dst_stride + 48), _mm_loadu_si128((__m128i*)(src + 3 * src_stride + 48)));

        src += (src_stride << 2);
        dst += (dst_stride << 2);
    }
    (void)area_width;
    (void)area_height;

    return;
}
/*******************************************************************************
                      PictureAdditionKernel_INTRIN
*******************************************************************************/
void eb_vp9_picture_addition_kernel4x4_sse_intrin(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    uint32_t y;
    __m128i xmm0, recon_0_3;
    xmm0 = _mm_setzero_si128();

    for (y = 0; y < 4; ++y){

        recon_0_3 = _mm_packus_epi16(_mm_add_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)pred_ptr), xmm0), _mm_loadl_epi64((__m128i *)residual_ptr)), xmm0);

        *(uint32_t *)recon_ptr = _mm_cvtsi128_si32(recon_0_3);
        pred_ptr += pred_stride;
        residual_ptr += residual_stride;
        recon_ptr += recon_stride;
    }
    (void)width;
    (void)height;

    return;
}

void eb_vp9_picture_addition_kernel8x8_sse2_intrin(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{

    __m128i recon_0_7, xmm0;
    uint32_t y;

    xmm0 = _mm_setzero_si128();

    for (y = 0; y < 8; ++y){

        recon_0_7 = _mm_packus_epi16(_mm_add_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)pred_ptr), xmm0), _mm_loadu_si128((__m128i *)residual_ptr)), xmm0);

        *(uint64_t *)recon_ptr = _mm_cvtsi128_si64(recon_0_7);
        pred_ptr += pred_stride;
        residual_ptr += residual_stride;
        recon_ptr += recon_stride;
    }
    (void)width;
    (void)height;

    return;
}

void eb_vp9_picture_addition_kernel16x16_sse2_intrin(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    __m128i xmm0, xmm_clip_U8, pred_0_15, recon_0_7, recon_8_15;
    uint32_t y;

    xmm0 = _mm_setzero_si128();

    for (y = 0; y < 16; ++y){

        pred_0_15 = _mm_loadu_si128((__m128i *)pred_ptr);
        recon_0_7 = _mm_add_epi16(_mm_unpacklo_epi8(pred_0_15, xmm0), _mm_loadu_si128((__m128i *)residual_ptr));
        recon_8_15 = _mm_add_epi16(_mm_unpackhi_epi8(pred_0_15, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 8)));
        xmm_clip_U8 = _mm_packus_epi16(recon_0_7, recon_8_15);

        _mm_storeu_si128((__m128i*)recon_ptr, xmm_clip_U8);

        pred_ptr += pred_stride;
        residual_ptr += residual_stride;
        recon_ptr += recon_stride;
    }
    (void)width;
    (void)height;

    return;

}
void eb_vp9_picture_addition_kernel32x32_sse2_intrin(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    uint32_t y;
    __m128i xmm0, pred_0_15, pred_16_31, recon_0_15_clipped, recon_0_7, recon_8_15, recon_16_23, recon_24_31, recon_16_31_clipped;
    xmm0 = _mm_setzero_si128();

    for (y = 0; y < 32; ++y){
        pred_0_15 = _mm_loadu_si128((__m128i *)pred_ptr);
        pred_16_31 = _mm_loadu_si128((__m128i *)(pred_ptr + 16));

        recon_0_7 = _mm_add_epi16(_mm_unpacklo_epi8(pred_0_15, xmm0), _mm_loadu_si128((__m128i *)residual_ptr));
        recon_8_15 = _mm_add_epi16(_mm_unpackhi_epi8(pred_0_15, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 8)));
        recon_16_23 = _mm_add_epi16(_mm_unpacklo_epi8(pred_16_31, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 16)));
        recon_24_31 = _mm_add_epi16(_mm_unpackhi_epi8(pred_16_31, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 24)));

        recon_0_15_clipped = _mm_packus_epi16(recon_0_7, recon_8_15);
        recon_16_31_clipped = _mm_packus_epi16(recon_16_23, recon_24_31);

        _mm_storeu_si128((__m128i*)recon_ptr, recon_0_15_clipped);
        _mm_storeu_si128((__m128i*)(recon_ptr + 16), recon_16_31_clipped);

        pred_ptr += pred_stride;
        residual_ptr += residual_stride;
        recon_ptr += recon_stride;
    }
    (void)width;
    (void)height;

    return;
}

void eb_vp9_picture_addition_kernel64x64_sse2_intrin(
    uint8_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint8_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    uint32_t y;

    __m128i xmm0, pred_0_15, pred_16_31, pred_32_47, pred_48_63;
    __m128i recon_0_15_clipped, recon_16_31_clipped, recon_32_47_clipped, recon_48_63_clipped;
    __m128i recon_0_7, recon_8_15, recon_16_23, recon_24_31, recon_32_39, recon_40_47, recon_48_55, recon_56_63;

    xmm0 = _mm_setzero_si128();

    for (y = 0; y < 64; ++y){

        pred_0_15 = _mm_loadu_si128((__m128i *)pred_ptr);
        pred_16_31 = _mm_loadu_si128((__m128i *)(pred_ptr + 16));
        pred_32_47 = _mm_loadu_si128((__m128i *)(pred_ptr + 32));
        pred_48_63 = _mm_loadu_si128((__m128i *)(pred_ptr + 48));

        recon_0_7 = _mm_add_epi16(_mm_unpacklo_epi8(pred_0_15, xmm0), _mm_loadu_si128((__m128i *)residual_ptr));
        recon_8_15 = _mm_add_epi16(_mm_unpackhi_epi8(pred_0_15, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 8)));
        recon_16_23 = _mm_add_epi16(_mm_unpacklo_epi8(pred_16_31, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 16)));
        recon_24_31 = _mm_add_epi16(_mm_unpackhi_epi8(pred_16_31, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 24)));
        recon_32_39 = _mm_add_epi16(_mm_unpacklo_epi8(pred_32_47, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 32)));
        recon_40_47 = _mm_add_epi16(_mm_unpackhi_epi8(pred_32_47, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 40)));
        recon_48_55 = _mm_add_epi16(_mm_unpacklo_epi8(pred_48_63, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 48)));
        recon_56_63 = _mm_add_epi16(_mm_unpackhi_epi8(pred_48_63, xmm0), _mm_loadu_si128((__m128i *)(residual_ptr + 56)));

        recon_0_15_clipped = _mm_packus_epi16(recon_0_7, recon_8_15);
        recon_16_31_clipped = _mm_packus_epi16(recon_16_23, recon_24_31);
        recon_32_47_clipped = _mm_packus_epi16(recon_32_39, recon_40_47);
        recon_48_63_clipped = _mm_packus_epi16(recon_48_55, recon_56_63);

        _mm_storeu_si128((__m128i*)recon_ptr, recon_0_15_clipped);
        _mm_storeu_si128((__m128i*)(recon_ptr + 16), recon_16_31_clipped);
        _mm_storeu_si128((__m128i*)(recon_ptr + 32), recon_32_47_clipped);
        _mm_storeu_si128((__m128i*)(recon_ptr + 48), recon_48_63_clipped);

        pred_ptr += pred_stride;
        residual_ptr += residual_stride;
        recon_ptr += recon_stride;
    }
    (void)width;
    (void)height;

    return;
}

/******************************************************************************************************
eb_vp9_residual_kernel
***********************************************************************************************************/
void eb_vp9_residual_kernel_sub_sampled4x4_sse_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height,
    uint8_t    last_line)
{
    __m128i residual_0_3, xmm0 = _mm_setzero_si128();
    uint32_t y;
    //hard code subampling dimensions, keep residual_stride
    area_height>>=1;
    input_stride<<=1;
    pred_stride<<=1;

    for (y = 0; y < area_height; ++y){

        residual_0_3 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)input), xmm0),
                                     _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)pred), xmm0));

        *(uint64_t *)residual = _mm_cvtsi128_si64(residual_0_3);

        residual += residual_stride;
        *(uint64_t *)residual = _mm_cvtsi128_si64(residual_0_3);

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
    (void)area_width;
    //compute the last line

    if(last_line){
    input -= (input_stride)>>1;
    pred  -= (pred_stride )>>1;
    residual -= residual_stride;
    residual_0_3 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)input), xmm0),
                                 _mm_unpacklo_epi8(_mm_cvtsi32_si128(*(uint32_t *)pred), xmm0));

    *(uint64_t *)residual = _mm_cvtsi128_si64(residual_0_3);
    }

    return;
}

void eb_vp9_residual_kernel_sub_sampled8x8_sse2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height,
    uint8_t    last_line

    )
{
    __m128i xmm0, residual_0_7;
    uint32_t y;

    xmm0 = _mm_setzero_si128();
    //hard code subampling dimensions, keep residual_stride
    area_height>>=1;
    input_stride<<=1;
    pred_stride<<=1;

    for (y = 0; y < area_height; ++y){

        residual_0_7 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)pred), xmm0));

        _mm_storeu_si128((__m128i*)residual, residual_0_7);

        residual += residual_stride;
        _mm_storeu_si128((__m128i*)residual, residual_0_7);

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
    (void)area_width;
    //compute the last line
    if(last_line){

    input -= (input_stride)>>1;
    pred  -= (pred_stride )>>1;
    residual -= residual_stride;

    residual_0_7 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)pred), xmm0));

    _mm_storeu_si128((__m128i*)residual, residual_0_7);

    }

    return;
}

void eb_vp9_residual_kernel_sub_sampled16x16_sse2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height,
    uint8_t    last_line

    )
{
    __m128i xmm0, residual_0_7, residual_8_15;
    uint32_t y;

    xmm0 = _mm_setzero_si128();
    //hard code subampling dimensions, keep residual_stride
    area_height>>=1;
    input_stride<<=1;
    pred_stride<<=1;

    for (y = 0; y < area_height; ++y){

        residual_0_7 = _mm_sub_epi16( _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_8_15 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));

        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);

        residual += residual_stride;
        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
    (void)area_width;
    //compute the last line

    if(last_line){

    input -= (input_stride)>>1;
    pred  -= (pred_stride )>>1;
    residual -= residual_stride;

    residual_0_7 = _mm_sub_epi16( _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
    residual_8_15 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));

    _mm_storeu_si128((__m128i*)residual, residual_0_7);
    _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);

    }
    return;
}

void eb_vp9_residual_kernel_sub_sampled32x32_sse2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height,
    uint8_t    last_line)
{
    __m128i xmm0, residual_0_7, residual_8_15, residual_16_23, residual_24_31;
    uint32_t y;

    xmm0 = _mm_setzero_si128();

    //hard code subampling dimensions, keep residual_stride
    area_height>>=1;
    input_stride<<=1;
    pred_stride<<=1;

    for (y = 0; y < area_height; ++y){

        residual_0_7 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_8_15 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_16_23 = _mm_sub_epi16( _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));
        residual_24_31 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));

        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);
        _mm_storeu_si128((__m128i*)(residual + 16), residual_16_23);
        _mm_storeu_si128((__m128i*)(residual + 24), residual_24_31);

         residual += residual_stride;
        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);
        _mm_storeu_si128((__m128i*)(residual + 16), residual_16_23);
        _mm_storeu_si128((__m128i*)(residual + 24), residual_24_31);

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
    (void)area_width;
        //compute the last line

    if(last_line){
        input -= (input_stride)>>1;
        pred  -= (pred_stride )>>1;
        residual -= residual_stride;

        residual_0_7 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_8_15 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_16_23 = _mm_sub_epi16( _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));
        residual_24_31 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));

        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);
        _mm_storeu_si128((__m128i*)(residual + 16), residual_16_23);
        _mm_storeu_si128((__m128i*)(residual + 24), residual_24_31);
    }

    return;
}

void eb_vp9_residual_kernel_sub_sampled64x64_sse2_intrin(
    uint8_t   *input,
    uint32_t   input_stride,
    uint8_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height,
    uint8_t    last_line)
{
    __m128i xmm0, residual_0_7, residual_8_15, residual_16_23, residual_24_31, resdiaul_32_39, residual_40_47, residual_48_55, residual_56_63;
    uint32_t y;

    xmm0 = _mm_setzero_si128();

    //hard code subampling dimensions, keep residual_stride
    area_height>>=1;
    input_stride<<=1;
    pred_stride<<=1;

    for (y = 0; y < area_height; ++y){

        residual_0_7 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_8_15 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_16_23 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));
        residual_24_31 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));
        resdiaul_32_39 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 32)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 32)), xmm0));
        residual_40_47 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 32)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 32)), xmm0));
        residual_48_55 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 48)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 48)), xmm0));
        residual_56_63 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 48)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 48)), xmm0));

        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);
        _mm_storeu_si128((__m128i*)(residual + 16), residual_16_23);
        _mm_storeu_si128((__m128i*)(residual + 24), residual_24_31);
        _mm_storeu_si128((__m128i*)(residual + 32), resdiaul_32_39);
        _mm_storeu_si128((__m128i*)(residual + 40), residual_40_47);
        _mm_storeu_si128((__m128i*)(residual + 48), residual_48_55);
        _mm_storeu_si128((__m128i*)(residual + 56), residual_56_63);

//duplicate top field residual to bottom field
         residual += residual_stride;
        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);
        _mm_storeu_si128((__m128i*)(residual + 16), residual_16_23);
        _mm_storeu_si128((__m128i*)(residual + 24), residual_24_31);
        _mm_storeu_si128((__m128i*)(residual + 32), resdiaul_32_39);
        _mm_storeu_si128((__m128i*)(residual + 40), residual_40_47);
        _mm_storeu_si128((__m128i*)(residual + 48), residual_48_55);
        _mm_storeu_si128((__m128i*)(residual + 56), residual_56_63);

        input += input_stride;
        pred += pred_stride;
        residual += residual_stride;
    }
    (void)area_width;
        //compute the last line

    if(last_line){
        input -= (input_stride)>>1;
        pred  -= (pred_stride )>>1;
        residual -= residual_stride;

        residual_0_7 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_8_15 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)input), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)pred), xmm0));
        residual_16_23 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));
        residual_24_31 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 16)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 16)), xmm0));
        resdiaul_32_39 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 32)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 32)), xmm0));
        residual_40_47 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 32)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 32)), xmm0));
        residual_48_55 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(input + 48)), xmm0), _mm_unpacklo_epi8(_mm_loadu_si128((__m128i *)(pred + 48)), xmm0));
        residual_56_63 = _mm_sub_epi16(_mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(input + 48)), xmm0), _mm_unpackhi_epi8(_mm_loadu_si128((__m128i *)(pred + 48)), xmm0));

        _mm_storeu_si128((__m128i*)residual, residual_0_7);
        _mm_storeu_si128((__m128i*)(residual + 8), residual_8_15);
        _mm_storeu_si128((__m128i*)(residual + 16), residual_16_23);
        _mm_storeu_si128((__m128i*)(residual + 24), residual_24_31);
        _mm_storeu_si128((__m128i*)(residual + 32), resdiaul_32_39);
        _mm_storeu_si128((__m128i*)(residual + 40), residual_40_47);
        _mm_storeu_si128((__m128i*)(residual + 48), residual_48_55);
        _mm_storeu_si128((__m128i*)(residual + 56), residual_56_63);

    }

    return;
}
/******************************************************************************************************
                                       eb_vp9_residual_kernel16bit_sse2_intrin
******************************************************************************************************/
void eb_vp9_residual_kernel16bit_sse2_intrin(
    uint16_t   *input,
    uint32_t   input_stride,
    uint16_t   *pred,
    uint32_t   pred_stride,
    int16_t   *residual,
    uint32_t   residual_stride,
    uint32_t   area_width,
    uint32_t   area_height)
{
    uint32_t x, y;
    __m128i residual0, residual1;

    if (area_width == 4)
    {
        for (y = 0; y < area_height; y += 2){

            residual0 = _mm_sub_epi16(_mm_loadl_epi64((__m128i*)input), _mm_loadl_epi64((__m128i*)pred));
            residual1 = _mm_sub_epi16(_mm_loadl_epi64((__m128i*)(input + input_stride)), _mm_loadl_epi64((__m128i*)(pred +  pred_stride)));

            _mm_storel_epi64((__m128i*)residual, residual0);
            _mm_storel_epi64((__m128i*)(residual + residual_stride), residual1);

            input += input_stride << 1;
            pred += pred_stride << 1;
            residual += residual_stride << 1;
        }
    }
    else if (area_width == 8){
        for (y = 0; y < area_height; y += 2){

            residual0 = _mm_sub_epi16(_mm_loadu_si128((__m128i*)input), _mm_loadu_si128((__m128i*)pred));
            residual1 = _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride)), _mm_loadu_si128((__m128i*)(pred + pred_stride)));

            _mm_storeu_si128((__m128i*) residual, residual0);
            _mm_storeu_si128((__m128i*) (residual + residual_stride), residual1);

            input += input_stride << 1;
            pred += pred_stride << 1;
            residual += residual_stride << 1;
        }
    }
    else if(area_width == 16){

        __m128i residual2, residual3;

        for (y = 0; y < area_height; y += 2){

            residual0 = _mm_sub_epi16(_mm_loadu_si128((__m128i*)input), _mm_loadu_si128((__m128i*)pred));
            residual1 = _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 8)), _mm_loadu_si128((__m128i*)(pred + 8)));
            residual2 = _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input+input_stride)), _mm_loadu_si128((__m128i*)(pred+pred_stride)));
            residual3 = _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride+8)), _mm_loadu_si128((__m128i*)(pred + pred_stride+8)));

            _mm_storeu_si128((__m128i*)residual, residual0);
            _mm_storeu_si128((__m128i*)(residual + 8), residual1);
            _mm_storeu_si128((__m128i*)(residual+residual_stride), residual2);
            _mm_storeu_si128((__m128i*)(residual +residual_stride+ 8), residual3);

            input += input_stride << 1;
            pred += pred_stride << 1;
            residual += residual_stride << 1;
        }
    }
    else if(area_width == 32){

        for (y = 0; y < area_height; y += 2){
            //residual[column_index] = ((int16_t)input[column_index]) - ((int16_t)pred[column_index]);
            _mm_storeu_si128((__m128i*) residual, _mm_sub_epi16(_mm_loadu_si128((__m128i*)input), _mm_loadu_si128((__m128i*)pred)));
            _mm_storeu_si128((__m128i*) (residual + 8), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 8)), _mm_loadu_si128((__m128i*)(pred + 8))));
            _mm_storeu_si128((__m128i*) (residual + 16), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 16)), _mm_loadu_si128((__m128i*)(pred + 16))));
            _mm_storeu_si128((__m128i*) (residual + 24), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 24)), _mm_loadu_si128((__m128i*)(pred + 24))));

            _mm_storeu_si128((__m128i*) (residual + residual_stride), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input+input_stride)), _mm_loadu_si128((__m128i*)(pred+pred_stride))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 8), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 8)), _mm_loadu_si128((__m128i*)(pred+pred_stride + 8))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 16), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 16)), _mm_loadu_si128((__m128i*)(pred + pred_stride+ 16))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 24), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 24)), _mm_loadu_si128((__m128i*)(pred + pred_stride+ 24))));

            input += input_stride << 1;
            pred += pred_stride << 1;
            residual += residual_stride << 1;
        }
    }
    else if(area_width == 64){ // Branch was not tested because the encoder had max tu_size of 32

        for (y = 0; y < area_height; y += 2){

            //residual[column_index] = ((int16_t)input[column_index]) - ((int16_t)pred[column_index]) 8 indices per _mm_sub_epi16
            _mm_storeu_si128((__m128i*) residual,  _mm_sub_epi16(_mm_loadu_si128((__m128i*)input), _mm_loadu_si128((__m128i*)pred)));
            _mm_storeu_si128((__m128i*) (residual + 8), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 8)), _mm_loadu_si128((__m128i*)(pred + 8))));
            _mm_storeu_si128((__m128i*) (residual + 16), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 16)), _mm_loadu_si128((__m128i*)(pred + 16))));
            _mm_storeu_si128((__m128i*) (residual + 24), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 24)), _mm_loadu_si128((__m128i*)(pred + 24))));
            _mm_storeu_si128((__m128i*) (residual + 32), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 32)), _mm_loadu_si128((__m128i*)(pred + 32))));
            _mm_storeu_si128((__m128i*) (residual + 40), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 40)), _mm_loadu_si128((__m128i*)(pred + 40))));
            _mm_storeu_si128((__m128i*) (residual + 48), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 48)), _mm_loadu_si128((__m128i*)(pred + 48))));
            _mm_storeu_si128((__m128i*) (residual + 56), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + 56)), _mm_loadu_si128((__m128i*)(pred + 56))));

            _mm_storeu_si128((__m128i*) (residual + residual_stride), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride)), _mm_loadu_si128((__m128i*)(pred + pred_stride))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 8), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 8)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 8))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 16), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 16)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 16))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 24), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 24)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 24))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 32), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 32)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 32))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 40), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 40)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 40))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 48), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 48)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 48))));
            _mm_storeu_si128((__m128i*) (residual + residual_stride + 56), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride + 56)), _mm_loadu_si128((__m128i*)(pred + pred_stride + 56))));

            input += input_stride << 1;
            pred += pred_stride << 1;
            residual += residual_stride << 1;
        }
    }
    else {

        uint32_t input_stride_diff = 2 * input_stride;
        uint32_t pred_stride_diff = 2 * pred_stride;
        uint32_t residual_stride_diff = 2 * residual_stride;
        input_stride_diff -= area_width;
        pred_stride_diff -= area_width;
        residual_stride_diff -= area_width;

        if (!(area_width & 7)){

            for (x = 0; x < area_height; x += 2){
                for (y = 0; y < area_width; y += 8){

                    _mm_storeu_si128((__m128i*) residual, _mm_sub_epi16(_mm_loadu_si128((__m128i*)input), _mm_loadu_si128((__m128i*)pred)));
                    _mm_storeu_si128((__m128i*) (residual + residual_stride), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride)), _mm_loadu_si128((__m128i*)(pred + pred_stride))));

                    input += 8;
                    pred += 8;
                    residual += 8;
                }
                input = input + input_stride_diff;
                pred = pred + pred_stride_diff;
                residual = residual + residual_stride_diff;
            }
        }
        else{
            for (x = 0; x < area_height; x += 2){
                for (y = 0; y < area_width; y += 4){

                    _mm_storel_epi64((__m128i*) residual, _mm_sub_epi16(_mm_loadu_si128((__m128i*)input), _mm_loadu_si128((__m128i*)pred)));
                    _mm_storel_epi64((__m128i*) (residual + residual_stride), _mm_sub_epi16(_mm_loadu_si128((__m128i*)(input + input_stride)), _mm_loadu_si128((__m128i*)(pred + pred_stride))));

                    input += 4;
                    pred += 4;
                    residual += 4;
                }
                input = input + input_stride_diff;
                pred = pred + pred_stride_diff;
                residual = residual + residual_stride_diff;
            }
        }
    }
    return;
}

/******************************************************************************************************
                                   eb_vp9_picture_addition_kernel16bit_sse2_intrin
******************************************************************************************************/

void eb_vp9_picture_addition_kernel16bit_sse2_intrin(
    uint16_t  *pred_ptr,
    uint32_t  pred_stride,
    int16_t  *residual_ptr,
    uint32_t  residual_stride,
    uint16_t  *recon_ptr,
    uint32_t  recon_stride,
    uint32_t  width,
    uint32_t  height)
{
    __m128i xmm_0, xmm_Max10bit;

    uint32_t y, x;

    xmm_0 = _mm_setzero_si128();
    xmm_Max10bit = _mm_set1_epi16(1023);

    if (width == 4)
    {
        __m128i xmm_sum_0_3, xmm_sum_s0_s3, xmm_clip3_0_3, xmm_clip3_s0_s3;
        for (y = 0; y < height; y += 2){

            xmm_sum_0_3 = _mm_adds_epi16(_mm_loadl_epi64((__m128i*)pred_ptr), _mm_loadl_epi64((__m128i*)residual_ptr));
            xmm_sum_s0_s3 = _mm_adds_epi16(_mm_loadl_epi64((__m128i*)(pred_ptr + pred_stride)), _mm_loadl_epi64((__m128i*)(residual_ptr + residual_stride)));

            xmm_clip3_0_3 = _mm_max_epi16(_mm_min_epi16(xmm_sum_0_3, xmm_Max10bit), xmm_0);
            xmm_clip3_s0_s3 = _mm_max_epi16(_mm_min_epi16(xmm_sum_s0_s3, xmm_Max10bit), xmm_0);

            _mm_storel_epi64((__m128i*) recon_ptr, xmm_clip3_0_3);
            _mm_storel_epi64((__m128i*) (recon_ptr + recon_stride), xmm_clip3_s0_s3);

            pred_ptr += pred_stride << 1;
            residual_ptr += residual_stride << 1;
            recon_ptr += recon_stride << 1;
        }
    }
    else if (width == 8){

        __m128i xmm_sum_0_7, xmm_sum_s0_s7, xmm_clip3_0_7, xmm_clip3_s0_s7;

        for (y = 0; y < height; y += 2){

            xmm_sum_0_7 = _mm_adds_epi16( _mm_loadu_si128((__m128i*)pred_ptr),_mm_loadu_si128((__m128i*)residual_ptr));
            xmm_sum_s0_s7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride)));

            xmm_clip3_0_7 = _mm_max_epi16(_mm_min_epi16(xmm_sum_0_7, xmm_Max10bit), xmm_0);
            xmm_clip3_s0_s7 = _mm_max_epi16(_mm_min_epi16(xmm_sum_s0_s7, xmm_Max10bit), xmm_0);

            _mm_storeu_si128((__m128i*) recon_ptr, xmm_clip3_0_7);
            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride), xmm_clip3_s0_s7);

            pred_ptr += pred_stride << 1;
            residual_ptr += residual_stride << 1;
            recon_ptr += recon_stride << 1;
        }
    }
    else if (width == 16){

        __m128i sum_0_7, sum_8_15, sum_s0_s7, sum_s8_s15, clip3_0_7, clip3_8_15, clip3_s0_s7, clip3_s8_s15;

        for (y = 0; y < height; y += 2){

            sum_0_7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)pred_ptr), _mm_loadu_si128((__m128i*)residual_ptr));
            sum_8_15 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 8)), _mm_loadu_si128((__m128i*)(residual_ptr + 8)));
            sum_s0_s7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride)));
            sum_s8_s15 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride + 8)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride + 8)));

            clip3_0_7 = _mm_max_epi16(_mm_min_epi16(sum_0_7, xmm_Max10bit), xmm_0);
            clip3_8_15 = _mm_max_epi16(_mm_min_epi16(sum_8_15, xmm_Max10bit), xmm_0);
            clip3_s0_s7 = _mm_max_epi16(_mm_min_epi16(sum_s0_s7, xmm_Max10bit), xmm_0);
            clip3_s8_s15 = _mm_max_epi16(_mm_min_epi16(sum_s8_s15, xmm_Max10bit), xmm_0);

            _mm_storeu_si128((__m128i*) recon_ptr, clip3_0_7);
            _mm_storeu_si128((__m128i*) (recon_ptr + 8), clip3_8_15);
            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride), clip3_s0_s7);
            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride + 8), clip3_s8_s15);

            pred_ptr += pred_stride << 1;
            residual_ptr += residual_stride << 1;
            recon_ptr += recon_stride << 1;
        }
    }
    else if (width == 32){
        __m128i sum_0_7, sum_8_15, sum_16_23, sum_24_31, sum_s0_s7, sum_s8_s15, sum_s16_s23, sum_s24_s31;
        __m128i clip3_0_7, clip3_8_15, clip3_16_23, clip3_24_31, clip3_s0_s7, clip3_s8_s15, clip3_s16_s23, clip3_s24_s31;

        for (y = 0; y < height; y += 2){

            sum_0_7   = _mm_adds_epi16(_mm_loadu_si128((__m128i*)pred_ptr), _mm_loadu_si128((__m128i*)residual_ptr));
            sum_8_15  = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 8)), _mm_loadu_si128((__m128i*)(residual_ptr + 8)));
            sum_16_23 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 16)), _mm_loadu_si128((__m128i*)(residual_ptr + 16)));
            sum_24_31 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 24)), _mm_loadu_si128((__m128i*)(residual_ptr + 24)));

            sum_s0_s7   = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride)));
            sum_s8_s15  = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride + 8)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride + 8)));
            sum_s16_s23 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride + 16)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride + 16)));
            sum_s24_s31 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride + 24)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride + 24)));

            clip3_0_7   = _mm_max_epi16(_mm_min_epi16(sum_0_7, xmm_Max10bit), xmm_0);
            clip3_8_15  = _mm_max_epi16(_mm_min_epi16(sum_8_15 , xmm_Max10bit), xmm_0);
            clip3_16_23 = _mm_max_epi16(_mm_min_epi16(sum_16_23, xmm_Max10bit), xmm_0);
            clip3_24_31 = _mm_max_epi16(_mm_min_epi16(sum_24_31, xmm_Max10bit), xmm_0);

            clip3_s0_s7   = _mm_max_epi16(_mm_min_epi16(sum_s0_s7, xmm_Max10bit), xmm_0);
            clip3_s8_s15  = _mm_max_epi16(_mm_min_epi16(sum_s8_s15, xmm_Max10bit), xmm_0);
            clip3_s16_s23 = _mm_max_epi16(_mm_min_epi16(sum_s16_s23, xmm_Max10bit), xmm_0);
            clip3_s24_s31 = _mm_max_epi16(_mm_min_epi16(sum_s24_s31, xmm_Max10bit), xmm_0);

            _mm_storeu_si128((__m128i*) recon_ptr,        clip3_0_7);
            _mm_storeu_si128((__m128i*) (recon_ptr + 8),  clip3_8_15);
            _mm_storeu_si128((__m128i*) (recon_ptr + 16), clip3_16_23);
            _mm_storeu_si128((__m128i*) (recon_ptr + 24), clip3_24_31);

            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride),      clip3_s0_s7);
            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride + 8),  clip3_s8_s15);
            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride + 16), clip3_s16_s23);
            _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride + 24), clip3_s24_s31);

            pred_ptr += pred_stride << 1;
            residual_ptr += residual_stride << 1;
            recon_ptr += recon_stride << 1;
        }
    }
    else if (width == 64){ // Branch not tested due to Max TU size is 32 at time of development

        __m128i sum_0_7, sum_8_15, sum_16_23, sum_24_31, sum_32_39, sum_40_47, sum_48_55, sum_56_63;
        __m128i clip3_0_7, clip3_8_15, clip3_16_23, clip3_24_31, clip3_32_39, clip3_40_47, clip3_48_55, clip3_56_63;

        for (y = 0; y < height; ++y ){

            sum_0_7   = _mm_adds_epi16(_mm_loadu_si128((__m128i*)pred_ptr), _mm_loadu_si128((__m128i*)residual_ptr));
            sum_8_15  = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 8)), _mm_loadu_si128((__m128i*)(residual_ptr + 8)));
            sum_16_23 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 16)), _mm_loadu_si128((__m128i*)(residual_ptr + 16)));
            sum_24_31 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 24)), _mm_loadu_si128((__m128i*)(residual_ptr + 24)));
            sum_32_39 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 32)), _mm_loadu_si128((__m128i*)(residual_ptr + 32)));
            sum_40_47 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 40)), _mm_loadu_si128((__m128i*)(residual_ptr + 40)));
            sum_48_55 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 48)), _mm_loadu_si128((__m128i*)(residual_ptr + 48)));
            sum_56_63 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + 56)), _mm_loadu_si128((__m128i*)(residual_ptr + 56)));

            clip3_0_7   = _mm_max_epi16(_mm_min_epi16(sum_0_7  , xmm_Max10bit), xmm_0);
            clip3_8_15  = _mm_max_epi16(_mm_min_epi16(sum_8_15 , xmm_Max10bit), xmm_0);
            clip3_16_23 = _mm_max_epi16(_mm_min_epi16(sum_16_23, xmm_Max10bit), xmm_0);
            clip3_24_31 = _mm_max_epi16(_mm_min_epi16(sum_24_31, xmm_Max10bit), xmm_0);
            clip3_32_39 = _mm_max_epi16(_mm_min_epi16(sum_32_39, xmm_Max10bit), xmm_0);
            clip3_40_47 = _mm_max_epi16(_mm_min_epi16(sum_40_47, xmm_Max10bit), xmm_0);
            clip3_48_55 = _mm_max_epi16(_mm_min_epi16(sum_48_55, xmm_Max10bit), xmm_0);
            clip3_56_63 = _mm_max_epi16(_mm_min_epi16(sum_56_63, xmm_Max10bit), xmm_0);

            _mm_storeu_si128((__m128i*) recon_ptr,        clip3_0_7  );
            _mm_storeu_si128((__m128i*) (recon_ptr + 8),  clip3_8_15 );
            _mm_storeu_si128((__m128i*) (recon_ptr + 16), clip3_16_23);
            _mm_storeu_si128((__m128i*) (recon_ptr + 24), clip3_24_31);
            _mm_storeu_si128((__m128i*) (recon_ptr + 32), clip3_32_39);
            _mm_storeu_si128((__m128i*) (recon_ptr + 40), clip3_40_47);
            _mm_storeu_si128((__m128i*) (recon_ptr + 48), clip3_48_55);
            _mm_storeu_si128((__m128i*) (recon_ptr + 56), clip3_56_63);

            pred_ptr += pred_stride ;
            residual_ptr += residual_stride ;
            recon_ptr += recon_stride ;
        }
    }
    else
    {
        uint32_t pred_stride_diff = 2 * pred_stride;
        uint32_t residual_stride_diff = 2 * residual_stride;
        uint32_t recon_stride_diff = 2 * recon_stride;
        pred_stride_diff -= width;
        residual_stride_diff -= width;
        recon_stride_diff -= width;

        if (!(width & 7)){

            __m128i xmm_sum_0_7, xmm_sum_s0_s7, xmm_clip3_0_7, xmm_clip3_s0_s7;

            for (x = 0; x < height; x += 2){
                for (y = 0; y < width; y += 8){

                    xmm_sum_0_7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)pred_ptr), _mm_loadu_si128((__m128i*)residual_ptr));
                    xmm_sum_s0_s7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride )));

                    xmm_clip3_0_7 = _mm_max_epi16(_mm_min_epi16(xmm_sum_0_7, xmm_Max10bit), xmm_0);
                    xmm_clip3_s0_s7 = _mm_max_epi16(_mm_min_epi16(xmm_sum_s0_s7, xmm_Max10bit), xmm_0);

                    _mm_storeu_si128((__m128i*) recon_ptr, xmm_clip3_0_7);
                    _mm_storeu_si128((__m128i*) (recon_ptr + recon_stride), xmm_clip3_s0_s7);

                    pred_ptr += 8;
                    residual_ptr += 8;
                    recon_ptr += 8;
                }
                pred_ptr += pred_stride_diff;
                residual_ptr +=  residual_stride_diff;
                recon_ptr +=  recon_stride_diff;
            }
        }
        else{
            __m128i xmm_sum_0_7, xmm_sum_s0_s7,  xmm_clip3_0_3, xmm_clip3_s0_s3;
            for (x = 0; x < height; x += 2){
                for (y = 0; y < width; y += 4){

                    xmm_sum_0_7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)pred_ptr), _mm_loadu_si128((__m128i*)residual_ptr));
                    xmm_sum_s0_s7 = _mm_adds_epi16(_mm_loadu_si128((__m128i*)(pred_ptr + pred_stride)), _mm_loadu_si128((__m128i*)(residual_ptr + residual_stride)));

                    xmm_clip3_0_3 = _mm_max_epi16(_mm_min_epi16(xmm_sum_0_7, xmm_Max10bit), xmm_0);
                    xmm_clip3_s0_s3 = _mm_max_epi16(_mm_min_epi16(xmm_sum_s0_s7, xmm_Max10bit), xmm_0);

                    _mm_storel_epi64((__m128i*) recon_ptr, xmm_clip3_0_3);
                    _mm_storel_epi64((__m128i*) (recon_ptr + recon_stride), xmm_clip3_s0_s3);

                    pred_ptr += 4;
                    residual_ptr += 4;
                    recon_ptr += 4;
                }
                pred_ptr +=  pred_stride_diff;
                residual_ptr +=  residual_stride_diff;
                recon_ptr += recon_stride_diff;
            }
        }
    }
    return;
}
