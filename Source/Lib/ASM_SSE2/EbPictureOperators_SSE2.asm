;
; Copyright(c) 2019 Intel Corporation
; SPDX - License - Identifier: BSD - 2 - Clause - Patent
;

%include "x64inc.asm"
section .text
; ----------------------------------------------------------------------------------------

cglobal eb_vp9_picture_copy_kernel_sse2

; Requirement: areaWidthInBytes = 4, 8, 12, 16, 24, 32, 48, 64 or 128
; Requirement: area_height % 2 = 0

%define src              r0
%define srcStrideInBytes r1
%define dst              r2
%define dstStrideInBytes r3
%define areaWidthInBytes r4
%define area_height       r5

    GET_PARAM_5UXD
    GET_PARAM_6UXD
    XMM_SAVE

    cmp             areaWidthInBytes,        16
    jg              Label_PictureCopyKernel_SSE2_WIDTH_Big
    je              Label_PictureCopyKernel_SSE2_WIDTH16

    cmp             areaWidthInBytes,        4
    je              Label_PictureCopyKernel_SSE2_WIDTH4

    cmp             areaWidthInBytes,        8
    je              Label_PictureCopyKernel_SSE2_WIDTH8

Label_PictureCopyKernel_SSE2_WIDTH12:
    movq            xmm0,           [src]
    movd            xmm1,           [src+8]
    movq            xmm2,           [src+srcStrideInBytes]
    movd            xmm3,           [src+srcStrideInBytes+8]
    lea             src,            [src+2*srcStrideInBytes]
    movq            [dst],          xmm0
    movd            [dst+8],        xmm1
    movq            [dst+dstStrideInBytes], xmm2
    movd            [dst+dstStrideInBytes+8], xmm3
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH12

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH8:
    movq            xmm0,           [src]
    movq            xmm1,           [src+srcStrideInBytes]
    lea             src,            [src+2*srcStrideInBytes]
    movq            [dst],          xmm0
    movq            [dst+dstStrideInBytes], xmm1
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH8

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH4:
    movd            xmm0,           [src]
    movd            xmm1,           [src+srcStrideInBytes]
    lea             src,            [src+2*srcStrideInBytes]
    movd            [dst],          xmm0
    movd            [dst+dstStrideInBytes], xmm1
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH4

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH_Big:
    cmp             areaWidthInBytes,        24
    je              Label_PictureCopyKernel_SSE2_WIDTH24

    cmp             areaWidthInBytes,        32
    je              Label_PictureCopyKernel_SSE2_WIDTH32

    cmp             areaWidthInBytes,        48
    je              Label_PictureCopyKernel_SSE2_WIDTH48

    cmp             areaWidthInBytes,        64
    je              Label_PictureCopyKernel_SSE2_WIDTH64

Label_PictureCopyKernel_SSE2_WIDTH128:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+32]
    movdqu          xmm3,           [src+48]
    movdqu          xmm4,           [src+64]
    movdqu          xmm5,           [src+80]
    movdqu          xmm6,           [src+96]
    movdqu          xmm7,           [src+112]
    lea             src,            [src+srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+48],       xmm3
    movdqu          [dst+64],       xmm4
    movdqu          [dst+80],       xmm5
    movdqu          [dst+96],       xmm6
    movdqu          [dst+112],      xmm7
    lea             dst,            [dst+dstStrideInBytes]
    sub             area_height,     1
    jne             Label_PictureCopyKernel_SSE2_WIDTH128

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH64:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+32]
    movdqu          xmm3,           [src+48]
    movdqu          xmm4,           [src+srcStrideInBytes]
    movdqu          xmm5,           [src+srcStrideInBytes+16]
    movdqu          xmm6,           [src+srcStrideInBytes+32]
    movdqu          xmm7,           [src+srcStrideInBytes+48]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+48],       xmm3
    movdqu          [dst+dstStrideInBytes],    xmm4
    movdqu          [dst+dstStrideInBytes+16], xmm5
    movdqu          [dst+dstStrideInBytes+32], xmm6
    movdqu          [dst+dstStrideInBytes+48], xmm7
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH64

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH48:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+32]
    movdqu          xmm3,           [src+srcStrideInBytes]
    movdqu          xmm4,           [src+srcStrideInBytes+16]
    movdqu          xmm5,           [src+srcStrideInBytes+32]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+dstStrideInBytes], xmm3
    movdqu          [dst+dstStrideInBytes+16], xmm4
    movdqu          [dst+dstStrideInBytes+32], xmm5
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH48

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH32:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+srcStrideInBytes]
    movdqu          xmm3,           [src+srcStrideInBytes+16]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+dstStrideInBytes], xmm2
    movdqu          [dst+dstStrideInBytes+16], xmm3
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH32

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH24:
    movdqu          xmm0,           [src]
    movq            xmm1,           [src+16]
    movdqu          xmm2,           [src+srcStrideInBytes]
    movq            xmm3,           [src+srcStrideInBytes+16]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movq            [dst+16],       xmm1
    movdqu          [dst+dstStrideInBytes], xmm2
    movq            [dst+dstStrideInBytes+16], xmm3
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH24

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH16:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+srcStrideInBytes]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+dstStrideInBytes], xmm1
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             area_height,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH16

    XMM_RESTORE
    ret

; ----------------------------------------------------------------------------------------
    cglobal eb_vp9_Log2f_SSE2
    bsr rax, r0
    ret
