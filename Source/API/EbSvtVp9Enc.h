/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSvtVp9Enc_h
#define EbSvtVp9Enc_h

#include <stdint.h>
#include <EbApiVersion.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define EB_IndexH265EncParam                    0x7F004000
#define EB_IndexH265DynEncParam                 0x7F004001
#define EB_MAX_TEMPORAL_LAYERS                  MAX_TEMPORAL_LAYERS
#define EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT     2
#define EB_HME_SEARCH_AREA_ROW_MAX_COUNT        2

/********************************
* Defines
********************************/
#define EB_ENCODERSTREAMPORT                    1
#define EB_ENCODERRECONPORT                     2
#define EB_ENCODERSTATISTICSPORT                3

#define EbBool                  uint8_t
#define EB_FALSE                0
#define EB_TRUE                 1

#define EB_BUFFERFLAG_EOS           0x00000001  // signals the last packet of the stream
#define EB_BUFFERFLAG_SHOW_EXT      0x00000002  // signals that the packet contains a show existing frame at the end

#ifdef _WIN32
#define EB_API __declspec(dllexport)
#else
#define EB_API __attribute__((visibility("default")))
#endif

#define EB_PICTURE           uint32_t
#define EB_B_PICTURE         0
#define EB_P_PICTURE         1
#define EB_I_PICTURE         2
#define EB_IDR_PICTURE       3
#define EB_NON_REF_PICTURE   4
#define EB_INVALID_PICTURE   0xFF

    typedef struct EbComponentType
    {
        uint32_t n_size;
        void* p_component_private;
        void* p_application_private;
    } EbComponentType;

    typedef struct EbSvtEncInput
    {
        // Hosts 8 bit or 16 bit input YUV420p / YUV420p10le
        uint8_t *luma;
        uint8_t *cb;
        uint8_t *cr;

        // Hosts LSB 2 bits of 10bit input when the compressed 10bit format is used
        uint8_t *luma_ext;
        uint8_t *cb_ext;
        uint8_t *cr_ext;

        uint32_t y_stride;
        uint32_t cr_stride;
        uint32_t cb_stride;

    } EbSvtEncInput;

    typedef struct EbBufferHeaderType
    {
        // EbBufferHeaderType size
        uint32_t size;

        // picture (input or output) buffer
        uint8_t* p_buffer;
        uint32_t n_filled_len;
        uint32_t n_alloc_len;

        // pic private data
        void*    p_app_private;
        void*    wrapper_ptr;

        // pic timing param
        uint32_t n_tick_count;
        int64_t  dts;
        int64_t  pts;

        // pic info
        uint32_t qp;
        uint32_t pic_type;

        // pic n_flags
        uint32_t flags;

    } EbBufferHeaderType;

    typedef enum EbErrorType
    {
        EB_ErrorNone = 0,
        EB_ErrorInsufficientResources = (int32_t)0x80001000,
        EB_ErrorUndefined = (int32_t)0x80001001,
        EB_ErrorComponentNotFound = (int32_t)0x80001003,
        EB_ErrorInvalidComponent = (int32_t)0x80001004,
        EB_ErrorBadParameter = (int32_t)0x80001005,
        EB_ErrorNotImplemented = (int32_t)0x80001006,
        EB_ErrorCreateThreadFailed = (int32_t)0x80002010,
        EB_ErrorThreadUnresponsive = (int32_t)0x80002011,
        EB_ErrorDestroyThreadFailed = (int32_t)0x80002012,
        EB_ErrorNullThread = (int32_t)0x80002013,
        EB_ErrorCreateSemaphoreFailed = (int32_t)0x80002020,
        EB_ErrorSemaphoreUnresponsive = (int32_t)0x80002021,
        EB_ErrorDestroySemaphoreFailed = (int32_t)0x80002022,
        EB_ErrorCreateMutexFailed = (int32_t)0x80002030,
        EB_ErrorMutexUnresponsive = (int32_t)0x80002031,
        EB_ErrorDestroyMutexFailed = (int32_t)0x80002032,
        EB_NoErrorEmptyQueue = (int32_t)0x80002033,
        EB_ErrorMax = 0x7FFFFFFF
    } EbErrorType;

