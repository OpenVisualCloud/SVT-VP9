/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EbAppContext.h"
#include "EbAppConfig.h"
#include "EbSvtVp9ErrorCodes.h"
#include "EbAppTime.h"

/***************************************
 * Macros
 ***************************************/
#define VP9_FOURCC 0x30395056
#define CLIP3(min_val, max_val, a)        (((a)<(min_val)) ? (min_val) : (((a)>(max_val)) ? (max_val) :(a)))
#define FUTURE_WINDOW_WIDTH                 4
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
extern volatile int keep_running;

/***************************************
* Process Error Log
***************************************/
static void log_error_output(
    FILE     *error_log_file,
    uint32_t  error_code) {

    switch (error_code) {

        // EB_ENC_ERRORS:
    case EB_ENC_ROB_OF_ERROR:
        fprintf(error_log_file, "Error: Recon Output Buffer Overflow!\n");
        break;

        // EB_ENC_PM_ERRORS:

    case EB_ENC_PM_ERROR0:
        fprintf(error_log_file, "Error: EbPictureManager: dependent_count underflow!\n");
        break;

    case EB_ENC_PM_ERROR8:
        fprintf(error_log_file, "Error: eb_vp9_PictureManagerKernel: referenceEntryPtr should never be null!\n");
        break;

    case EB_ENC_PM_ERROR1:
        fprintf(error_log_file, "Error: PictureManagerProcess: The dependent_count underflow detected!\n");
        break;

    case EB_ENC_PM_ERROR2:
        fprintf(error_log_file, "Error: PictureManagerProcess: Empty input queue!\n");
        break;

    case EB_ENC_PM_ERROR3:
        fprintf(error_log_file, "Error: PictureManagerProcess: Empty reference queue!\n");
        break;

    case EB_ENC_PM_ERROR4:
        fprintf(error_log_file, "Error: PictureManagerProcess: The capped elaspedNonIdrCount must be larger than the maximum supported delta ref poc!\n");
        break;

    case EB_ENC_PM_ERROR5:
        fprintf(error_log_file, "Error: PictureManagerProcess: Reference Picture Queue Full!\n");
        break;

    case EB_ENC_PM_ERROR6:
        fprintf(error_log_file, "Error: PictureManagerProcess: No reference match found - this will lead to a memory leak!\n");
        break;

    case EB_ENC_PM_ERROR7:
        fprintf(error_log_file, "Error: PictureManagerProcess: Unknown picture type!\n");
        break;

        // EB_ENC_RC_ERRORS:
    case EB_ENC_RC_ERROR0:
        fprintf(error_log_file, "Error: RateControlProcess: No RC interval found!\n");
        break;

        // picture decision Errors
    case EB_ENC_PD_ERROR7:
        fprintf(error_log_file, "Error: PictureDecisionProcess: Picture Decision Reorder Queue overflow\n");
        break;

    default:
        fprintf(error_log_file, "Error: Others!\n");
        break;
    }

    return;
}

/***************************************
 * Process Input Command
 ***************************************/
static inline void mem_put_le16(void *vmem, int32_t val) {
  uint8_t *mem = (uint8_t *)vmem;

  mem[0] = (uint8_t)((val >> 0) & 0xff);
  mem[1] = (uint8_t)((val >> 8) & 0xff);
}

static inline void mem_put_le32(void *vmem, int32_t val) {
  uint8_t *mem = (uint8_t *)vmem;

  mem[0] = (uint8_t)((val >>  0) & 0xff);
  mem[1] = (uint8_t)((val >>  8) & 0xff);
  mem[2] = (uint8_t)((val >> 16) & 0xff);
  mem[3] = (uint8_t)((val >> 24) & 0xff);
}

