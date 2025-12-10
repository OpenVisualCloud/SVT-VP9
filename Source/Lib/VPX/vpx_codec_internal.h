/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*!\file
 * \brief Describes the decoder algorithm interface for algorithm
 *        implementations.
 *
 * This file defines the private structures and data types that are only
 * relevant to implementing an algorithm, as opposed to using it.
 *
 * To create a decoder algorithm class, an interface structure is put
 * into the global namespace:
 *     <pre>
 *     my_codec.c:
 *       vpx_codec_iface_t my_codec = {
 *           "My Codec v1.0",
 *           VPX_CODEC_ALG_ABI_VERSION,
 *           ...
 *       };
 *     </pre>
 *
 * An application instantiates a specific decoder instance by using
 * vpx_codec_init() and a pointer to the algorithm's interface structure:
 *     <pre>
 *     my_app.c:
 *       extern vpx_codec_iface_t my_codec;
 *       {
 *           vpx_codec_ctx_t algo;
 *           res = vpx_codec_init(&algo, &my_codec);
 *       }
 *     </pre>
 *
 * Once initialized, the instance is manged using other functions from
 * the vpx_codec_* family.
 */
#ifndef VPX_VPX_INTERNAL_VPX_CODEC_INTERNAL_H_
#define VPX_VPX_INTERNAL_VPX_CODEC_INTERNAL_H_

#include <stdint.h>
#include "prob.h"
#include "vpx_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!\brief Current ABI version number
 *
 * \internal
 * If this file is altered in any way that changes the ABI, this value
 * must be bumped.  Examples include, but are not limited to, changing
 * types, removing or reassigning enums, adding/removing/rearranging
 * fields to structures
 */
#define VPX_CODEC_INTERNAL_ABI_VERSION (5) /**<\hideinitializer*/

typedef struct vpx_codec_alg_priv        vpx_codec_alg_priv_t;
typedef struct vpx_codec_priv_enc_mr_cfg vpx_codec_priv_enc_mr_cfg_t;
#include <stdio.h>
#include <setjmp.h>

#define CLANG_ANALYZER_NORETURN
#if defined(__has_feature)
#if __has_feature(attribute_analyzer_noreturn)
#undef CLANG_ANALYZER_NORETURN
#define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#endif
#endif
#ifdef __cplusplus
} // extern "C"
#endif

#endif // VPX_VPX_INTERNAL_VPX_CODEC_INTERNAL_H_
