/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBMCP_SSE2_H
#define EBMCP_SSE2_H

#include "EbDefinitions.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************************
* Assembly Declarations
**************************************************/
extern void eb_vp9_picture_copy_kernel_sse2(EbByte src, uint32_t src_stride, EbByte dst, uint32_t dst_stride,
                                            uint32_t area_width, uint32_t area_height);

#ifdef __cplusplus
}
#endif
#endif //EBMCP_SSE2_H
