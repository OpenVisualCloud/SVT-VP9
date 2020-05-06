/*
* Copyright(c) 2019 Intel Corporation
*     Authors: Jun Tian <jun.tian@intel.com> Xavier Hallade <xavier.hallade@intel.com>
* SPDX - License - Identifier: LGPL-2.1-or-later
*/

/**
 * SECTION:element-gstsvtvp9enc
 *
 * The svtvp9enc element does VP9 encoding using Scalable
 * Video Technology for VP9 Encoder (SVT-VP9 Encoder).
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -e videotestsrc ! video/x-raw ! svtvp9enc ! matroskamux ! filesink location=out.mkv
 * ]|
 * Encodes test input into VP9 compressed data which is then packaged in out.mkv
 * </refsect2>
 */

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>
#include "gstsvtvp9enc.h"

GST_DEBUG_CATEGORY_STATIC (gst_svtvp9enc_debug_category);
#define GST_CAT_DEFAULT gst_svtvp9enc_debug_category

/* prototypes */
static void gst_svtvp9enc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_svtvp9enc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_svtvp9enc_dispose (GObject * object);
static void gst_svtvp9enc_finalize (GObject * object);

gboolean gst_svtvp9enc_allocate_svt_buffers (GstSvtVp9Enc * svtvp9enc);
void gst_svthevenc_deallocate_svt_buffers (GstSvtVp9Enc * svtvp9enc);
static gboolean gst_svtvp9enc_configure_svt (GstSvtVp9Enc * svtvp9enc);
static GstFlowReturn gst_svtvp9enc_encode (GstSvtVp9Enc * svtvp9enc,
    GstVideoCodecFrame * frame);
static gboolean gst_svtvp9enc_send_eos (GstSvtVp9Enc * svtvp9enc);
static GstFlowReturn gst_svtvp9enc_dequeue_encoded_frames (GstSvtVp9Enc *
    svtvp9enc, gboolean closing_encoder, gboolean output_frames);

static gboolean gst_svtvp9enc_open (GstVideoEncoder * encoder);
static gboolean gst_svtvp9enc_close (GstVideoEncoder * encoder);
static gboolean gst_svtvp9enc_start (GstVideoEncoder * encoder);
static gboolean gst_svtvp9enc_stop (GstVideoEncoder * encoder);
static gboolean gst_svtvp9enc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_svtvp9enc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static GstFlowReturn gst_svtvp9enc_finish (GstVideoEncoder * encoder);
static GstFlowReturn gst_svtvp9enc_pre_push (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static GstCaps *gst_svtvp9enc_getcaps (GstVideoEncoder * encoder,
    GstCaps * filter);
static gboolean gst_svtvp9enc_sink_event (GstVideoEncoder * encoder,
    GstEvent * event);
static gboolean gst_svtvp9enc_src_event (GstVideoEncoder * encoder,
    GstEvent * event);
static gboolean gst_svtvp9enc_negotiate (GstVideoEncoder * encoder);
static gboolean gst_svtvp9enc_decide_allocation (GstVideoEncoder * encoder,
    GstQuery * query);
static gboolean gst_svtvp9enc_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);
static gboolean gst_svtvp9enc_flush (GstVideoEncoder * encoder);

/* helpers */
void set_default_svt_configuration (EbSvtVp9EncConfiguration * svt_config);
gint compare_video_code_frame_and_pts (const void *video_codec_frame_ptr,
    const void *pts_ptr);

enum
{
  PROP_0,
  PROP_ENCMODE,
  PROP_TUNE,
  PROP_SPEEDCONTROL,
  PROP_B_PYRAMID,
  PROP_P_FRAMES,
  PROP_PRED_STRUCTURE,
  PROP_GOP_SIZE,
  PROP_QP,
  PROP_DEBLOCKING,
  PROP_CONSTRAINED_INTRA,
  PROP_RC_MODE,
  PROP_BITRATE,
  PROP_QP_MAX,
  PROP_QP_MIN,
  PROP_LOOKAHEAD,
  PROP_SCD,
  PROP_CORES,
  PROP_SOCKET
};

#define PROP_RC_MODE_CQP 0
#define PROP_RC_MODE_VBR 1

#define PROP_ENCMODE_DEFAULT                9
#define PROP_TUNE_DEFAULT                   1
#define PROP_SPEEDCONTROL_DEFAULT           60
#define PROP_HIERARCHICAL_LEVEL_DEFAULT     4
#define PROP_P_FRAMES_DEFAULT               FALSE
#define PROP_PRED_STRUCTURE_DEFAULT         2
#define PROP_GOP_SIZE_DEFAULT               -1
#define PROP_QP_DEFAULT                     45
#define PROP_DEBLOCKING_DEFAULT             TRUE
#define PROP_CONSTRAINED_INTRA_DEFAULT      FALSE
#define PROP_RC_MODE_DEFAULT                PROP_RC_MODE_CQP
#define PROP_BITRATE_DEFAULT                7000000
#define PROP_QP_MAX_DEFAULT                 63
#define PROP_QP_MIN_DEFAULT                 10
#define PROP_LOOKAHEAD_DEFAULT              (unsigned int)-1
#define PROP_SCD_DEFAULT                    FALSE
#define PROP_AUD_DEFAULT                    FALSE
#define PROP_CORES_DEFAULT                  0
#define PROP_SOCKET_DEFAULT                 -1

/* pad templates */
static GstStaticPadTemplate gst_svtvp9enc_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) {I420, I420_10LE}, "
        "width = (int) [64, 3840], "
        "height = (int) [64, 2160], " "framerate = (fraction) [0, MAX]")
    );

