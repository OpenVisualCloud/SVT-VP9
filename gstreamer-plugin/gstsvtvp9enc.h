/*
* Copyright(c) 2019 Intel Corporation
*     Authors: Jun Tian <jun.tian@intel.com> Xavier Hallade <xavier.hallade@intel.com>
* SPDX - License - Identifier: LGPL-2.1-or-later
*/

#ifndef _GST_SVTVP9ENC_H_
#define _GST_SVTVP9ENC_H_

#include <string.h>
#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>

#include <EbSvtVp9Enc.h>

G_BEGIN_DECLS
#define GST_TYPE_SVTVP9ENC \
  (gst_svtvp9enc_get_type())
#define GST_SVTVP9ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SVTVP9ENC,GstSvtVp9Enc))
#define GST_SVTVP9ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SVTVP9ENC,GstSvtHevcEncClass))
#define GST_IS_SVTVP9ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SVTVP9ENC))
#define GST_IS_SVTVP9ENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SVTVP9ENC))

typedef struct _GstSvtVp9Enc
{
  GstVideoEncoder video_encoder;

  /* SVT-VP9 Encoder Handle */
  EbComponentType *svt_encoder;

  /* GStreamer Codec state */
  GstVideoCodecState *state;

  /* SVT-VP9 configuration */
  EbSvtVp9EncConfiguration *svt_config;

  EbBufferHeaderType *input_buf;

  long long int frame_count;
  int dts_offset;
} GstSvtVp9Enc;

typedef struct _GstSvtVp9EncClass
{
  GstVideoEncoderClass video_encoder_class;
} GstSvtVp9EncClass;

GType gst_svtvp9enc_get_type (void);

G_END_DECLS
#endif
