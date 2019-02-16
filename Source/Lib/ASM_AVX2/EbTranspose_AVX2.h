/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTranspose_AVX2_h
#define EbTranspose_AVX2_h

#include "immintrin.h"

#include "vpx_dsp_rtcd.h"
#include "EbMemory_AVX2.h"

static INLINE void transpose_8bit_16x16_avx2(const __m128i *const in,
                                             __m128i *const out) {
  // Combine inputs. Goes from:
  // in[ 0]: 00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F
  // in[ 1]: 10 11 12 13 14 15 16 17  18 19 1A 1B 1C 1D 1E 1F
  // in[ 2]: 20 21 22 23 24 25 26 27  28 29 2A 2B 2C 2D 2E 2F
  // in[ 3]: 30 31 32 33 34 35 36 37  38 39 3A 3B 3C 3D 3E 3F
  // in[ 4]: 40 41 42 43 44 45 46 47  48 49 4A 4B 4C 4D 4E 4F
  // in[ 5]: 50 51 52 53 54 55 56 57  58 59 5A 5B 5C 5D 5E 5F
  // in[ 6]: 60 61 62 63 64 65 66 67  68 69 6A 6B 6C 6D 6E 6F
  // in[ 7]: 70 71 72 73 74 75 76 77  78 79 7A 7B 7C 7D 7E 7F
  // in[ 8]: 80 81 82 83 84 85 86 87  88 89 8A 8B 8C 8D 8E 8F
  // in[ 9]: 90 91 92 93 94 95 96 97  98 99 9A 9B 9C 9D 9E 9F
  // in[10]: A0 A1 A2 A3 A4 A5 A6 A7  A8 A9 AA AB AC AD AE AF
  // in[11]: B0 B1 B2 B3 B4 B5 B6 B7  B8 B9 BA BB BC BD BE BF
  // in[12]: C0 C1 C2 C3 C4 C5 C6 C7  C8 C9 CA CB CC CD CE CF
  // in[13]: D0 D1 D2 D3 D4 D5 D6 D7  D8 D9 DA DB DC DD DE DF
  // in[14]: E0 E1 E2 E3 E4 E5 E6 E7  E8 E9 EA EB EC ED EE EF
  // in[15]: F0 F1 F2 F3 F4 F5 F6 F7  F8 F9 FA FB FC FD FE FF
  // to:
  // a0:     00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F
  //         80 81 82 83 84 85 86 87  88 89 8A 8B 8C 8D 8E 8F
  // a1:     10 11 12 13 14 15 16 17  18 19 1A 1B 1C 1D 1E 1F
  //         90 91 92 93 94 95 96 97  98 99 9A 9B 9C 9D 9E 9F
  // a2:     20 21 22 23 24 25 26 27  28 29 2A 2B 2C 2D 2E 2F
  //         A0 A1 A2 A3 A4 A5 A6 A7  A8 A9 AA AB AC AD AE AF
  // a3:     30 31 32 33 34 35 36 37  38 39 3A 3B 3C 3D 3E 3F
  //         B0 B1 B2 B3 B4 B5 B6 B7  B8 B9 BA BB BC BD BE BF
  // a4:     40 41 42 43 44 45 46 47  48 49 4A 4B 4C 4D 4E 4F
  //         C0 C1 C2 C3 C4 C5 C6 C7  C8 C9 CA CB CC CD CE CF
  // a5:     50 51 52 53 54 55 56 57  58 59 5A 5B 5C 5D 5E 5F
  //         D0 D1 D2 D3 D4 D5 D6 D7  D8 D9 DA DB DC DD DE DF
  // a6:     60 61 62 63 64 65 66 67  68 69 6A 6B 6C 6D 6E 6F
  //         E0 E1 E2 E3 E4 E5 E6 E7  E8 E9 EA EB EC ED EE EF
  // a7:     70 71 72 73 74 75 76 77  78 79 7A 7B 7C 7D 7E 7F
  //         F0 F1 F2 F3 F4 F5 F6 F7  F8 F9 FA FB FC FD FE FF
  const __m256i a0 = _mm256_setr_m128i(in[0], in[8]);
  const __m256i a1 = _mm256_setr_m128i(in[1], in[9]);
  const __m256i a2 = _mm256_setr_m128i(in[2], in[10]);
  const __m256i a3 = _mm256_setr_m128i(in[3], in[11]);
  const __m256i a4 = _mm256_setr_m128i(in[4], in[12]);
  const __m256i a5 = _mm256_setr_m128i(in[5], in[13]);
  const __m256i a6 = _mm256_setr_m128i(in[6], in[14]);
  const __m256i a7 = _mm256_setr_m128i(in[7], in[15]);

  // Unpack 8 bit elements resulting in:
  // b0: 00 10 01 11 02 12 03 13  04 14 05 15 06 16 07 17
  //     80 90 81 91 82 92 83 93  84 94 85 95 86 96 87 97
  // b1: 08 18 09 19 0A 1A 0B 1B  0C 1C 0D 1D 0E 1E 0F 1F
  //     88 98 89 99 8A 9A 8B 9B  8C 9C 8D 9D 8E 9E 8F 9F
  // b2: 20 30 21 31 22 32 23 33  24 34 25 35 26 36 27 37
  //     A0 B0 A1 B1 A2 B2 A3 B3  A4 B4 A5 B5 A6 B6 A7 B7
  // b3: 28 38 29 39 2A 3A 2B 3B  2C 3C 2D 3D 2E 3E 2F 3F
  //     A8 B8 A9 B9 AA BA AB BB  AC BC AD BD AE BE AF BF
  // b4: 40 50 41 51 42 52 43 53  44 54 45 55 46 56 47 57
  //     C0 D0 C1 D1 C2 D2 C3 D3  C4 D4 C5 D5 C6 D6 C7 D7
  // b5: 48 58 49 59 4A 5A 4B 5B  4C 5C 4D 5D 4E 5E 4F 5F
  //     C8 D8 C9 D9 CA DA CB DB  CC DC CD DD CE DE CF DF
  // b6: 60 70 61 71 62 72 63 73  64 74 65 75 66 76 67 77
  //     E0 F0 E1 F1 E2 F2 E3 F3  E4 F4 E5 F5 E6 F6 E7 F7
  // b7: 68 78 69 79 6A 7A 6B 7B  6C 7C 6D 7D 6E 7E 6F 7F
  //     E8 F8 E9 F9 EA FA EB FB  EC FC ED FD EE FE EF FF
  const __m256i b0 = _mm256_unpacklo_epi8(a0, a1);
  const __m256i b1 = _mm256_unpackhi_epi8(a0, a1);
  const __m256i b2 = _mm256_unpacklo_epi8(a2, a3);
  const __m256i b3 = _mm256_unpackhi_epi8(a2, a3);
  const __m256i b4 = _mm256_unpacklo_epi8(a4, a5);
  const __m256i b5 = _mm256_unpackhi_epi8(a4, a5);
  const __m256i b6 = _mm256_unpacklo_epi8(a6, a7);
  const __m256i b7 = _mm256_unpackhi_epi8(a6, a7);

  // Unpack 16 bit elements resulting in:
  // c0: 00 10 20 30 01 11 21 31  02 12 22 32 03 13 23 33
  //     80 90 A0 B0 81 91 A1 B1  82 92 A2 B2 83 93 A3 B3
  // c1: 04 14 24 34 05 15 25 35  06 16 26 36 07 17 27 37
  //     84 94 A4 B4 85 95 A5 B5  86 96 A6 B6 87 97 A7 B7
  // c2: 08 18 28 38 09 19 29 39  0A 1A 2A 3A 0B 1B 2B 3B
  //     88 98 A8 B8 89 99 A9 B9  8A 9A AA BA 8B 9B AB BB
  // c3: 0C 1C 2C 3C 0D 1D 2D 3D  0E 1E 2E 3E 0F 1F 2F 3F
  //     8C 9C AC BC 8D 9D AD BD  8E 9E AE BE 8F 9F AF BF
  // c4: 40 50 60 70 41 51 61 71  42 52 62 72 43 53 63 73
  //     C0 D0 E0 F0 C1 D1 E1 F1  C2 D2 E2 F2 C3 D3 E3 F3
  // c5: 44 54 64 74 45 55 65 75  46 56 66 76 47 57 67 77
  //     C4 D4 E4 F4 C5 D5 E5 F5  C6 D6 E6 F6 C7 D7 E7 F7
  // c6: 48 58 68 78 49 59 69 79  4A 5A 6A 7A 4B 5B 6B 7B
  //     C8 D8 E8 F8 C9 D9 E9 F9  CA DA EA FA CB DB EB FB
  // c7: 4C 5C 6C 7C 4D 5D 6D 7D  4E 5E 6E 7E 4F 5F 6F 7F
  //     CC DC EC FC CD DD ED FD  CE DE EE FE CF DF EF FF
  const __m256i c0 = _mm256_unpacklo_epi16(b0, b2);
  const __m256i c1 = _mm256_unpackhi_epi16(b0, b2);
  const __m256i c2 = _mm256_unpacklo_epi16(b1, b3);
  const __m256i c3 = _mm256_unpackhi_epi16(b1, b3);
  const __m256i c4 = _mm256_unpacklo_epi16(b4, b6);
  const __m256i c5 = _mm256_unpackhi_epi16(b4, b6);
  const __m256i c6 = _mm256_unpacklo_epi16(b5, b7);
  const __m256i c7 = _mm256_unpackhi_epi16(b5, b7);

  // Unpack 32 bit elements resulting in:
  // d0: 00 10 20 30 40 50 60 70  01 11 21 31 41 51 61 71
  //     80 90 A0 B0 C0 D0 E0 F0  81 91 A1 B1 C1 D1 E1 F1
  // d1: 02 12 22 32 42 52 62 72  03 13 23 33 43 53 63 73
  //     82 92 A2 B2 C2 D2 E2 F2  83 93 A3 B3 C3 D3 E3 F3
  // d2: 04 14 24 34 44 54 64 74  05 15 25 35 45 55 65 75
  //     84 94 A4 B4 C4 D4 E4 F4  85 95 A5 B5 C5 D5 E5 F5
  // d3: 06 16 26 36 46 56 66 76  07 17 27 37 47 57 67 77
  //     86 96 A6 B6 C6 D6 E6 F6  87 97 A7 B7 C7 D7 E7 F7
  // d4: 08 18 28 38 48 58 68 78  09 19 29 39 49 59 69 79
  //     88 98 A8 B8 C8 D8 E8 F8  89 99 A9 B9 C9 D9 E9 F9
  // d5: 0A 1A 2A 3A 4A 5A 6A 7A  0B 1B 2B 3B 4B 5B 6B 7B
  //     8A 9A AA BA CA DA EA FA  8B 9B AB BB CB DB EB FB
  // d6: 0C 1C 2C 3C 4C 5C 6C 7C  0D 1D 2D 3D 4D 5D 6D 7D
  //     8C 9C AC BC CC DC EC FC  8D 9D AD BD CD DD ED FD
  // d7: 0E 1E 2E 3E 4E 5E 6E 7E  0F 1F 2F 3F 4F 5F 6F 7F
  //     8E 9E AE BE CE DE EE FE  8F 9F AF BF CF DF EF FF
  const __m256i d0 = _mm256_unpacklo_epi32(c0, c4);
  const __m256i d1 = _mm256_unpackhi_epi32(c0, c4);
  const __m256i d2 = _mm256_unpacklo_epi32(c1, c5);
  const __m256i d3 = _mm256_unpackhi_epi32(c1, c5);
  const __m256i d4 = _mm256_unpacklo_epi32(c2, c6);
  const __m256i d5 = _mm256_unpackhi_epi32(c2, c6);
  const __m256i d6 = _mm256_unpacklo_epi32(c3, c7);
  const __m256i d7 = _mm256_unpackhi_epi32(c3, c7);

  // Unpack 64 bit elements resulting in:
  // e0: 00 10 20 30 40 50 60 70  80 90 A0 B0 C0 D0 E0 F0
  //     01 11 21 31 41 51 61 71  81 91 A1 B1 C1 D1 E1 F1
  // e1: 02 12 22 32 42 52 62 72  82 92 A2 B2 C2 D2 E2 F2
  //     03 13 23 33 43 53 63 73  83 93 A3 B3 C3 D3 E3 F3
  // e2: 04 14 24 34 44 54 64 74  84 94 A4 B4 C4 D4 E4 F4
  //     05 15 25 35 45 55 65 75  85 95 A5 B5 C5 D5 E5 F5
  // e3: 06 16 26 36 46 56 66 76  86 96 A6 B6 C6 D6 E6 F6
  //     07 17 27 37 47 57 67 77  87 97 A7 B7 C7 D7 E7 F7
  // e4: 08 18 28 38 48 58 68 78  88 98 A8 B8 C8 D8 E8 F8
  //     09 19 29 39 49 59 69 79  89 99 A9 B9 C9 D9 E9 F9
  // e5: 0A 1A 2A 3A 4A 5A 6A 7A  8A 9A AA BA CA DA EA FA
  //     0B 1B 2B 3B 4B 5B 6B 7B  8B 9B AB BB CB DB EB FB
  // e6: 0C 1C 2C 3C 4C 5C 6C 7C  8C 9C AC BC CC DC EC FC
  //     0D 1D 2D 3D 4D 5D 6D 7D  8D 9D AD BD CD DD ED FD
  // e7: 0E 1E 2E 3E 4E 5E 6E 7E  8E 9E AE BE CE DE EE FE
  //     0F 1F 2F 3F 4F 5F 6F 7F  8F 9F AF BF CF DF EF FF
  const __m256i e0 = _mm256_permute4x64_epi64(d0, 0xD8);
  const __m256i e1 = _mm256_permute4x64_epi64(d1, 0xD8);
  const __m256i e2 = _mm256_permute4x64_epi64(d2, 0xD8);
  const __m256i e3 = _mm256_permute4x64_epi64(d3, 0xD8);
  const __m256i e4 = _mm256_permute4x64_epi64(d4, 0xD8);
  const __m256i e5 = _mm256_permute4x64_epi64(d5, 0xD8);
  const __m256i e6 = _mm256_permute4x64_epi64(d6, 0xD8);
  const __m256i e7 = _mm256_permute4x64_epi64(d7, 0xD8);

  // Extract 64 bit elements resulting in:
  // out[ 0]: 00 10 20 30 40 50 60 70  80 90 A0 B0 C0 D0 E0 F0
  // out[ 1]: 01 11 21 31 41 51 61 71  81 91 A1 B1 C1 D1 E1 F1
  // out[ 2]: 02 12 22 32 42 52 62 72  82 92 A2 B2 C2 D2 E2 F2
  // out[ 3]: 03 13 23 33 43 53 63 73  83 93 A3 B3 C3 D3 E3 F3
  // out[ 4]: 04 14 24 34 44 54 64 74  84 94 A4 B4 C4 D4 E4 F4
  // out[ 5]: 05 15 25 35 45 55 65 75  85 95 A5 B5 C5 D5 E5 F5
  // out[ 6]: 06 16 26 36 46 56 66 76  86 96 A6 B6 C6 D6 E6 F6
  // out[ 7]: 07 17 27 37 47 57 67 77  87 97 A7 B7 C7 D7 E7 F7
  // out[ 8]: 08 18 28 38 48 58 68 78  88 98 A8 B8 C8 D8 E8 F8
  // out[ 9]: 09 19 29 39 49 59 69 79  89 99 A9 B9 C9 D9 E9 F9
  // out[10]: 0A 1A 2A 3A 4A 5A 6A 7A  8A 9A AA BA CA DA EA FA
  // out[11]: 0B 1B 2B 3B 4B 5B 6B 7B  8B 9B AB BB CB DB EB FB
  // out[12]: 0C 1C 2C 3C 4C 5C 6C 7C  8C 9C AC BC CC DC EC FC
  // out[13]: 0D 1D 2D 3D 4D 5D 6D 7D  8D 9D AD BD CD DD ED FD
  // out[14]: 0E 1E 2E 3E 4E 5E 6E 7E  8E 9E AE BE CE DE EE FE
  // out[15]: 0F 1F 2F 3F 4F 5F 6F 7F  8F 9F AF BF CF DF EF FF
  out[0] = _mm256_extracti128_si256(e0, 0);
  out[1] = _mm256_extracti128_si256(e0, 1);
  out[2] = _mm256_extracti128_si256(e1, 0);
  out[3] = _mm256_extracti128_si256(e1, 1);
  out[4] = _mm256_extracti128_si256(e2, 0);
  out[5] = _mm256_extracti128_si256(e2, 1);
  out[6] = _mm256_extracti128_si256(e3, 0);
  out[7] = _mm256_extracti128_si256(e3, 1);
  out[8] = _mm256_extracti128_si256(e4, 0);
  out[9] = _mm256_extracti128_si256(e4, 1);
  out[10] = _mm256_extracti128_si256(e5, 0);
  out[11] = _mm256_extracti128_si256(e5, 1);
  out[12] = _mm256_extracti128_si256(e6, 0);
  out[13] = _mm256_extracti128_si256(e6, 1);
  out[14] = _mm256_extracti128_si256(e7, 0);
  out[15] = _mm256_extracti128_si256(e7, 1);
}