static GstStaticPadTemplate gst_svtvp9enc_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp9, "
        "stream-format = (string) byte-stream, "
        "alignment = (string) au, "
        "width = (int) [64, 3840], "
        "height = (int) [64, 2160], " "framerate = (fraction) [0, MAX]")
    );

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (GstSvtVp9Enc, gst_svtvp9enc, GST_TYPE_VIDEO_ENCODER,
    GST_DEBUG_CATEGORY_INIT (gst_svtvp9enc_debug_category, "svtvp9enc", 0,
        "debug category for SVT-VP9 encoder element"));

/* this mutex is required to avoid race conditions in SVT-VP9 memory allocations, which aren't thread-safe */
G_LOCK_DEFINE_STATIC (init_mutex);

static void
gst_svtvp9enc_class_init (GstSvtVp9EncClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoEncoderClass *video_encoder_class = GST_VIDEO_ENCODER_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_svtvp9enc_src_pad_template);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_svtvp9enc_sink_pad_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "SvtVp9Enc", "Codec/Encoder/Video",
      "Scalable Video Technology for VP9 Encoder (SVT-VP9 Encoder)",
      "Jun Tian <jun.tian@intel.com> Xavier Hallade <xavier.hallade@intel.com>");

  gobject_class->set_property = gst_svtvp9enc_set_property;
  gobject_class->get_property = gst_svtvp9enc_get_property;
  gobject_class->dispose = gst_svtvp9enc_dispose;
  gobject_class->finalize = gst_svtvp9enc_finalize;
  video_encoder_class->open = GST_DEBUG_FUNCPTR (gst_svtvp9enc_open);
  video_encoder_class->close = GST_DEBUG_FUNCPTR (gst_svtvp9enc_close);
  video_encoder_class->start = GST_DEBUG_FUNCPTR (gst_svtvp9enc_start);
  video_encoder_class->stop = GST_DEBUG_FUNCPTR (gst_svtvp9enc_stop);
  video_encoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_svtvp9enc_set_format);
  video_encoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_svtvp9enc_handle_frame);
  video_encoder_class->finish = GST_DEBUG_FUNCPTR (gst_svtvp9enc_finish);
  video_encoder_class->pre_push = GST_DEBUG_FUNCPTR (gst_svtvp9enc_pre_push);
  video_encoder_class->getcaps = GST_DEBUG_FUNCPTR (gst_svtvp9enc_getcaps);
  video_encoder_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_svtvp9enc_sink_event);
  video_encoder_class->src_event = GST_DEBUG_FUNCPTR (gst_svtvp9enc_src_event);
  video_encoder_class->negotiate = GST_DEBUG_FUNCPTR (gst_svtvp9enc_negotiate);
  video_encoder_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_svtvp9enc_decide_allocation);
  video_encoder_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_svtvp9enc_propose_allocation);
  video_encoder_class->flush = GST_DEBUG_FUNCPTR (gst_svtvp9enc_flush);

  g_object_class_install_property (gobject_class, PROP_ENCMODE,
      g_param_spec_uint ("speed", "speed (Encoder Mode)",
          "Quality vs density tradeoff point that the encoding is to be performed at"
          " (0 is the highest quality, 9 is the highest speed) ",
          0, 9, PROP_ENCMODE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_TUNE,
      g_param_spec_uint("tune", "Tune",
          "0 gives a visually optimized mode."
          " Set to 1 to tune for PSNR/SSIM, 2 for VMAF.",
          0, 2, PROP_TUNE_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SPEEDCONTROL,
      g_param_spec_uint ("speed-control", "Speed Control (in fps)",
          "Dynamically change the encoding speed preset"
          " to meet this defined average encoding speed (in fps)",
          1, 240, PROP_SPEEDCONTROL_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_B_PYRAMID,
  //    g_param_spec_uint ("hierarchical-level", "Hierarchical levels",
  //        "3 : 4 - Level Hierarchy,"
  //        "4 : 5 - Level Hierarchy",
  //        3, 4, PROP_HIERARCHICAL_LEVEL_DEFAULT,
  //        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_P_FRAMES,
      g_param_spec_boolean ("p-frames", "P Frames",
          "Use P-frames in the base layer",
          PROP_P_FRAMES_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PRED_STRUCTURE,
      g_param_spec_uint ("pred-struct", "Prediction Structure",
          "0 : Low Delay P, 1 : Low Delay B"
          ", 2 : Random Access",
          0, 2, PROP_PRED_STRUCTURE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GOP_SIZE,
      g_param_spec_int ("gop-size", "GOP size",
          "Period of Intra Frames insertion (-1 is auto)",
          -1, 251, PROP_GOP_SIZE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP,
      g_param_spec_uint ("qp", "Quantization parameter",
          "Quantization parameter used in CQP mode",
          0, 63, PROP_QP_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_DEBLOCKING,
  //    g_param_spec_boolean ("deblocking", "Deblock Filter",
  //        "Enable Deblocking Loop Filtering",
  //        PROP_DEBLOCKING_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_CONSTRAINED_INTRA,
  //    g_param_spec_boolean ("constrained-intra", "Constrained Intra",
  //        "Enable Constrained Intra"
  //        "- this yields to sending two PPSs in the VP9 Elementary streams",
  //        PROP_CONSTRAINED_INTRA_DEFAULT,
  //        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RC_MODE,
      g_param_spec_uint ("rc", "Rate-control mode",
          "0 : CQP, 1 : VBR",
          0, 1, PROP_RC_MODE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* TODO: add GST_PARAM_MUTABLE_PLAYING property and handle it? */
  g_object_class_install_property (gobject_class, PROP_BITRATE,
      g_param_spec_uint ("bitrate", "Target bitrate",
          "Target bitrate in bits/sec. Only used when in VBR mode",
          1, G_MAXUINT, PROP_BITRATE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MAX,
      g_param_spec_uint ("max-qp", "Max Quantization parameter",
          "Maximum QP value allowed for rate control use"
          " Only used in VBR mode.",
          0, 63, PROP_QP_MAX_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MIN,
      g_param_spec_uint ("min-qp", "Min Quantization parameter",
          "Minimum QP value allowed for rate control use"
          " Only used in VBR mode.",
          0, 63, PROP_QP_MIN_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_LOOKAHEAD,
  //    g_param_spec_int ("lookahead", "Look Ahead Distance",
  //        "Number of frames to look ahead. -1 lets the encoder pick a value",
  //        -1, 250, PROP_LOOKAHEAD_DEFAULT,
  //        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_SCD,
  //    g_param_spec_boolean ("scd", "Scene Change Detection",
  //        "Enable Scene Change Detection algorithm",
  //        PROP_SCD_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_CORES,
  //    g_param_spec_uint ("cores", "Number of logical cores",
  //        "Number of logical cores to be used. 0: auto",
  //        0, UINT_MAX, PROP_CORES_DEFAULT,
  //        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  //g_object_class_install_property (gobject_class, PROP_SOCKET,
  //    g_param_spec_int ("socket", "Target socket",
  //        "Target socket to run on. -1: all available",
  //        -1, 15, PROP_SOCKET_DEFAULT,
  //        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_svtvp9enc_init (GstSvtVp9Enc * svtvp9enc)
{
  GST_OBJECT_LOCK (svtvp9enc);
  svtvp9enc->svt_config = g_malloc (sizeof (EbSvtVp9EncConfiguration));
  if (!svtvp9enc->svt_config) {
    GST_ERROR_OBJECT (svtvp9enc, "insufficient resources");
    GST_OBJECT_UNLOCK (svtvp9enc);
    return;
  }
  memset (&svtvp9enc->svt_encoder, 0, sizeof (svtvp9enc->svt_encoder));
  svtvp9enc->frame_count = 0;
  svtvp9enc->dts_offset = 0;

  EbErrorType res =
      eb_vp9_svt_init_handle(&svtvp9enc->svt_encoder, NULL, svtvp9enc->svt_config);
  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svtvp9enc, "eb_vp9_svt_init_handle failed with error %d", res);
    GST_OBJECT_UNLOCK (svtvp9enc);
    return;
  }
  /* setting configuration here since eb_svt_init_handle overrides it */
  set_default_svt_configuration (svtvp9enc->svt_config);
  GST_OBJECT_UNLOCK (svtvp9enc);
}

void
gst_svtvp9enc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (object);

  /* TODO: support reconfiguring on the fly when possible */
  if (svtvp9enc->state) {
    GST_ERROR_OBJECT (svtvp9enc,
        "encoder state has been set before properties, this isn't supported yet.");
    return;
  }

  GST_LOG_OBJECT (svtvp9enc, "setting property %u", property_id);

  switch (property_id) {
    case PROP_ENCMODE:
      svtvp9enc->svt_config->enc_mode = g_value_get_uint (value);
      break;
    case PROP_TUNE:
        svtvp9enc->svt_config->tune = g_value_get_uint(value);
      break;
    case PROP_GOP_SIZE:
        svtvp9enc->svt_config->intra_period = g_value_get_int(value) - 1;
        break;
    case PROP_SPEEDCONTROL:
      if (g_value_get_uint (value) > 0) {
        svtvp9enc->svt_config->injector_frame_rate = g_value_get_uint (value);
        svtvp9enc->svt_config->speed_control_flag = 1;
      } else {
        svtvp9enc->svt_config->injector_frame_rate = 60 << 16;
        svtvp9enc->svt_config->speed_control_flag = 0;
      }
      break;
    //case PROP_B_PYRAMID:
    //  svtvp9enc->svt_config->hierarchical_levels = g_value_get_uint (value);
    //  break;
    case PROP_PRED_STRUCTURE:
        svtvp9enc->svt_config->pred_structure = g_value_get_uint(value);
        break;
    case PROP_P_FRAMES:
      svtvp9enc->svt_config->base_layer_switch_mode = g_value_get_boolean (value);
      break;
    case PROP_QP:
      svtvp9enc->svt_config->qp = g_value_get_uint (value);
      break;
    //case PROP_DEBLOCKING:
    //  svtvp9enc->svt_config->disable_dlf_flag = !g_value_get_boolean (value);
    //  break;
    //case PROP_CONSTRAINED_INTRA:
    //  svtvp9enc->svt_config->constrained_intra = g_value_get_boolean (value);
    //  break;
    case PROP_RC_MODE:
      svtvp9enc->svt_config->rate_control_mode = g_value_get_uint (value);
      break;
    case PROP_BITRATE:
      svtvp9enc->svt_config->target_bit_rate = g_value_get_uint (value) * 1000;
      break;
    case PROP_QP_MAX:
      svtvp9enc->svt_config->max_qp_allowed = g_value_get_uint (value);
      break;
    case PROP_QP_MIN:
      svtvp9enc->svt_config->min_qp_allowed = g_value_get_uint (value);
      break;
    //case PROP_LOOKAHEAD:
    //  svtvp9enc->svt_config->look_ahead_distance =
    //      (unsigned int)g_value_get_int(value);
    //  break;
    //case PROP_SCD:
    //  svtvp9enc->svt_config->scene_change_detection =
    //      g_value_get_boolean (value);
    //  break;
    //case PROP_CORES:
    //  svtvp9enc->svt_config->logical_processors = g_value_get_uint (value);
    //  break;
    //case PROP_SOCKET:
    //  svtvp9enc->svt_config->target_socket = g_value_get_int (value);
    //  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_svtvp9enc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (object);

  GST_LOG_OBJECT (svtvp9enc, "getting property %u", property_id);

  switch (property_id) {
    case PROP_ENCMODE:
      g_value_set_uint (value, svtvp9enc->svt_config->enc_mode);
      break;
    case PROP_TUNE:
      g_value_set_uint(value, svtvp9enc->svt_config->tune);
      break;
    case PROP_SPEEDCONTROL:
      if (svtvp9enc->svt_config->speed_control_flag) {
        g_value_set_uint (value, svtvp9enc->svt_config->injector_frame_rate);
      } else {
        g_value_set_uint (value, 0);
      }
      break;
    //case PROP_B_PYRAMID:
    //  g_value_set_uint (value, svtvp9enc->svt_config->hierarchical_levels);
    //  break;
    case PROP_P_FRAMES:
      g_value_set_boolean (value,
          svtvp9enc->svt_config->base_layer_switch_mode == 1);
      break;
    case PROP_PRED_STRUCTURE:
      g_value_set_uint (value, svtvp9enc->svt_config->pred_structure);
      break;
    case PROP_GOP_SIZE:
      g_value_set_int (value, svtvp9enc->svt_config->intra_period + 1);
      break;
    case PROP_QP:
      g_value_set_uint (value, svtvp9enc->svt_config->qp);
      break;
    //case PROP_DEBLOCKING:
    //  g_value_set_boolean (value, svtvp9enc->svt_config->disable_dlf_flag == 0);
    //  break;
    //case PROP_CONSTRAINED_INTRA:
    //  g_value_set_boolean (value,
    //      svtvp9enc->svt_config->constrained_intra == 1);
    //  break;
    case PROP_RC_MODE:
      g_value_set_uint (value, svtvp9enc->svt_config->rate_control_mode);
      break;
    case PROP_BITRATE:
      g_value_set_uint (value, svtvp9enc->svt_config->target_bit_rate / 1000);
      break;
    case PROP_QP_MAX:
      g_value_set_uint (value, svtvp9enc->svt_config->max_qp_allowed);
      break;
    case PROP_QP_MIN:
      g_value_set_uint (value, svtvp9enc->svt_config->min_qp_allowed);
      break;
    //case PROP_LOOKAHEAD:
    //    g_value_set_int(value, (int)svtvp9enc->svt_config->look_ahead_distance);
    //  break;
    //case PROP_SCD:
    //  g_value_set_boolean (value,
    //      svtvp9enc->svt_config->scene_change_detection == 1);
    //  break;
    //case PROP_CORES:
    //  g_value_set_uint (value, svtvp9enc->svt_config->logical_processors);
    //  break;
    //case PROP_SOCKET:
    //  g_value_set_int (value, svtvp9enc->svt_config->target_socket);
    //  break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_svtvp9enc_dispose (GObject * object)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (object);

  GST_DEBUG_OBJECT (svtvp9enc, "dispose");

  /* clean up as possible.  may be called multiple times */
  if (svtvp9enc->state)
    gst_video_codec_state_unref (svtvp9enc->state);
  svtvp9enc->state = NULL;

  G_OBJECT_CLASS (gst_svtvp9enc_parent_class)->dispose (object);
}

void
gst_svtvp9enc_finalize (GObject * object)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (object);

  GST_DEBUG_OBJECT (svtvp9enc, "finalizing svtvp9enc");

  GST_OBJECT_LOCK (svtvp9enc);
  eb_vp9_deinit_handle(svtvp9enc->svt_encoder);
  svtvp9enc->svt_encoder = NULL;
  g_free (svtvp9enc->svt_config);
  GST_OBJECT_UNLOCK (svtvp9enc);

  G_OBJECT_CLASS (gst_svtvp9enc_parent_class)->finalize (object);
}

gboolean
gst_svtvp9enc_allocate_svt_buffers (GstSvtVp9Enc * svtvp9enc)
{
  svtvp9enc->input_buf = g_malloc (sizeof (EbBufferHeaderType));
  if (!svtvp9enc->input_buf) {
    GST_ERROR_OBJECT (svtvp9enc, "insufficient resources");
    return FALSE;
  }
  svtvp9enc->input_buf->p_buffer = g_malloc (sizeof (EbSvtEncInput));
  if (!svtvp9enc->input_buf->p_buffer) {
    GST_ERROR_OBJECT (svtvp9enc, "insufficient resources");
    return FALSE;
  }
  memset(svtvp9enc->input_buf->p_buffer, 0, sizeof(EbSvtEncInput));
  svtvp9enc->input_buf->size = sizeof (EbBufferHeaderType);
  svtvp9enc->input_buf->p_app_private = NULL;
  svtvp9enc->input_buf->pic_type = EB_I_PICTURE;

  return TRUE;
}

void
gst_svthevenc_deallocate_svt_buffers (GstSvtVp9Enc * svtvp9enc)
{
  if (svtvp9enc->input_buf) {
    g_free (svtvp9enc->input_buf->p_buffer);
    svtvp9enc->input_buf->p_buffer = NULL;
    g_free (svtvp9enc->input_buf);
    svtvp9enc->input_buf = NULL;
  }
}

gboolean
gst_svtvp9enc_configure_svt (GstSvtVp9Enc * svtvp9enc)
{
  if (!svtvp9enc->state) {
    GST_WARNING_OBJECT (svtvp9enc, "no state, can't configure encoder yet");
    return FALSE;
  }

  /* set properties out of GstVideoInfo */
  GstVideoInfo *info = &svtvp9enc->state->info;
  svtvp9enc->svt_config->encoder_bit_depth = GST_VIDEO_INFO_COMP_DEPTH (info, 0);
  svtvp9enc->svt_config->source_width = GST_VIDEO_INFO_WIDTH (info);
  svtvp9enc->svt_config->source_height = GST_VIDEO_INFO_HEIGHT (info);
  svtvp9enc->svt_config->frame_rate_numerator = GST_VIDEO_INFO_FPS_N (info) > 0 ? GST_VIDEO_INFO_FPS_N (info) : 1;
  svtvp9enc->svt_config->frame_rate_denominator = GST_VIDEO_INFO_FPS_D (info) > 0 ? GST_VIDEO_INFO_FPS_D (info) : 1;
  svtvp9enc->svt_config->frame_rate =
      svtvp9enc->svt_config->frame_rate_numerator /
      svtvp9enc->svt_config->frame_rate_denominator;

  if (svtvp9enc->svt_config->frame_rate < 1000) {
      svtvp9enc->svt_config->frame_rate = svtvp9enc->svt_config->frame_rate << 16;
  }

  GST_LOG_OBJECT(svtvp9enc, "width %d, height %d, framerate %d", svtvp9enc->svt_config->source_width, svtvp9enc->svt_config->source_height, svtvp9enc->svt_config->frame_rate);

  /* pick a default value for the look ahead distance
   * in CQP mode:2*minigop+1. in VBR:  intra Period */
  //if (svtvp9enc->svt_config->look_ahead_distance == (unsigned int) -1) {
  //  svtvp9enc->svt_config->look_ahead_distance =
  //      (svtvp9enc->svt_config->rate_control_mode == PROP_RC_MODE_VBR) ?
  //      svtvp9enc->svt_config->intra_period_length :
  //      2 * (1 << svtvp9enc->svt_config->hierarchical_levels) + 1;
  //}

  /* TODO: better handle HDR metadata when GStreamer will have such support
   * https://gitlab.freedesktop.org/gstreamer/gst-plugins-base/issues/400 */
  //if (GST_VIDEO_INFO_COLORIMETRY (info).matrix == GST_VIDEO_COLOR_MATRIX_BT2020
  //    && GST_VIDEO_INFO_COMP_DEPTH (info, 0) > 8) {
  //  svtvp9enc->svt_config->high_dynamic_range_input = TRUE;
  //}

  EbErrorType res =
      eb_vp9_svt_enc_set_parameter(svtvp9enc->svt_encoder, svtvp9enc->svt_config);
  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svtvp9enc, "eb_vp9_svt_enc_set_parameter failed with error %d", res);
    return FALSE;
  }
  return TRUE;
}

gboolean
gst_svtvp9enc_start_svt (GstSvtVp9Enc * svtvp9enc)
{
  G_LOCK (init_mutex);
  EbErrorType res = eb_vp9_init_encoder(svtvp9enc->svt_encoder);
  G_UNLOCK (init_mutex);

  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svtvp9enc, "eb_vp9_init_encoder failed with error %d", res);
    return FALSE;
  }
  return TRUE;
}

void
set_default_svt_configuration (EbSvtVp9EncConfiguration * svt_config)
{
  memset(svt_config, 0, sizeof(EbSvtVp9EncConfiguration));
  svt_config->source_width = 0;
  svt_config->source_height = 0;
  svt_config->intra_period = PROP_GOP_SIZE_DEFAULT - 1;
  svt_config->enc_mode = PROP_ENCMODE_DEFAULT;
  svt_config->tune = PROP_TUNE_DEFAULT;
  svt_config->frame_rate = 25;
  svt_config->frame_rate_denominator = 1;
  svt_config->frame_rate_numerator = 25;
  //svt_config->hierarchical_levels = PROP_HIERARCHICAL_LEVEL_DEFAULT;
  svt_config->base_layer_switch_mode = PROP_P_FRAMES_DEFAULT;
  svt_config->pred_structure = PROP_PRED_STRUCTURE_DEFAULT;
  //svt_config->scene_change_detection = PROP_SCD_DEFAULT;
  //svt_config->look_ahead_distance = (uint32_t)~0;
  //svt_config->framesToBeEncoded = 0;
  svt_config->rate_control_mode = PROP_RC_MODE_DEFAULT;
  svt_config->target_bit_rate = PROP_BITRATE_DEFAULT;
  svt_config->max_qp_allowed = PROP_QP_MAX_DEFAULT;
  svt_config->min_qp_allowed = PROP_QP_MIN_DEFAULT;
  svt_config->qp = PROP_QP_DEFAULT;
  svt_config->use_qp_file = FALSE;
  svt_config->loop_filter = TRUE;
  //svt_config->disable_dlf_flag = (PROP_DEBLOCKING_DEFAULT == FALSE);
  //svt_config->enable_denoise_flag = FALSE;
  //svt_config->film_grain_denoise_strength = FALSE;
  //svt_config->enable_warped_motion = FALSE;
  svt_config->use_default_me_hme = TRUE;
  svt_config->enable_hme_flag = TRUE;
  //svt_config->enable_hme_level0_flag = TRUE;
  //svt_config->enable_hme_level1_flag = FALSE;
  //svt_config->enable_hme_level2_flag = FALSE;
  //svt_config->ext_block_flag = TRUE;
  //svt_config->in_loop_me_flag = TRUE;
  svt_config->search_area_width = 16;
  svt_config->search_area_height = 7;
  //svt_config->number_hme_search_region_in_width = 2;
  //svt_config->number_hme_search_region_in_height = 2;
  //svt_config->hme_level0_total_search_area_width = 64;
  //svt_config->hme_level0_total_search_area_height = 25;
  //svt_config->hme_level0_search_area_in_width_array[0] = 32;
  //svt_config->hme_level0_search_area_in_width_array[1] = 32;
  //svt_config->hme_level0_search_area_in_height_array[0] = 12;
  //svt_config->hme_level0_search_area_in_height_array[1] = 13;
  //svt_config->hme_level1_search_area_in_width_array[0] = 1;
  //svt_config->hme_level1_search_area_in_width_array[1] = 1;
  //svt_config->hme_level1_search_area_in_height_array[0] = 1;
  //svt_config->hme_level1_search_area_in_height_array[1] = 1;
  //svt_config->hme_level2_search_area_in_width_array[0] = 1;
  //svt_config->hme_level2_search_area_in_width_array[1] = 1;
  //svt_config->hme_level2_search_area_in_height_array[0] = 1;
  //svt_config->hme_level2_search_area_in_height_array[1] = 1;
  //svt_config->constrained_intra = PROP_CONSTRAINED_INTRA_DEFAULT;
  svt_config->channel_id = 0;
  svt_config->active_channel_count = 1;
  //svt_config->logical_processors = PROP_CORES_DEFAULT;
  //svt_config->target_socket = PROP_SOCKET_DEFAULT;
  //svt_config->improve_sharpness = FALSE;
  ///* needed to have correct framerate/duration in bitstream */
  //svt_config->high_dynamic_range_input = FALSE;
  //svt_config->access_unit_delimiter = PROP_AUD_DEFAULT;
  //svt_config->buffering_period_sei = FALSE;
  //svt_config->picture_timing_sei = FALSE;
  //svt_config->registered_user_data_sei_flag = FALSE;
  //svt_config->unregistered_user_data_sei_flag = FALSE;
  //svt_config->recovery_point_sei_flag = FALSE;
  //svt_config->enable_temporal_id = 1;
  svt_config->encoder_bit_depth = 8;
  //svt_config->compressed_ten_bit_format = FALSE;
  svt_config->profile = 0;
  //svt_config->tier = 0;
  svt_config->level = 0;
  svt_config->injector_frame_rate = PROP_SPEEDCONTROL_DEFAULT;
  svt_config->speed_control_flag = FALSE;
  //svt_config->sb_sz = 64;
  //svt_config->super_block_size = 128;
  svt_config->partition_depth = 0;
  svt_config->enable_qp_scaling_flag = 0;
  svt_config->asm_type = 1;
  svt_config->input_picture_stride = 0;
}

GstFlowReturn
gst_svtvp9enc_encode (GstSvtVp9Enc * svtvp9enc, GstVideoCodecFrame * frame)
{
  GstFlowReturn ret = GST_FLOW_OK;
  EbErrorType res = EB_ErrorNone;
  EbBufferHeaderType *input_buffer = svtvp9enc->input_buf;
  EbSvtEncInput *input_picture_buffer =
      (EbSvtEncInput *) svtvp9enc->input_buf->p_buffer;
  GstVideoFrame video_frame;

  if (!gst_video_frame_map (&video_frame, &svtvp9enc->state->info,
          frame->input_buffer, GST_MAP_READ)) {
    GST_ERROR_OBJECT (svtvp9enc, "couldn't map input frame");
    return GST_FLOW_ERROR;
  }

  input_picture_buffer->y_stride =
      GST_VIDEO_FRAME_COMP_STRIDE (&video_frame,
      0) / GST_VIDEO_FRAME_COMP_PSTRIDE (&video_frame, 0);
  input_picture_buffer->cb_stride =
      GST_VIDEO_FRAME_COMP_STRIDE (&video_frame,
      1) / GST_VIDEO_FRAME_COMP_PSTRIDE (&video_frame, 1);
  input_picture_buffer->cr_stride =
      GST_VIDEO_FRAME_COMP_STRIDE (&video_frame,
      2) / GST_VIDEO_FRAME_COMP_PSTRIDE (&video_frame, 2);

  input_picture_buffer->luma = GST_VIDEO_FRAME_PLANE_DATA (&video_frame, 0);
  input_picture_buffer->cb = GST_VIDEO_FRAME_PLANE_DATA (&video_frame, 1);
  input_picture_buffer->cr = GST_VIDEO_FRAME_PLANE_DATA (&video_frame, 2);

  input_buffer->n_filled_len = GST_VIDEO_FRAME_SIZE (&video_frame);

  /* Fill in Buffers Header control data */
  input_buffer->flags = 0;
  input_buffer->p_app_private = (void *) frame;
  input_buffer->pts = frame->pts;
  input_buffer->pic_type = EB_INVALID_PICTURE;

  if (GST_VIDEO_CODEC_FRAME_IS_FORCE_KEYFRAME (frame)) {
    input_buffer->pic_type = EB_IDR_PICTURE;
  }

  res = eb_vp9_svt_enc_send_picture(svtvp9enc->svt_encoder, input_buffer);
  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svtvp9enc, "Issue %d sending picture to SVT-VP9.", res);
    ret = GST_FLOW_ERROR;
  }
  gst_video_frame_unmap (&video_frame);

  return ret;
}

gboolean
gst_svtvp9enc_send_eos (GstSvtVp9Enc * svtvp9enc)
{
  EbErrorType ret = EB_ErrorNone;

  EbBufferHeaderType input_buffer;
  input_buffer.n_alloc_len = 0;
  input_buffer.n_filled_len = 0;
  input_buffer.n_tick_count = 0;
  input_buffer.p_app_private = NULL;
  input_buffer.flags = EB_BUFFERFLAG_EOS;
  input_buffer.p_buffer = NULL;

  ret = eb_vp9_svt_enc_send_picture(svtvp9enc->svt_encoder, &input_buffer);

  if (ret != EB_ErrorNone) {
    GST_ERROR_OBJECT (svtvp9enc, "couldn't send EOS frame.");
    return FALSE;
  }

  return (ret == EB_ErrorNone);
}

gboolean
gst_svtvp9enc_flush (GstVideoEncoder * encoder)
{
  GstFlowReturn ret =
      gst_svtvp9enc_dequeue_encoded_frames (GST_SVTVP9ENC (encoder), TRUE,
      FALSE);

  return (ret != GST_FLOW_ERROR);
}

gint
compare_video_code_frame_and_pts (const void *video_codec_frame_ptr,
    const void *pts_ptr)
{
  return ((GstVideoCodecFrame *) video_codec_frame_ptr)->pts -
      *((GstClockTime *) pts_ptr);
}

GstFlowReturn
gst_svtvp9enc_dequeue_encoded_frames (GstSvtVp9Enc * svtvp9enc,
    gboolean done_sending_pics, gboolean output_frames)
{
  GstFlowReturn ret = GST_FLOW_OK;
  EbErrorType res = EB_ErrorNone;
  gboolean encode_at_eos = FALSE;

  do {
    GList *pending_frames = NULL;
    GList *frame_list_element = NULL;
    GstVideoCodecFrame *frame = NULL;
    EbBufferHeaderType *output_buf = NULL;

    res =
        eb_vp9_svt_get_packet(svtvp9enc->svt_encoder, &output_buf,
        done_sending_pics);

    if (output_buf != NULL)
      encode_at_eos =
          ((output_buf->flags & EB_BUFFERFLAG_EOS) == EB_BUFFERFLAG_EOS);

    if (res == EB_ErrorMax) {
      GST_ERROR_OBJECT (svtvp9enc, "Error while encoding, return\n");
      return GST_FLOW_ERROR;
    } else if (res != EB_NoErrorEmptyQueue && output_frames && output_buf) {
      /* if p_app_private is indeed propagated, get the frame through it
       * it's not currently the case with SVT-VP9
       * so we fallback on using its PTS to find it back */
      if (output_buf->p_app_private) {
        frame = (GstVideoCodecFrame *) output_buf->p_app_private;
      } else {
        pending_frames = gst_video_encoder_get_frames (GST_VIDEO_ENCODER
            (svtvp9enc));
        frame_list_element = g_list_find_custom (pending_frames,
            &output_buf->pts, compare_video_code_frame_and_pts);

        if (frame_list_element == NULL)
          return GST_FLOW_ERROR;

        frame = (GstVideoCodecFrame *) frame_list_element->data;
      }

      if (output_buf->pic_type == EB_IDR_PICTURE
          || output_buf->pic_type == EB_I_PICTURE) {
        GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
      }

      frame->output_buffer =
          gst_buffer_new_allocate (NULL, output_buf->n_filled_len, NULL);
      GST_BUFFER_FLAG_SET(frame->output_buffer, GST_BUFFER_FLAG_LIVE);
      gst_buffer_fill (frame->output_buffer, 0,
          output_buf->p_buffer, output_buf->n_filled_len);

      /* SVT-VP9 may return first frames with a negative DTS,
       * offsetting it to start at 0 since GStreamer 1.x doesn't support it */
      if (output_buf->dts + svtvp9enc->dts_offset < 0) {
        svtvp9enc->dts_offset = -output_buf->dts;
      }
      /* Gstreamer doesn't support negative DTS so we return
       * very small increasing ones for the first frames. */
      if (output_buf->dts < 1) {
        frame->dts = frame->output_buffer->dts =
            output_buf->dts + svtvp9enc->dts_offset;
      } else {
        frame->dts = frame->output_buffer->dts =
            (output_buf->dts *
            svtvp9enc->svt_config->frame_rate_denominator * GST_SECOND) /
            svtvp9enc->svt_config->frame_rate_numerator;
      }

      frame->pts = frame->output_buffer->pts = output_buf->pts;

      GST_LOG_OBJECT (svtvp9enc, "#frame:%lld dts:%" G_GINT64_FORMAT " pts:%"
          G_GINT64_FORMAT " SliceType:%d\n", svtvp9enc->frame_count,
           (frame->dts), (frame->pts), output_buf->pic_type);

      eb_vp9_svt_release_out_buffer(&output_buf);
      output_buf = NULL;

      ret = gst_video_encoder_finish_frame (GST_VIDEO_ENCODER (svtvp9enc), frame);

      if (pending_frames != NULL) {
        g_list_free_full (pending_frames,
            (GDestroyNotify) gst_video_codec_frame_unref);
      }

      svtvp9enc->frame_count++;
    }

  } while (res == EB_ErrorNone && !encode_at_eos);

  return ret;
}

static gboolean
gst_svtvp9enc_open (GstVideoEncoder * encoder)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "open");

  return TRUE;
}

static gboolean
gst_svtvp9enc_close (GstVideoEncoder * encoder)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "close");

  return TRUE;
}

static gboolean
gst_svtvp9enc_start (GstVideoEncoder * encoder)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "start");
  /* starting the encoder is done in set_format,
   * once caps are fully negotiated */

  return TRUE;
}