/******************************************************
* Copy fields from the stream to the input buffer
    Input   : stream
    Output  : valid input buffer
******************************************************/
void process_input_field_standard_mode(
    EbConfig              *config,
    EbBufferHeaderType    *header_ptr,
    FILE                  *input_file,
    uint8_t               *luma_input_ptr,
    uint8_t               *cb_input_ptr,
    uint8_t               *cr_input_ptr,
    unsigned char          is16bit)
{
    int64_t  input_padded_width = config->source_width;
    int64_t  input_padded_height = config->source_height;

    uint64_t  source_luma_row_size = (uint64_t)(input_padded_width << is16bit);

    uint64_t  source_chroma_row_size = source_luma_row_size >> 1;

    uint8_t  *eb_input_ptr;
    uint32_t  input_row_index;

    // Y
    eb_input_ptr = luma_input_ptr;
    // Skip 1 luma row if bottom field (point to the bottom field)
    if (config->processed_frame_count % 2 != 0)
        fseeko64(input_file, (long)source_luma_row_size, SEEK_CUR);

    for (input_row_index = 0; input_row_index < input_padded_height; input_row_index++) {

        header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, source_luma_row_size, input_file);
        // Skip 1 luma row (only fields)
        fseeko64(input_file, (long)source_luma_row_size, SEEK_CUR);
        eb_input_ptr += source_luma_row_size;
    }

    // U
    eb_input_ptr = cb_input_ptr;
    // Step back 1 luma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    if (config->processed_frame_count % 2 != 0) {
        fseeko64(input_file, -(long)source_luma_row_size, SEEK_CUR);
        fseeko64(input_file, (long)source_chroma_row_size, SEEK_CUR);
    }

    for (input_row_index = 0; input_row_index < input_padded_height >> 1; input_row_index++) {

        header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, source_chroma_row_size, input_file);
        // Skip 1 chroma row (only fields)
        fseeko64(input_file, (long)source_chroma_row_size, SEEK_CUR);
        eb_input_ptr += source_chroma_row_size;
    }

    // V
    eb_input_ptr = cr_input_ptr;
    // Step back 1 chroma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    // => no action

    for (input_row_index = 0; input_row_index < input_padded_height >> 1; input_row_index++) {

        header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, source_chroma_row_size, input_file);
        // Skip 1 chroma row (only fields)
        fseeko64(input_file, (long)source_chroma_row_size, SEEK_CUR);
        eb_input_ptr += source_chroma_row_size;
    }

    // Step back 1 chroma row if bottom field (undo the previous jump)
    if (config->processed_frame_count % 2 != 0) {
        fseeko64(input_file, -(long)source_chroma_row_size, SEEK_CUR);
    }
}

//************************************/
// get_next_qp_from_qp_file
// Reads and extracts one qp from the qp_file
// Input  : QP file
// Output : QP value
/************************************/
static int qpReadFromFile = 0;

int32_t get_next_qp_from_qp_file(
    EbConfig  *config){

    uint8_t *line;
    int32_t qp = 0;
    uint32_t read_size = 0, eof = 0;
    EB_APP_MALLOC(uint8_t*, line, 8, EB_N_PTR, EB_ErrorInsufficientResources);
    memset(line,0,8);
    read_size = (uint32_t)fread(line, 1, 2, config->qp_file);

    if (read_size == 0) {
        // end of file
        return -1;
    }
    else if (read_size == 1) {
        qp = strtol((const char*)line, NULL, 0);
        if (qp == 0) // eof
            qp = -1;
    }
    else if (read_size == 2 && (line[0] == '\n')) {
        // new line
        fseek(config->qp_file, -1, SEEK_CUR);
        qp = 0;
    }
    else if (read_size == 2 && (line[1] == '\n')) {
        // new line
        qp = strtol((const char*)line, NULL, 0);
    }
    else if (read_size == 2 && (line[0] == '#' || line[0] == '/' || line[0] == '-' || line[0] == ' ')) {
        // Backup one step to not miss the new line char
        fseek(config->qp_file, -1, SEEK_CUR);
        do {
            read_size = (uint32_t)fread(line, 1, 1, config->qp_file);
            if (read_size != 1)
                break;
        } while (line[0] != '\n');

        if (eof != 0)
            // end of file
            qp = -1;
        else
            // skip line
            qp = 0;
    }
    else if (read_size == 2) {
        qp = strtol((const char*)line, NULL, 0);
        do {
            read_size = (uint32_t)fread(line, 1, 1, config->qp_file);
            if (read_size != 1)
                break;
        } while (line[0] != '\n');
    }

    if (qp > 0)
        qpReadFromFile |= 1;

    return qp;
}