static INLINE void transpose_16bit_8x16_avx2(const __m128i *const in,
                                             __m256i *const out) {
  // Unpack 16 bit elements. Goes from:
  // in[ 0]: 00 01 02 03 04 05 06 07
  // in[ 1]: 10 11 12 13 14 15 16 17
  // in[ 2]: 20 21 22 23 24 25 26 27
  // in[ 3]: 30 31 32 33 34 35 36 37
  // in[ 4]: 40 41 42 43 44 45 46 47
  // in[ 5]: 50 51 52 53 54 55 56 57
  // in[ 6]: 60 61 62 63 64 65 66 67
  // in[ 7]: 70 71 72 73 74 75 76 77
  // in[ 8]: 80 81 82 83 84 85 86 87
  // in[ 9]: 90 91 92 93 94 95 96 97
  // in[10]: A0 A1 A2 A3 A4 A5 A6 A7
  // in[11]: B0 B1 B2 B3 B4 B5 B6 B7
  // in[12]: C0 C1 C2 C3 C4 C5 C6 C7
  // in[13]: D0 D1 D2 D3 D4 D5 D6 D7
  // in[14]: E0 E1 E2 E3 E4 E5 E6 E7
  // in[15]: F0 F1 F2 F3 F4 F5 F6 F7
  // to:
  // a0:   : 00 01 02 03 04 05 06 07  80 81 82 83 84 85 86 87
  // a1:   : 10 11 12 13 14 15 16 17  90 91 92 93 94 95 96 97
  // a2:   : 20 21 22 23 24 25 26 27  A0 A1 A2 A3 A4 A5 A6 A7
  // a3:   : 30 31 32 33 34 35 36 37  B0 B1 B2 B3 B4 B5 B6 B7
  // a4:   : 40 41 42 43 44 45 46 47  C0 C1 C2 C3 C4 C5 C6 C7
  // a5:   : 50 51 52 53 54 55 56 57  D0 D1 D2 D3 D4 D5 D6 D7
  // a6:   : 60 61 62 63 64 65 66 67  E0 E1 E2 E3 E4 E5 E6 E7
  // a7:   : 70 71 72 73 74 75 76 77  F0 F1 F2 F3 F4 F5 F6 F7
  const __m256i a0 = _mm256_setr_m128i(in[0], in[8]);
  const __m256i a1 = _mm256_setr_m128i(in[1], in[9]);
  const __m256i a2 = _mm256_setr_m128i(in[2], in[10]);
  const __m256i a3 = _mm256_setr_m128i(in[3], in[11]);
  const __m256i a4 = _mm256_setr_m128i(in[4], in[12]);
  const __m256i a5 = _mm256_setr_m128i(in[5], in[13]);
  const __m256i a6 = _mm256_setr_m128i(in[6], in[14]);
  const __m256i a7 = _mm256_setr_m128i(in[7], in[15]);

  // Unpack 16 bit elements. Goes from:
  // b0:     00 10 01 11 02 12 03 13  80 90 81 91 82 92 83 93
  // b1:     04 14 05 15 06 16 07 17  84 94 85 95 86 96 87 97
  // b2:     20 30 21 31 22 32 23 33  A0 B0 A1 B1 A2 B2 A3 B3
  // b3:     24 34 25 35 26 36 27 37  A4 B4 A5 B5 A6 B6 A7 B7
  // b4:     40 50 41 51 42 52 43 53  C0 D0 C1 D1 C2 D2 C3 D3
  // b5:     44 54 45 55 46 56 47 57  C4 D4 C5 D5 C6 D6 C7 D7
  // b6:     60 70 61 71 62 72 63 73  E0 F0 E1 F1 E2 F2 E3 F3
  // b7:     64 74 65 75 66 76 67 77  E4 F4 E5 F5 E6 F6 E7 F7
  const __m256i b0 = _mm256_unpacklo_epi16(a0, a1);
  const __m256i b1 = _mm256_unpackhi_epi16(a0, a1);
  const __m256i b2 = _mm256_unpacklo_epi16(a2, a3);
  const __m256i b3 = _mm256_unpackhi_epi16(a2, a3);
  const __m256i b4 = _mm256_unpacklo_epi16(a4, a5);
  const __m256i b5 = _mm256_unpackhi_epi16(a4, a5);
  const __m256i b6 = _mm256_unpacklo_epi16(a6, a7);
  const __m256i b7 = _mm256_unpackhi_epi16(a6, a7);

  // Unpack 32 bit elements resulting in:
  // b0: 00 10 20 30 01 11 21 31  80 90 A0 B0 81 91 A1 B1
  // b1: 02 12 22 32 03 13 23 33  82 92 A2 B2 83 93 A3 B3
  // b2: 04 14 24 34 05 15 25 35  84 94 A4 B4 85 95 A5 B5
  // b3: 06 16 26 36 07 17 27 37  86 96 A6 B6 87 97 A7 B7
  // b4: 40 50 60 70 41 51 61 71  C0 D0 E0 F0 C1 D1 E1 F1
  // b5: 42 52 62 72 43 53 63 73  C2 D2 E2 F2 C3 D3 E3 F3
  // b6: 44 54 64 74 45 55 65 75  C4 D4 E4 F4 C5 D5 E5 F5
  // b7: 46 56 66 76 47 57 67 77  C6 D6 E6 F6 C7 D7 E7 F7
  const __m256i c0 = _mm256_unpacklo_epi32(b0, b2);
  const __m256i c1 = _mm256_unpackhi_epi32(b0, b2);
  const __m256i c2 = _mm256_unpacklo_epi32(b1, b3);
  const __m256i c3 = _mm256_unpackhi_epi32(b1, b3);
  const __m256i c4 = _mm256_unpacklo_epi32(b4, b6);
  const __m256i c5 = _mm256_unpackhi_epi32(b4, b6);
  const __m256i c6 = _mm256_unpacklo_epi32(b5, b7);
  const __m256i c7 = _mm256_unpackhi_epi32(b5, b7);

  // Unpack 64 bit elements resulting in:
  // c0: 00 10 20 30 40 50 60 70  80 90 A0 B0 C0 D0 E0 F0
  // c1: 01 11 21 31 41 51 61 71  81 91 A1 B1 C1 D1 E1 F1
  // c2: 02 12 22 32 42 52 62 72  82 92 A2 B2 C2 D2 E2 F2
  // c3: 03 13 23 33 43 53 63 73  83 93 A3 B3 C3 D3 E3 F3
  // c4: 04 14 24 34 44 54 64 74  84 94 A4 B4 C4 D4 E4 F4
  // c5: 05 15 25 35 45 55 65 75  85 95 A5 B5 C5 D5 E5 F5
  // c6: 06 16 26 36 46 56 66 76  86 96 A6 B6 C6 D6 E6 F6
  // c7: 07 17 27 37 47 57 67 77  87 97 A7 B7 C7 D7 E7 F7
  out[0] = _mm256_unpacklo_epi64(c0, c4);
  out[1] = _mm256_unpackhi_epi64(c0, c4);
  out[2] = _mm256_unpacklo_epi64(c1, c5);
  out[3] = _mm256_unpackhi_epi64(c1, c5);
  out[4] = _mm256_unpacklo_epi64(c2, c6);
  out[5] = _mm256_unpackhi_epi64(c2, c6);
  out[6] = _mm256_unpacklo_epi64(c3, c7);
  out[7] = _mm256_unpackhi_epi64(c3, c7);
}