static gboolean
gst_svtvp9enc_stop (GstVideoEncoder * encoder)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "stop");

  GstVideoCodecFrame *remaining_frame = NULL;
  while ((remaining_frame =
          gst_video_encoder_get_oldest_frame (encoder)) != NULL) {
    GST_WARNING_OBJECT (svtvp9enc,
        "encoder is being stopped, dropping frame %d",
        remaining_frame->system_frame_number);
    remaining_frame->output_buffer = NULL;
    gst_video_encoder_finish_frame (encoder, remaining_frame);
  }

  GST_OBJECT_LOCK (svtvp9enc);
  if (svtvp9enc->state)
    gst_video_codec_state_unref (svtvp9enc->state);
  svtvp9enc->state = NULL;
  GST_OBJECT_UNLOCK (svtvp9enc);

  GST_OBJECT_LOCK (svtvp9enc);
  eb_vp9_deinit_encoder(svtvp9enc->svt_encoder);
  /* Destruct the buffer memory pool */
  gst_svthevenc_deallocate_svt_buffers (svtvp9enc);
  GST_OBJECT_UNLOCK (svtvp9enc);

  return TRUE;
}

static gboolean
gst_svtvp9enc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);
  GstClockTime min_latency_frames = 0;
  GstCaps *src_caps = NULL;
  GST_DEBUG_OBJECT (svtvp9enc, "set_format");

  /* TODO: handle configuration changes while encoder is running
   * and if there was already a state. */
  svtvp9enc->state = gst_video_codec_state_ref (state);

  gst_svtvp9enc_configure_svt (svtvp9enc);
  gst_svtvp9enc_allocate_svt_buffers (svtvp9enc);
  gst_svtvp9enc_start_svt (svtvp9enc);

  uint32_t fps = (uint32_t)((svtvp9enc->svt_config->frame_rate > 1000) ?
      svtvp9enc->svt_config->frame_rate >> 16 : svtvp9enc->svt_config->frame_rate);
  fps = fps > 120 ? 120 : fps;
  fps = fps < 24 ? 24 : fps;

  min_latency_frames = /*svtvp9enc->svt_config->look_ahead_distance + */((fps * 5) >> 2);

  /* TODO: find a better value for max_latency */
  gst_video_encoder_set_latency (encoder,
      min_latency_frames * GST_SECOND / svtvp9enc->svt_config->frame_rate,
      3 * GST_SECOND);

  src_caps =
      gst_static_pad_template_get_caps (&gst_svtvp9enc_src_pad_template);
  gst_video_encoder_set_output_state (GST_VIDEO_ENCODER (encoder), src_caps,
      svtvp9enc->state);
  gst_caps_unref (src_caps);

  GST_DEBUG_OBJECT (svtvp9enc, "output caps: %" GST_PTR_FORMAT,
      svtvp9enc->state->caps);

  return TRUE;
}