void read_input_frames(
    EbConfig           *config,
    unsigned char       is16bit,
    EbBufferHeaderType *header_ptr){

    uint64_t  read_size;
    int64_t   input_padded_width  = config->source_width;
    int64_t   input_padded_height = config->source_height;

    FILE    *input_file           = config->input_file;
    uint8_t *eb_input_ptr;
    EbSvtEncInput* input_ptr      = (EbSvtEncInput*)header_ptr->p_buffer;

    input_ptr->y_stride  = input_padded_width;
    input_ptr->cr_stride = input_padded_width >> 1;
    input_ptr->cb_stride = input_padded_width >> 1;

    if (config->buffered_input == -1) {

        if (is16bit == 0 || is16bit == 1) {

            read_size = (uint64_t)SIZE_OF_ONE_FRAME_IN_BYTES(input_padded_width, input_padded_height, is16bit);

            header_ptr->n_filled_len = 0;

            uint64_t luma_read_size = (uint64_t)input_padded_width*input_padded_height << is16bit;
            eb_input_ptr = input_ptr->luma;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size, input_file);
            eb_input_ptr = input_ptr->cb;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
            eb_input_ptr = input_ptr->cr;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
            input_ptr->luma = input_ptr->luma + (0 << is16bit);
            input_ptr->cb = input_ptr->cb + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)) << is16bit);
            input_ptr->cr = input_ptr->cr + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)) << is16bit);

            if (read_size != header_ptr->n_filled_len) {

                fseek(input_file, 0, SEEK_SET);
                eb_input_ptr = input_ptr->luma;
                header_ptr->n_filled_len = (uint32_t)fread(eb_input_ptr, 1, luma_read_size, input_file);
                eb_input_ptr = input_ptr->cb;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
                eb_input_ptr = input_ptr->cr;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);

                input_ptr->cb = input_ptr->cb + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));
                input_ptr->cr = input_ptr->cr + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));

            }
        }

        // 10-bit Unpacked Mode
        else {

            read_size = (uint64_t)SIZE_OF_ONE_FRAME_IN_BYTES(input_padded_width, input_padded_height, 1);

            header_ptr->n_filled_len = 0;

            uint64_t luma_read_size = (uint64_t)input_padded_width*input_padded_height;

            eb_input_ptr = input_ptr->luma;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size, input_file);
            eb_input_ptr = input_ptr->cb;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
            eb_input_ptr = input_ptr->cr;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);

            input_ptr->cb = input_ptr->cb + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));
            input_ptr->cr = input_ptr->cr + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));

            eb_input_ptr = input_ptr->luma_ext;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size, input_file);
            eb_input_ptr = input_ptr->cb_ext;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
            eb_input_ptr = input_ptr->cr_ext;
            header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);

            input_ptr->cb_ext = input_ptr->cb_ext + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));
            input_ptr->cr_ext = input_ptr->cr_ext + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));

            if (read_size != header_ptr->n_filled_len) {

                fseek(input_file, 0, SEEK_SET);
                eb_input_ptr = input_ptr->luma;
                header_ptr->n_filled_len = (uint32_t)fread(eb_input_ptr, 1, luma_read_size, input_file);
                eb_input_ptr = input_ptr->cb;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
                eb_input_ptr = input_ptr->cr;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);

                input_ptr->cb = input_ptr->cb + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));
                input_ptr->cr = input_ptr->cr + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));

                eb_input_ptr = input_ptr->luma_ext;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size, input_file);
                eb_input_ptr = input_ptr->cb_ext;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);
                eb_input_ptr = input_ptr->cr_ext;
                header_ptr->n_filled_len += (uint32_t)fread(eb_input_ptr, 1, luma_read_size >> 2, input_file);

                input_ptr->cb_ext = input_ptr->cb_ext + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));
                input_ptr->cr_ext = input_ptr->cr_ext + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)));

            }
        }
    }
    else {

        const int ten_bit_packed_mode = (config->encoder_bit_depth > 8) ? 1 : 0;

        // Determine size of each plane
        const size_t luma8bit_size =
            (config->source_width) *
            (config->source_height) *
            (1 << ten_bit_packed_mode);

        const size_t chroma8bit_size = luma8bit_size >> 2;

        const size_t luma10bit_size = (config->encoder_bit_depth > 8 && ten_bit_packed_mode == 0) ? luma8bit_size : 0;
        const size_t chroma10bit_size = (config->encoder_bit_depth > 8 && ten_bit_packed_mode == 0) ? chroma8bit_size : 0;

        EbSvtEncInput* input_ptr = (EbSvtEncInput*)header_ptr->p_buffer;

        input_ptr->y_stride = config->source_width;
        input_ptr->cr_stride = config->source_width >> 1;
        input_ptr->cb_stride = config->source_width >> 1;
        input_ptr->luma = config->sequence_buffer[config->processed_frame_count % config->buffered_input];
        input_ptr->cb = config->sequence_buffer[config->processed_frame_count % config->buffered_input] + luma8bit_size;
        input_ptr->cr = config->sequence_buffer[config->processed_frame_count % config->buffered_input] + luma8bit_size + chroma8bit_size;
        input_ptr->luma = input_ptr->luma + (0 << ten_bit_packed_mode);
        input_ptr->cb = input_ptr->cb + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)) << ten_bit_packed_mode);
        input_ptr->cr = input_ptr->cr + (((config->source_width >> 1)*(0 >> 1) + (0 >> 1)) << ten_bit_packed_mode);

        if (is16bit) {
            input_ptr->luma_ext = config->sequence_buffer[config->processed_frame_count % config->buffered_input] + luma8bit_size + 2 * chroma8bit_size;
            input_ptr->cb_ext = config->sequence_buffer[config->processed_frame_count % config->buffered_input] + luma8bit_size + 2 * chroma8bit_size + luma10bit_size;
            input_ptr->cr_ext = config->sequence_buffer[config->processed_frame_count % config->buffered_input] + luma8bit_size + 2 * chroma8bit_size + luma10bit_size + chroma10bit_size;
            input_ptr->cb_ext = input_ptr->cb_ext + (config->source_width >> 1)*(0 >> 1) + (0 >> 1);
            input_ptr->cr_ext = input_ptr->cr_ext + (config->source_width >> 1)*(0 >> 1) + (0 >> 1);

        }
        header_ptr->n_filled_len = (uint32_t)(uint64_t)SIZE_OF_ONE_FRAME_IN_BYTES(input_padded_width, input_padded_height, is16bit);
    }

    // If we reached the end of file, loop over again
    if (feof(input_file) != 0) {
        fseek(input_file, 0, SEEK_SET);
    }

    return;
}