// Will contain the EbEncApi which will live in the EncHandle class
// Only modifiable during config-time.
typedef struct EbSvtVp9EncConfiguration
{
    // Encoding preset
    /* A preset defining the quality vs density tradeoff point that the encoding
    * is to be performed at. 0 is the highest quality mode, 12 is the highest
    * density mode.
    * [0,  9] for all resolution and modes.
    * Default is 9. */
    uint8_t                  enc_mode;

    /* Encoder tuning for specific aim.
    * 0 = SQ - visually optimized mode.
    * 1 = OQ - PSNR / SSIM / VMAF optimized mode.
    * 2 = VMAF - VMAF optimized mode
    * Default is 1. */
    uint8_t                  tune;

    // GOP Structure
    /* The intra period defines the interval of frames after which you insert an
    * Intra refresh. It is strongly recommended to set the value to multiple of
    * 8 minus 1 the closest to 1 second (e.g. 55, 47, 31, 23 should be used for
    * 60, 50, 30, (24 or 25) respectively.
    * -1 = no intra update.
    * -2 = auto.
    * [-2 - 255]
    * Deault is -2. */
    int32_t                  intra_period;

    /* Prediction structure used to construct GOP. There are two main structures
    * supported, which are: Low Delay (P or B) and Random Access.
    *
    * In Low Delay structure, pictures within a mini GOP refer to the previously
    * encoded pictures in display order. In other words, pictures with display
    * order N can only be referenced by pictures with display order greater than
    * N, and it can only refer pictures with picture order lower than N. The Low
    * Delay structure can be flat structured (e.g. IPPPPPPP…) or hierarchically
    * structured. B/b pictures can be used instead of P/p pictures. However, the
    * reference picture list 0 and the reference picture list 1 will contain the
    * same reference picture.
    *
    * In Random Access structure, the B/b pictures can refer to reference pictures
    * from both directions (past and future).
    * [2]
    * Default is 2. */
    uint8_t                  pred_structure;

    /* Decides whether to use B picture or P picture in the base layer.
    * 0 = B Picture.
    * 1 = P Picture.
    * Default is 0. */
    uint32_t                 base_layer_switch_mode;

    // Input Info
    /* The width of input source in units of picture luma pixels.
    * [64 - 8192]
    * Default is 0. */
    uint32_t                 source_width;

    /* The height of input source in units of picture luma pixels.
    * [64 - 4320]
    * Default is 0. */
    uint32_t                 source_height;

    /* The frequecy of images being displayed. If the number is less than 1000,
    * the input frame rate is an integer number between 1 and 60, else the input
    * number is in Q16 format, shifted by 16 bits, where max allowed is 240 fps.
    * If FrameRateNumerator and FrameRateDenominator are both not equal to zero,
    * the encoder will ignore this parameter.
    * [0 - 2^64-1]
    * Default is 25. */
    uint32_t                 frame_rate;

    /* Frame rate numerator. When zero, the encoder will use –fps if
    * FrameRateDenominator is also zero, otherwise an error is returned.
    * [0 - 2^64-1]
    * Default is 0. */
    uint32_t                 frame_rate_numerator;

    /* Frame rate denominator. When zero, the encoder will use –fps if
    * FrameRateNumerator is also zero, otherwise an error is returned.
    * [0 - 2^64 - 1]
    * Default is 0. */
    uint32_t                 frame_rate_denominator;

    /* Specifies the bit depth of input video.
    * [8] = 8 bit.
    * Default is 8. */
    uint32_t                 encoder_bit_depth;

    /* The maximum partitioning depth with 0 being the superblock depth
    * Default is 4. */
    uint32_t                 partition_depth;

    // Quantization
    /* Initial quantization parameter for the Intra pictures used under constant
    * qp rate control mode.
    * [0 - 63]
    * Default is 50. */
    uint32_t                 qp;

    /* force qp values for every picture that are passed in the header pointer
    * Default is 0.*/
    EbBool                   use_qp_file;

    /* Enable picture QP scaling between hierarchical levels
    * Default is null.*/
    uint32_t                 enable_qp_scaling_flag;

    // Deblock Filter
    /* Flag to enable the Deblocking Loop Filtering.
    * Default is 1. */
    EbBool                   loop_filter;

    /* Flag to enable the use of default ME HME parameters.
    * Default is 1. */
    EbBool                   use_default_me_hme;

    /* Flag to enable Hierarchical Motion Estimation.
    * Default is 1. */
    EbBool                   enable_hme_flag;

    // ME Parameters
    /* Number of search positions in the horizontal direction.
    * [1 - 256]
    * Default depends on input resolution. */
    uint32_t                 search_area_width;

    /* Number of search positions in the vertical direction.
    * [1 - 256]
    * Default depends on input resolution. */
    uint32_t                 search_area_height;

    // Rate Control

    /* Rate control mode.
    * 0 = Constant QP.
    * 1 = Variable BitRate.
    * 2 = Constant BitRate.
    [0 - 2]
    * Default is 0. */
    uint32_t                 rate_control_mode;

    /* Target bitrate in bits/second, only apllicable when rate control mode is
    * set to 1.
    * Default is 7000000. */
    uint32_t                 target_bit_rate;
    /* Maxium QP value allowed for rate control use, only apllicable when rate
    * control mode is set to 1. It has to be greater or equal to minQpAllowed.
    * Default is 63. */

    uint32_t                 max_qp_allowed;
    /* Minimum QP value allowed for rate control use, only apllicable when rate
    * control mode is set to 1. It has to be smaller or equal to maxQpAllowed.
    * Default is 0. */

    uint32_t                 min_qp_allowed;

    /* Defined set of coding tools to create bitstream.
    * 0 = 8bit 4:2:0
    * Default is 0. */
    uint32_t                 profile;

    /* Constraints for bitstream in terms of max bitrate and max buffer size.
    * 0 = auto determination.
    * Default is 0. */
    uint32_t                 level;

    /* Assembly instruction set used by encoder.
    *
    * 0 = non-AVX2, C only.
    * 1 = up to AVX512, auto-select highest assembly insturction set supported.
    *
    * Default is 1. */
    uint32_t                 asm_type;

    // Application Specific parameters
    /* ID assigned to each channel when multiple instances are running within the
    * same application. */
    uint32_t                 channel_id;
    uint32_t                 active_channel_count;

    /* Flag to enable the Speed Control functionality to achieve the real-time
    * encoding speed defined by dynamically changing the encoding preset to meet
    * the average speed defined in injectorFrameRate. When this parameter is set
    * to 1 it forces –inj to be 1 -inj-frm-rt to be set to the –fps.
    * Default is 0. */
    uint32_t                 speed_control_flag;

    /* Frame Rate used for the injector. Recommended to match the encoder speed.
    * [1 - 240]
    * Default is 60. */
    int32_t                  injector_frame_rate;

    /* The number of logical processor which encoder threads run on. If
    * LogicalProcessorNumber and TargetSocket are not set, threads are managed by
    * OS thread scheduler. */
    uint32_t                logical_processors;

    /* Target socket to run on. For dual socket systems, this can specify which
     * socket the encoder runs on.
     *
     * -1 = Both Sockets.
     *  0 = Socket 0.
     *  1 = Socket 1.
     *
     * Default is -1. */
    int32_t                 target_socket;

    // Debug tools

    /* Output reconstructed yuv used for debug purposes. The value is set through
    * ReconFile token (-o) and using the feature will affect the speed of encoder.
    *
    * Default is 0. */
    uint32_t                 recon_file;

    // Input stride
    uint32_t               input_picture_stride; // Includes padding

    // VBV Parameters
    /* Sets the maximum rate the VBV buffer should be assumed to refill at
    *
    *  Default is 0. */
    uint32_t                    vbv_max_rate;

    /* Sets the size of the VBV buffer in bits.
    *
    *  Default is 0. */
    uint32_t                    vbv_buf_size;

    uint64_t                    frames_to_be_encoded;

} EbSvtVp9EncConfiguration;