static GstFlowReturn
gst_svtvp9enc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);
  GstFlowReturn ret = GST_FLOW_OK;

  GST_DEBUG_OBJECT (svtvp9enc, "handle_frame");

  ret = gst_svtvp9enc_encode (svtvp9enc, frame);
  if (ret != GST_FLOW_OK) {
    GST_DEBUG_OBJECT (svtvp9enc, "gst_svtvp9enc_encode returned %d", ret);
    return ret;
  }

  return gst_svtvp9enc_dequeue_encoded_frames (svtvp9enc, FALSE, TRUE);
}

static GstFlowReturn
gst_svtvp9enc_finish (GstVideoEncoder * encoder)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "finish");

  gst_svtvp9enc_send_eos (svtvp9enc);

  return gst_svtvp9enc_dequeue_encoded_frames (svtvp9enc, TRUE, TRUE);
}

static GstFlowReturn
gst_svtvp9enc_pre_push (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "pre_push");

  return GST_FLOW_OK;
}

static GstCaps *
gst_svtvp9enc_getcaps (GstVideoEncoder * encoder, GstCaps * filter)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "getcaps");

  GstCaps *sink_caps =
      gst_static_pad_template_get_caps (&gst_svtvp9enc_sink_pad_template);
  GstCaps *ret =
      gst_video_encoder_proxy_getcaps (GST_VIDEO_ENCODER (svtvp9enc),
      sink_caps, filter);
  gst_caps_unref (sink_caps);

  return ret;
}