void send_qp_on_the_fly(
    EbConfig           *config,
    EbBufferHeaderType *header_ptr){
    {
        uint32_t qp_ptr;
        int32_t  tmp_qp = 0;

        do {
            // get next qp
            tmp_qp = get_next_qp_from_qp_file(config);

            if (tmp_qp == (int32_t)EB_ErrorInsufficientResources) {
                printf("Malloc has failed due to insuffucient resources");
                return;
            }

            // check if eof
            if ((tmp_qp == -1) && (qpReadFromFile != 0))
                fseek(config->qp_file, 0, SEEK_SET);

            // check if the qp read is valid
            else if (tmp_qp > 0)
                break;

        } while (tmp_qp == 0 || ((tmp_qp == -1) && (qpReadFromFile != 0)));

        if (tmp_qp == -1) {
            config->use_qp_file = EB_FALSE;
            printf("\nWarning: QP File did not contain any valid QPs");
        }

        qp_ptr = CLIP3(0, 63, tmp_qp);

        header_ptr->qp = qp_ptr;
    }
    return;
}

static void app_svt_vp9_injector(uint64_t processed_frame_count,
                             uint32_t injector_frame_rate) {
    static uint64_t start_times_seconds;
    static uint64_t start_timesu_seconds;
    static int first_time = 0;

    if (first_time == 0) {
        first_time = 1;
        app_svt_vp9_get_time(&start_times_seconds, &start_timesu_seconds);
    } else {
        uint64_t current_times_seconds, current_timesu_seconds;
        app_svt_vp9_get_time(&current_times_seconds, &current_timesu_seconds);
        const double elapsed_time = app_svt_vp9_compute_overall_elapsed_time(
            start_times_seconds, start_timesu_seconds, current_times_seconds,
            current_timesu_seconds);
        const int buffer_frames =
            1;  // How far ahead of time should we let it get
        const double injector_interval =
            (double)(1 << 16) /
            injector_frame_rate;  // 1.0 / injector frame rate (in this
                                  // case, 1.0/encodRate)
        const double predicted_time =
            (processed_frame_count - buffer_frames) * injector_interval;
        const int milli_sec_ahead =
            (int)(1000 * (predicted_time - elapsed_time));
        if (milli_sec_ahead > 0) app_svt_vp9_sleep(milli_sec_ahead);
    }
}