/* STEP 1: Call the library to construct a Component Handle.
*
* Parameter:
* @ **p_handle      Handle to be called in the future for manipulating the
*                  component.
* @ *p_app_data      Callback data.
* @ *config_ptr     Pointer passed back to the client during callbacks, it will be
*                  loaded with default params from the library. */
EB_API EbErrorType eb_vp9_svt_init_handle(
    EbComponentType** p_handle,
    void* p_app_data,
    EbSvtVp9EncConfiguration  *config_ptr); // config_ptr will be loaded with default params from the library

/* STEP 2: Set all configuration parameters.
*
* Parameter:
* @ *svt_enc_component              Encoder handler.
* @ *pComponentParameterStructure  Encoder and buffer configurations will be copied to the library. */
EB_API EbErrorType eb_vp9_svt_enc_set_parameter(
    EbComponentType           *svt_enc_component,
    EbSvtVp9EncConfiguration    *p_component_parameter_structure); // pComponentParameterStructure contents will be copied to the library

/* STEP 3: Initialize encoder and allocates memory to necessary buffers.
*
* Parameter:
* @ *svt_enc_component  Encoder handler. */
EB_API EbErrorType eb_vp9_init_encoder(
    EbComponentType *svt_enc_component);

/* OPTIONAL: Get stream headers at init time.
*
* Parameter:
* @ *svt_enc_component   Encoder handler.
* @ **output_stream_ptr  Output buffer. */
EB_API EbErrorType eb_vp9_svt_enc_stream_header(
    EbComponentType           *svt_enc_component,
    EbBufferHeaderType       **output_stream_ptr);