static gboolean
gst_svtvp9enc_sink_event (GstVideoEncoder * encoder, GstEvent * event)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "sink_event");

  return
      GST_VIDEO_ENCODER_CLASS (gst_svtvp9enc_parent_class)->sink_event
      (encoder, event);
}

static gboolean
gst_svtvp9enc_src_event (GstVideoEncoder * encoder, GstEvent * event)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "src_event");

  return
      GST_VIDEO_ENCODER_CLASS (gst_svtvp9enc_parent_class)->src_event (encoder,
      event);
}

static gboolean
gst_svtvp9enc_negotiate (GstVideoEncoder * encoder)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "negotiate");

  return
      GST_VIDEO_ENCODER_CLASS (gst_svtvp9enc_parent_class)->negotiate(encoder);
}

static gboolean
gst_svtvp9enc_decide_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "decide_allocation");

  return TRUE;
}

static gboolean
gst_svtvp9enc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  GstSvtVp9Enc *svtvp9enc = GST_SVTVP9ENC (encoder);

  GST_DEBUG_OBJECT (svtvp9enc, "propose_allocation");

  return TRUE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "svtvp9enc", GST_RANK_SECONDARY,
      GST_TYPE_SVTVP9ENC);
}

#ifndef VERSION
#define VERSION "1.0"
#endif
#ifndef PACKAGE
#define PACKAGE "gstreamer-svt-vp9"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "SVT-VP9 Encoder plugin for GStreamer"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://github.com/OpenVisualCloud"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    svtvp9enc,
    "Scalable Video Technology for VP9 Encoder (SVT-VP9 Encoder)",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