//************************************/
// process_input_buffer
// Reads yuv frames from file and copy
// them into the input buffer
/************************************/
AppExitConditionType process_input_buffer(
    EbConfig     *config,
    EbAppContext *app_call_back)
{
    uint8_t                 is16bit = (uint8_t)(config->encoder_bit_depth > 8);
    EbBufferHeaderType     *header_ptr = app_call_back->input_buffer_pool;
    EbComponentType        *component_handle = (EbComponentType*)app_call_back->svt_encoder_handle;

    AppExitConditionType    return_value = APP_ExitConditionNone;

    int64_t                 input_padded_width           = config->source_width;
    int64_t                 input_padded_height          = config->source_height;
    int64_t                 frames_to_be_encoded         = config->frames_to_be_encoded;
    int64_t                 total_bytes_to_process_count;
    int64_t                 remaining_byte_count;

    if (config->injector && config->processed_frame_count)
        app_svt_vp9_injector(config->processed_frame_count, config->injector_frame_rate);

    total_bytes_to_process_count = (frames_to_be_encoded < 0) ? -1 :
        frames_to_be_encoded * SIZE_OF_ONE_FRAME_IN_BYTES(input_padded_width, input_padded_height, is16bit);

    remaining_byte_count       = (total_bytes_to_process_count < 0) ?   -1 :  total_bytes_to_process_count - (int64_t)config->processed_byte_count;

    // If there are bytes left to encode, configure the header
    if (remaining_byte_count != 0 && config->stop_encoder == EB_FALSE) {

        read_input_frames(
            config,
            is16bit,
            header_ptr);

        // Update the context parameters
        config->processed_byte_count += header_ptr->n_filled_len;
        header_ptr->p_app_private          = (EbPtr)EB_NULL;
        ++config->processed_frame_count;

        // Configuration parameters changed on the fly
        if (config->use_qp_file && config->qp_file)
            send_qp_on_the_fly(
                config,
                header_ptr);

        if (keep_running == 0 && !config->stop_encoder) {
            config->stop_encoder = EB_TRUE;
        }

        // Fill in Buffers Header control data
        header_ptr->pts          = config->processed_frame_count-1;
        header_ptr->pic_type     = EB_INVALID_PICTURE;

        header_ptr->flags = 0;

        // Send the picture
        eb_vp9_svt_enc_send_picture(component_handle, header_ptr);

        if ((config->processed_frame_count == (uint64_t)config->frames_to_be_encoded) || config->stop_encoder) {

            header_ptr->n_alloc_len    = 0;
            header_ptr->n_filled_len   = 0;
            header_ptr->n_tick_count   = 0;
            header_ptr->p_app_private  = NULL;
            header_ptr->flags        = EB_BUFFERFLAG_EOS;
            header_ptr->p_buffer       = NULL;
            header_ptr->pic_type       = EB_INVALID_PICTURE;

            eb_vp9_svt_enc_send_picture(component_handle, header_ptr);

        }

        return_value = (header_ptr->flags == EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : return_value;

    }

    return return_value;
}

/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

static void write_ivf_stream_header(EbConfig *config)
{
    char header[32];
    header[0] = 'D';
    header[1] = 'K';
    header[2] = 'I';
    header[3] = 'F';
    mem_put_le16(header + 4, 0);                     // version
    mem_put_le16(header + 6, 32);                    // header size
    mem_put_le32(header + 8, VP9_FOURCC);                // fourcc
    mem_put_le16(header + 12, config->source_width);  // width
    mem_put_le16(header + 14, config->source_height); // height
    if (config->frame_rate_denominator!= 0 && config->frame_rate_numerator !=0){
        mem_put_le32(header + 16, config->frame_rate_numerator);  // rate
        mem_put_le32(header + 20, config->frame_rate_denominator);// scale
    }else{
        mem_put_le32(header + 16, (config->frame_rate >> 16) * 1000);  // rate
        mem_put_le32(header + 20, 1000);  // scale
    }

    mem_put_le32(header + 24, 0);               // length
    mem_put_le32(header + 28, 0);               // unused
    //config->performance_context.byte_count += 32;
    if (config->bitstream_file)
        fwrite(header, 1, 32, config->bitstream_file);

    return;
}

// IVF data: 12-byte header
// bytes 0 -  3 :   size of frame in bytes(not including the 12 - byte header)
// bytes 4 - 11 :   64 - bit presentation timestamp
static void write_ivf_frame_header(EbConfig *config, uint32_t byte_count, uint64_t  pts){
    char header[12];
    int32_t write_location = 0;

    mem_put_le32(&header[write_location], (int32_t)byte_count);
    write_location = write_location + 4;
    mem_put_le32(&header[write_location], (int32_t)((pts) & 0xFFFFFFFF));
    write_location = write_location + 4;
    mem_put_le32(&header[write_location], (int32_t)((pts) >> 32));
    write_location = write_location + 4;
    //config->performance_context.byte_count += write_location;

    if (config->bitstream_file)
        fwrite(header, 1, 12, config->bitstream_file);
}

#define OBU_FRAME_HEADER_SIZE       1
#define TD_SPS_SIZE                 17
#define LONG_ENCODE_FRAME_ENCODE    4000
#define SPEED_MEASUREMENT_INTERVAL  2000
#define START_STEADY_STATE          1000
AppExitConditionType process_output_stream_buffer(
    EbConfig             *config,
    EbAppContext         *app_call_back,
    uint8_t               pic_send_done)
{
    AppPortActiveType      *port_state       = &app_call_back->output_stream_port_active;
    EbBufferHeaderType     *header_ptr;
    EbComponentType        *component_handle = (EbComponentType*)app_call_back->svt_encoder_handle;
    AppExitConditionType    return_value    = APP_ExitConditionNone;
    EbErrorType             stream_status   = EB_ErrorNone;
    // Per channel variables
    FILE                   *stream_file       = config->bitstream_file;

    uint64_t               *total_latency     = &config->performance_context.total_latency;
    uint32_t               *max_latency       = &config->performance_context.max_latency;

    // System performance variables
    static int32_t         frame_count                = 0;

    // Local variables
    uint64_t               finishs_time     = 0;
    uint64_t               finishu_time     = 0;

    // non-blocking call until all input frames are sent
    stream_status = eb_vp9_svt_get_packet(component_handle, &header_ptr, pic_send_done);

    if (stream_status == EB_ErrorMax) {
        printf("\n");
        log_error_output(
            config->error_log_file,
            header_ptr->flags);
        return APP_ExitConditionError;
    }
    else if (stream_status != EB_NoErrorEmptyQueue) {
        ++(config->performance_context.frame_count);
        *total_latency += header_ptr->n_tick_count;
        *max_latency = (header_ptr->n_tick_count > *max_latency) ? header_ptr->n_tick_count : *max_latency;

        app_svt_vp9_get_time(&finishs_time, &finishu_time);

        // total execution time, inc init time
        config->performance_context.total_execution_time =
            app_svt_vp9_compute_overall_elapsed_time(
                config->performance_context.lib_start_time[0],
                config->performance_context.lib_start_time[1], finishs_time,
                finishu_time);

        // total encode time
        config->performance_context.total_encode_time =
            app_svt_vp9_compute_overall_elapsed_time(
                config->performance_context.encode_start_time[0],
                config->performance_context.encode_start_time[1], finishs_time,
                finishu_time);

        // Write Stream Data to file
        if (stream_file) {
            if (config->performance_context.frame_count == 1){
                write_ivf_stream_header(config);
            }

            if (header_ptr->flags & EB_BUFFERFLAG_SHOW_EXT){
                write_ivf_frame_header(config, header_ptr->n_filled_len  - (OBU_FRAME_HEADER_SIZE * 4), header_ptr->pts);
                fwrite(header_ptr->p_buffer, 1, header_ptr->n_filled_len - (OBU_FRAME_HEADER_SIZE * 4), stream_file);
                // 4 frame headers with an IVF frame header for each
                write_ivf_frame_header(config, OBU_FRAME_HEADER_SIZE, (header_ptr->pts - 2));
                fwrite(header_ptr->p_buffer + header_ptr->n_filled_len - (OBU_FRAME_HEADER_SIZE * 4), 1, OBU_FRAME_HEADER_SIZE, stream_file);
                write_ivf_frame_header(config, OBU_FRAME_HEADER_SIZE, (header_ptr->pts - 1));
                fwrite(header_ptr->p_buffer + header_ptr->n_filled_len - (OBU_FRAME_HEADER_SIZE * 3), 1, OBU_FRAME_HEADER_SIZE, stream_file);
                write_ivf_frame_header(config, OBU_FRAME_HEADER_SIZE, (header_ptr->pts));
                fwrite(header_ptr->p_buffer + header_ptr->n_filled_len - (OBU_FRAME_HEADER_SIZE * 2), 1, OBU_FRAME_HEADER_SIZE, stream_file);
                write_ivf_frame_header(config, OBU_FRAME_HEADER_SIZE, (header_ptr->pts + 1));
                fwrite(header_ptr->p_buffer + header_ptr->n_filled_len - OBU_FRAME_HEADER_SIZE, 1, OBU_FRAME_HEADER_SIZE, stream_file);
            }else{
                write_ivf_frame_header(config, header_ptr->n_filled_len, header_ptr->pts);
                fwrite(header_ptr->p_buffer, 1, header_ptr->n_filled_len, stream_file);
            }
        }
        config->performance_context.byte_count += header_ptr->n_filled_len;

        // Update Output Port Activity State
        *port_state = (header_ptr->flags & EB_BUFFERFLAG_EOS) ? APP_PortInactive : *port_state;
        return_value = (header_ptr->flags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;

        // Release the output buffer
        eb_vp9_svt_release_out_buffer(&header_ptr);

#if DEADLOCK_DEBUG
        ++frame_count;
#else
        //++frame_count;
        printf("\b\b\b\b\b\b\b\b\b%9d", ++frame_count);
#endif

        //++frame_count;
        fflush(stdout);

        config->performance_context.average_speed =
            config->performance_context.frame_count /
            (double)config->performance_context.total_encode_time;
        config->performance_context.average_latency =
            (double)config->performance_context.total_latency /
            config->performance_context.frame_count;

        if (!(frame_count % SPEED_MEASUREMENT_INTERVAL)) {
            {
                printf("\n");
                printf("Average System Encoding Speed:        %.2f\n", (double)(frame_count) / config->performance_context.total_encode_time);
            }
        }
    }
    return return_value;
}

AppExitConditionType process_output_recon_buffer(
    EbConfig     *config,
    EbAppContext *app_call_back){

    EbBufferHeaderType     *header_ptr       = app_call_back->recon_buffer; // needs to change for buffered input
    EbComponentType        *component_handle = (EbComponentType*)app_call_back->svt_encoder_handle;
    AppExitConditionType    return_value     = APP_ExitConditionNone;
    EbErrorType             recon_status     = EB_ErrorNone;
    int32_t                 fseek_return_val;
    // non-blocking call until all input frames are sent
    recon_status = eb_vp9_svt_get_recon(component_handle, header_ptr);

    if (recon_status == EB_ErrorMax) {
        printf("\n");
        log_error_output(
            config->error_log_file,
            header_ptr->flags);
        return APP_ExitConditionError;
    }
    else if (recon_status != EB_NoErrorEmptyQueue) {
        //Sets the File position to the beginning of the file.
        rewind(config->recon_file);
        uint64_t frameNum = header_ptr->pts;
        while (frameNum>0) {
            fseek_return_val = fseeko64(config->recon_file, header_ptr->n_filled_len, SEEK_CUR);

            if (fseek_return_val != 0) {
                printf("Error in fseeko64  returnVal %i\n", fseek_return_val);
                return APP_ExitConditionError;
            }
            frameNum = frameNum - 1;
        }

        fwrite(header_ptr->p_buffer, 1, header_ptr->n_filled_len, config->recon_file);

        // Update Output Port Activity State
        return_value = (header_ptr->flags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;
    }
    return return_value;
}
