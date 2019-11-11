/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// Hassene: to rename the file as no MCP here

#ifndef EBMCP_H
#define EBMCP_H

#include "EbMcp_SSE2.h"
#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void eb_vp9_generate_padding(
    EbByte   src_pic,
    uint32_t src_stride,
    uint32_t original_src_width,
    uint32_t original_src_height,
    uint32_t padding_width,
    uint32_t padding_height);

extern void eb_vp9_generate_padding_16bit(
    EbByte   src_pic,
    uint32_t src_stride,
    uint32_t original_src_width,
    uint32_t original_src_height,
    uint32_t padding_width,
    uint32_t padding_height);

extern void eb_vp9_pad_input_picture(
    EbByte   src_pic,
    uint32_t src_stride,
    uint32_t original_src_width,
    uint32_t original_src_height,
    uint32_t pad_right,
    uint32_t pad_bottom);

#ifdef __cplusplus
}
#endif
#endif // EBMCP_H