static INLINE void transpose_16bit_16x16_avx2(const __m256i *const in,
                                              __m256i *const out) {
  // Unpack 16 bit elements. Goes from:
  // in[ 0]: 00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F
  // in[ 1]: 10 11 12 13 14 15 16 17  18 19 1A 1B 1C 1D 1E 1F
  // in[ 2]: 20 21 22 23 24 25 26 27  28 29 2A 2B 2C 2D 2E 2F
  // in[ 3]: 30 31 32 33 34 35 36 37  38 39 3A 3B 3C 3D 3E 3F
  // in[ 4]: 40 41 42 43 44 45 46 47  48 49 4A 4B 4C 4D 4E 4F
  // in[ 5]: 50 51 52 53 54 55 56 57  58 59 5A 5B 5C 5D 5E 5F
  // in[ 6]: 60 61 62 63 64 65 66 67  68 69 6A 6B 6C 6D 6E 6F
  // in[ 7]: 70 71 72 73 74 75 76 77  78 79 7A 7B 7C 7D 7E 7F
  // in[ 8]: 80 81 82 83 84 85 86 87  88 89 8A 8B 8C 8D 8E 8F
  // in[ 9]: 90 91 92 93 94 95 96 97  98 99 9A 9B 9C 9D 9E 9F
  // in[10]: A0 A1 A2 A3 A4 A5 A6 A7  A8 A9 AA AB AC AD AE AF
  // in[11]: B0 B1 B2 B3 B4 B5 B6 B7  B8 B9 BA BB BC BD BE BF
  // in[12]: C0 C1 C2 C3 C4 C5 C6 C7  C8 C9 CA CB CC CD CE CF
  // in[13]: D0 D1 D2 D3 D4 D5 D6 D7  D8 D9 DA DB DC DD DE DF
  // in[14]: E0 E1 E2 E3 E4 E5 E6 E7  E8 E9 EA EB EC ED EE EF
  // in[15]: F0 F1 F2 F3 F4 F5 F6 F7  F8 F9 FA FB FC FD FE FF
  // to:
  // a0:     00 10 01 11 02 12 03 13  08 18 09 19 0A 1A 0B 1B
  // a1:     04 14 05 15 06 16 07 17  0C 1C 0D 1D 0E 1E 0F 1F
  // a2:     20 30 21 31 22 32 23 33  28 38 29 39 2A 3A 2B 3B
  // a3:     24 34 25 35 26 36 27 37  2C 3C 2D 3D 2E 3E 2F 3F
  // a4:     40 50 41 51 42 52 43 53  48 58 49 59 4A 5A 4B 5B
  // a5:     44 54 45 55 46 56 47 57  4C 5C 4D 5D 4E 5E 4F 5F
  // a6:     60 70 61 71 62 72 63 73  68 78 69 79 6A 7A 6B 7B
  // a7:     64 74 65 75 66 76 67 77  6C 7C 6D 7D 6E 7E 6F 7F
  // a8:     80 90 81 91 82 92 83 93  88 98 89 99 8A 9A 8B 9B
  // a9:     84 94 85 95 86 96 87 97  8C 9C 8D 9D 8E 9E 8F 9F
  // aA:     A0 B0 A1 B1 A2 B2 A3 B3  A8 B8 A9 B9 AA BA AB BB
  // aB:     A4 B4 A5 B5 A6 B6 A7 B7  AC BC AD BD AE BE AF BF
  // aC:     C0 D0 C1 D1 C2 D2 C3 D3  C8 D8 C9 D9 CA DA CB DB
  // aD:     C4 D4 C5 D5 C6 D6 C7 D7  CC DC CD DD CE DE CF DF
  // aE:     E0 F0 E1 F1 E2 F2 E3 F3  E8 F8 E9 F9 EA FA EB FB
  // aF:     E4 F4 E5 F5 E6 F6 E7 F7  EC FC ED FD EE FE EF FF
  const __m256i a0 = _mm256_unpacklo_epi16(in[0], in[1]);
  const __m256i a1 = _mm256_unpackhi_epi16(in[0], in[1]);
  const __m256i a2 = _mm256_unpacklo_epi16(in[2], in[3]);
  const __m256i a3 = _mm256_unpackhi_epi16(in[2], in[3]);
  const __m256i a4 = _mm256_unpacklo_epi16(in[4], in[5]);
  const __m256i a5 = _mm256_unpackhi_epi16(in[4], in[5]);
  const __m256i a6 = _mm256_unpacklo_epi16(in[6], in[7]);
  const __m256i a7 = _mm256_unpackhi_epi16(in[6], in[7]);
  const __m256i a8 = _mm256_unpacklo_epi16(in[8], in[9]);
  const __m256i a9 = _mm256_unpackhi_epi16(in[8], in[9]);
  const __m256i aA = _mm256_unpacklo_epi16(in[10], in[11]);
  const __m256i aB = _mm256_unpackhi_epi16(in[10], in[11]);
  const __m256i aC = _mm256_unpacklo_epi16(in[12], in[13]);
  const __m256i aD = _mm256_unpackhi_epi16(in[12], in[13]);
  const __m256i aE = _mm256_unpacklo_epi16(in[14], in[15]);
  const __m256i aF = _mm256_unpackhi_epi16(in[14], in[15]);

  // Unpack 32 bit elements resulting in:
  // b0: 00 10 20 30 01 11 21 31  08 18 28 38 09 19 29 39
  // b1: 02 12 22 32 03 13 23 33  0A 1A 2A 3A 0B 1B 2B 3B
  // b2: 04 14 24 34 05 15 25 35  0C 1C 2C 3C 0D 1D 2D 3D
  // b3: 06 16 26 36 07 17 27 37  0E 1E 2E 3E 0F 1F 2F 3F
  // b4: 40 50 60 70 41 51 61 71  48 58 68 78 49 59 69 79
  // b5: 42 52 62 72 43 53 63 73  4A 5A 6A 7A 4B 5B 6B 7B
  // b6: 44 54 64 74 45 55 65 75  4C 5C 6C 7C 4D 5D 6D 7D
  // b7: 46 56 66 76 47 57 67 77  4E 5E 6E 7E 4F 5F 6F 7F
  // b8: 80 90 A0 B0 81 91 A1 B1  88 98 A8 B8 89 99 A9 B9
  // b9: 82 92 A2 B2 83 93 A3 B3  8A 9A AA BA 8B 9B AB BB
  // bA: 84 94 A4 B4 85 95 A5 B5  8C 9C AC BC 8D 9D AD BD
  // bB: 86 96 A6 B6 87 97 A7 B7  8E 9E AE BE 8F 9F AF BF
  // bC: C0 D0 E0 F0 C1 D1 E1 F1  C8 D8 E8 F8 C9 D9 E9 F9
  // bD: C2 D2 E2 F2 C3 D3 E3 F3  CA DA EA FA CB DB EB FB
  // bE: C4 D4 E4 F4 C5 D5 E5 F5  CC DC EC FC CD DD ED FD
  // bF: C6 D6 E6 F6 C7 D7 E7 F7  CE DE EE FE CF DF EF FF
  const __m256i b0 = _mm256_unpacklo_epi32(a0, a2);
  const __m256i b1 = _mm256_unpackhi_epi32(a0, a2);
  const __m256i b2 = _mm256_unpacklo_epi32(a1, a3);
  const __m256i b3 = _mm256_unpackhi_epi32(a1, a3);
  const __m256i b4 = _mm256_unpacklo_epi32(a4, a6);
  const __m256i b5 = _mm256_unpackhi_epi32(a4, a6);
  const __m256i b6 = _mm256_unpacklo_epi32(a5, a7);
  const __m256i b7 = _mm256_unpackhi_epi32(a5, a7);
  const __m256i b8 = _mm256_unpacklo_epi32(a8, aA);
  const __m256i b9 = _mm256_unpackhi_epi32(a8, aA);
  const __m256i bA = _mm256_unpacklo_epi32(a9, aB);
  const __m256i bB = _mm256_unpackhi_epi32(a9, aB);
  const __m256i bC = _mm256_unpacklo_epi32(aC, aE);
  const __m256i bD = _mm256_unpackhi_epi32(aC, aE);
  const __m256i bE = _mm256_unpacklo_epi32(aD, aF);
  const __m256i bF = _mm256_unpackhi_epi32(aD, aF);

  // Unpack 64 bit elements resulting in:
  // c0: 00 10 20 30 40 50 60 70  08 18 28 38 48 58 68 78
  // c1: 01 11 21 31 41 51 61 71  09 19 29 39 49 59 69 79
  // c2: 02 12 22 32 42 52 62 72  0A 1A 2A 3A 4A 5A 6A 7A
  // c3: 03 13 23 33 43 53 63 73  0B 1B 2B 3B 4B 5B 6B 7B
  // c4: 04 14 24 34 44 54 64 74  0C 1C 2C 3C 4C 5C 6C 7C
  // c5: 05 15 25 35 45 55 65 75  0D 1D 2D 3D 4D 5D 6D 7D
  // c6: 06 16 26 36 46 56 66 76  0E 1E 2E 3E 4E 5E 6E 7E
  // c7: 07 17 27 37 47 57 67 77  0F 1F 2F 3F 4F 5F 6F 7F
  // c8: 80 90 A0 B0 C0 D0 E0 F0  88 98 A8 B8 C8 D8 E8 F8
  // c9: 81 91 A1 B1 C1 D1 E1 F1  89 99 A9 B9 C9 D9 E9 F9
  // cA: 82 92 A2 B2 C2 D2 E2 F2  8A 9A AA BA CA DA EA FA
  // cB: 83 93 A3 B3 C3 D3 E3 F3  8B 9B AB BB CB DB EB FB
  // cC: 84 94 A4 B4 C4 D4 E4 F4  8C 9C AC BC CC DC EC FC
  // cD: 85 95 A5 B5 C5 D5 E5 F5  8D 9D AD BD CD DD ED FD
  // cE: 86 96 A6 B6 C6 D6 E6 F6  8E 9E AE BE CE DE EE FE
  // cF: 87 97 A7 B7 C7 D7 E7 F7  8F 9F AF BF CF DF EF FF
  const __m256i c0 = _mm256_unpacklo_epi64(b0, b4);
  const __m256i c1 = _mm256_unpackhi_epi64(b0, b4);
  const __m256i c2 = _mm256_unpacklo_epi64(b1, b5);
  const __m256i c3 = _mm256_unpackhi_epi64(b1, b5);
  const __m256i c4 = _mm256_unpacklo_epi64(b2, b6);
  const __m256i c5 = _mm256_unpackhi_epi64(b2, b6);
  const __m256i c6 = _mm256_unpacklo_epi64(b3, b7);
  const __m256i c7 = _mm256_unpackhi_epi64(b3, b7);
  const __m256i c8 = _mm256_unpacklo_epi64(b8, bC);
  const __m256i c9 = _mm256_unpackhi_epi64(b8, bC);
  const __m256i cA = _mm256_unpacklo_epi64(b9, bD);
  const __m256i cB = _mm256_unpackhi_epi64(b9, bD);
  const __m256i cC = _mm256_unpacklo_epi64(bA, bE);
  const __m256i cD = _mm256_unpackhi_epi64(bA, bE);
  const __m256i cE = _mm256_unpacklo_epi64(bB, bF);
  const __m256i cF = _mm256_unpackhi_epi64(bB, bF);

  // Unpack 128 bit elements resulting in:
  // out[ 0]: 00 10 20 30 40 50 60 70  80 90 A0 B0 C0 D0 E0 F0
  // out[ 1]: 01 11 21 31 41 51 61 71  81 91 A1 B1 C1 D1 E1 F1
  // out[ 2]: 02 12 22 32 42 52 62 72  82 92 A2 B2 C2 D2 E2 F2
  // out[ 3]: 03 13 23 33 43 53 63 73  83 93 A3 B3 C3 D3 E3 F3
  // out[ 4]: 04 14 24 34 44 54 64 74  84 94 A4 B4 C4 D4 E4 F4
  // out[ 5]: 05 15 25 35 45 55 65 75  85 95 A5 B5 C5 D5 E5 F5
  // out[ 6]: 06 16 26 36 46 56 66 76  86 96 A6 B6 C6 D6 E6 F6
  // out[ 7]: 07 17 27 37 47 57 67 77  87 97 A7 B7 C7 D7 E7 F7
  // out[ 8]: 08 18 28 38 48 58 68 78  88 98 A8 B8 C8 D8 E8 F8
  // out[ 9]: 09 19 29 39 49 59 69 79  89 99 A9 B9 C9 D9 E9 F9
  // out[10]: 0A 1A 2A 3A 4A 5A 6A 7A  8A 9A AA BA CA DA EA FA
  // out[11]: 0B 1B 2B 3B 4B 5B 6B 7B  8B 9B AB BB CB DB EB FB
  // out[12]: 0C 1C 2C 3C 4C 5C 6C 7C  8C 9C AC BC CC DC EC FC
  // out[13]: 0D 1D 2D 3D 4D 5D 6D 7D  8D 9D AD BD CD DD ED FD
  // out[14]: 0E 1E 2E 3E 4E 5E 6E 7E  8E 9E AE BE CE DE EE FE
  // out[15]: 0F 1F 2F 3F 4F 5F 6F 7F  8F 9F AF BF CF DF EF FF
  out[0] = _mm256_inserti128_si256(c0, _mm256_extracti128_si256(c8, 0), 1);
  out[1] = _mm256_inserti128_si256(c1, _mm256_extracti128_si256(c9, 0), 1);
  out[2] = _mm256_inserti128_si256(c2, _mm256_extracti128_si256(cA, 0), 1);
  out[3] = _mm256_inserti128_si256(c3, _mm256_extracti128_si256(cB, 0), 1);
  out[4] = _mm256_inserti128_si256(c4, _mm256_extracti128_si256(cC, 0), 1);
  out[5] = _mm256_inserti128_si256(c5, _mm256_extracti128_si256(cD, 0), 1);
  out[6] = _mm256_inserti128_si256(c6, _mm256_extracti128_si256(cE, 0), 1);
  out[7] = _mm256_inserti128_si256(c7, _mm256_extracti128_si256(cF, 0), 1);
  out[8] = _mm256_inserti128_si256(c8, _mm256_extracti128_si256(c0, 1), 0);
  out[9] = _mm256_inserti128_si256(c9, _mm256_extracti128_si256(c1, 1), 0);
  out[10] = _mm256_inserti128_si256(cA, _mm256_extracti128_si256(c2, 1), 0);
  out[11] = _mm256_inserti128_si256(cB, _mm256_extracti128_si256(c3, 1), 0);
  out[12] = _mm256_inserti128_si256(cC, _mm256_extracti128_si256(c4, 1), 0);
  out[13] = _mm256_inserti128_si256(cD, _mm256_extracti128_si256(c5, 1), 0);
  out[14] = _mm256_inserti128_si256(cE, _mm256_extracti128_si256(c6, 1), 0);
  out[15] = _mm256_inserti128_si256(cF, _mm256_extracti128_si256(c7, 1), 0);
}

#endif  // EbTranspose_AVX2_h