/* OPTIONAL: Get the end of sequence Network Abstraction Layer.
*
* Parameter:
* @ *svt_enc_component  Encoder handler.
* @ **output_stream_ptr  Output stream. */
EB_API EbErrorType eb_vp9_svt_enc_eos_nal(
    EbComponentType           *svt_enc_component,
    EbBufferHeaderType       **output_stream_ptr);

/* STEP 4: Send the picture.
*
* Parameter:
* @ *svt_enc_component  Encoder handler.
* @ *p_buffer           Header pointer, picture buffer. */
EB_API EbErrorType eb_vp9_svt_enc_send_picture(
    EbComponentType      *svt_enc_component,
    EbBufferHeaderType   *p_buffer);

/* STEP 5: Receive packet.
* Parameter:
* @ *svt_enc_component  Encoder handler.
* @ **p_buffer          Header pointer to return packet with.
* @ pic_send_done       Flag to signal that all input pictures have been sent, this call becomes locking one this signal is 1.
* Non-locking call, returns EB_ErrorMax for an encode error, EB_NoErrorEmptyQueue when the library does not have any available packets.*/
EB_API EbErrorType eb_vp9_svt_get_packet(
    EbComponentType      *svt_enc_component,
    EbBufferHeaderType  **p_buffer,
    uint8_t                pic_send_done);

/* STEP 5-1: Release output buffer back into the pool.
*
* Parameter:
* @ **p_buffer          Header pointer that contains the output packet to be released. */
EB_API void eb_vp9_svt_release_out_buffer(
    EbBufferHeaderType  **p_buffer);

/* OPTIONAL: Fill buffer with reconstructed picture.
*
* Parameter:
* @ *svt_enc_component  Encoder handler.
* @ *p_buffer           Output buffer. */
EB_API EbErrorType eb_vp9_svt_get_recon(
    EbComponentType      *svt_enc_component,
    EbBufferHeaderType   *p_buffer);

/* STEP 6: Deinitialize encoder library.
*
* Parameter:
* @ *svt_enc_component  Encoder handler. */
EB_API EbErrorType eb_vp9_deinit_encoder(
    EbComponentType *svt_enc_component);

/* STEP 7: Deconstruct encoder handler.
*
* Parameter:
* @ *svt_enc_component  Encoder handler. */
EB_API EbErrorType eb_vp9_deinit_handle(
    EbComponentType  *svt_enc_component);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbSvtVp9Enc_h
