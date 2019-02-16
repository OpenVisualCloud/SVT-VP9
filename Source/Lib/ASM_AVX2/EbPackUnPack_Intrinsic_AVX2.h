/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPackUnPack_AVX2_h
#define EbPackUnPack_AVX2_h

#include "EbDefinitions.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISABLE_AVX512
    void eb_enc_msb_un_pack2_d_avx512_intrin(
    uint16_t      *in16_bit_buffer,
    uint32_t       in_stride,
    uint8_t       *out8_bit_buffer,
    uint8_t       *outn_bit_buffer,
    uint32_t       out8_stride,
    uint32_t       outn_stride,
    uint32_t       width,
    uint32_t       height);
#else
    void eb_enc_msb_un_pack2_d_sse2_intrin(
        uint16_t  *in16_bit_buffer,
        uint32_t   in_stride,
        uint8_t   *out8_bit_buffer,
        uint8_t   *outn_bit_buffer,
        uint32_t   out8_stride,
        uint32_t   outn_stride,
        uint32_t   width,
        uint32_t   height);
#endif

#ifdef __cplusplus
}
#endif

#endif // EbPackUnPack_AVX2_h
