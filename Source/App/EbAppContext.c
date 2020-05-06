/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/

#include <stdlib.h>

#include "EbAppContext.h"
#include "EbAppConfig.h"

#define INPUT_SIZE_576p_TH                0x90000        // 0.58 Million
#define INPUT_SIZE_1080i_TH                0xB71B0        // 0.75 Million
#define INPUT_SIZE_1080p_TH                0x1AB3F0    // 1.75 Million
#define INPUT_SIZE_4K_TH                0x29F630    // 2.75 Million

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
#define IS_16_BIT(bit_depth) (bit_depth==10?1:0)
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)                ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0  )

 /***************************************
 * Variables Defining a memory table
 *  hosting all allocated pointers
 ***************************************/
EbMemoryMapEntry                 *app_memory_map;
uint32_t                         *app_memory_map_index;
uint64_t                         *total_app_memory;
uint32_t                          app_malloc_count = 0;
static EbMemoryMapEntry          *app_memory_map_all_channels[MAX_CHANNEL_NUMBER];
static uint32_t                   app_memory_map_index_all_channels[MAX_CHANNEL_NUMBER];
static uint64_t                   app_memory_mallocd_all_channels[MAX_CHANNEL_NUMBER];

/***************************************
* Allocation and initializing a memory table
*  hosting all allocated pointers
***************************************/
void allocate_memory_table(
    uint32_t    instance_idx)
{
    // Malloc Memory Table for the instance @ instance_idx
    app_memory_map_all_channels[instance_idx]        = (EbMemoryMapEntry*)malloc(sizeof(EbMemoryMapEntry) * MAX_APP_NUM_PTR);

    // Init the table index
    app_memory_map_index_all_channels[instance_idx]   = 0;

    // Size of the table
    app_memory_mallocd_all_channels[instance_idx]    = sizeof(EbMemoryMapEntry) * MAX_APP_NUM_PTR;
    total_app_memory = &app_memory_mallocd_all_channels[instance_idx];

    // Set pointer to the first entry
    app_memory_map                                = app_memory_map_all_channels[instance_idx];

    // Set index to the first entry
    app_memory_map_index                           = &app_memory_map_index_all_channels[instance_idx];

    // Init Number of pointers
    app_malloc_count = 0;

    return;
}

/*************************************
**************************************
*** Helper functions Input / Output **
**************************************
**************************************/
/******************************************************
* Copy fields from the stream to the input buffer
Input   : stream
Output  : valid input buffer
******************************************************/
void process_input_field_buffering_mode(
    uint64_t      processed_frame_count,
    int          *filled_len,
    FILE         *input_file,
    uint8_t      *luma_input_ptr,
    uint8_t      *cb_input_ptr,
    uint8_t      *cr_input_ptr,
    uint32_t      input_padded_width,
    uint32_t      input_padded_height,
    unsigned char is16bit)
{
    uint64_t  source_luma_row_size = (uint64_t)(input_padded_width << is16bit);
    uint64_t  source_chroma_row_size = source_luma_row_size >> 1;

    uint8_t  *eb_input_ptr;
    uint32_t  input_row_index;

    // Y
    eb_input_ptr = luma_input_ptr;
    // Skip 1 luma row if bottom field (point to the bottom field)
    if (processed_frame_count % 2 != 0)
        fseeko64(input_file, (long)source_luma_row_size, SEEK_CUR);

    for (input_row_index = 0; input_row_index < input_padded_height; input_row_index++) {

        *filled_len += (uint32_t)fread(eb_input_ptr, 1, source_luma_row_size, input_file);
        // Skip 1 luma row (only fields)
        fseeko64(input_file, (long)source_luma_row_size, SEEK_CUR);
        eb_input_ptr += source_luma_row_size;
    }

    // U
    eb_input_ptr = cb_input_ptr;
    // Step back 1 luma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    if (processed_frame_count % 2 != 0) {
        fseeko64(input_file, -(long)source_luma_row_size, SEEK_CUR);
        fseeko64(input_file, (long)source_chroma_row_size, SEEK_CUR);
    }

    for (input_row_index = 0; input_row_index < input_padded_height >> 1; input_row_index++) {

        *filled_len += (uint32_t)fread(eb_input_ptr, 1, source_chroma_row_size, input_file);
        // Skip 1 chroma row (only fields)
        fseeko64(input_file, (long)source_chroma_row_size, SEEK_CUR);
        eb_input_ptr += source_chroma_row_size;
    }

    // V
    eb_input_ptr = cr_input_ptr;
    // Step back 1 chroma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    // => no action

    for (input_row_index = 0; input_row_index < input_padded_height >> 1; input_row_index++) {

        *filled_len += (uint32_t)fread(eb_input_ptr, 1, source_chroma_row_size, input_file);
        // Skip 1 chroma row (only fields)
        fseeko64(input_file, (long)source_chroma_row_size, SEEK_CUR);
        eb_input_ptr += source_chroma_row_size;
    }

    // Step back 1 chroma row if bottom field (undo the previous jump)
    if (processed_frame_count % 2 != 0) {
        fseeko64(input_file, -(long)source_chroma_row_size, SEEK_CUR);
    }
}

/***********************************************
* Copy configuration parameters from
*  The config structure, to the
*  callback structure to send to the library
***********************************************/
EbErrorType  copy_configuration_parameters(
    EbConfig     *config,
    EbAppContext *callback_data,
    uint32_t      instance_idx)
{
    EbErrorType   return_error = EB_ErrorNone;

    // Assign Instance index to the library
    callback_data->eb_enc_parameters.channel_id = (uint8_t)instance_idx;

    // Initialize Port Activity Flags
    callback_data->output_stream_port_active = APP_PortActive;
    callback_data->eb_enc_parameters.source_width = config->source_width;
    callback_data->eb_enc_parameters.source_height = config->source_height;
    callback_data->eb_enc_parameters.intra_period = config->intra_period;
    callback_data->eb_enc_parameters.base_layer_switch_mode = config->base_layer_switch_mode;
    callback_data->eb_enc_parameters.enc_mode = (uint8_t)config->enc_mode;
    callback_data->eb_enc_parameters.frame_rate = config->frame_rate;
    callback_data->eb_enc_parameters.frame_rate_denominator = config->frame_rate_denominator;
    callback_data->eb_enc_parameters.frame_rate_numerator = config->frame_rate_numerator;
    callback_data->eb_enc_parameters.pred_structure = (uint8_t)config->pred_structure;
    callback_data->eb_enc_parameters.rate_control_mode = config->rate_control_mode;
    callback_data->eb_enc_parameters.target_bit_rate = config->target_bit_rate;
    callback_data->eb_enc_parameters.max_qp_allowed = config->max_qp_allowed;
    callback_data->eb_enc_parameters.min_qp_allowed = config->min_qp_allowed;
    callback_data->eb_enc_parameters.qp = config->qp;
    callback_data->eb_enc_parameters.vbv_max_rate = config->vbv_max_rate;
    callback_data->eb_enc_parameters.vbv_buf_size = config->vbv_buf_size;
    callback_data->eb_enc_parameters.frames_to_be_encoded = config->frames_to_be_encoded;
    callback_data->eb_enc_parameters.use_qp_file = (uint8_t)config->use_qp_file;
    callback_data->eb_enc_parameters.loop_filter = (uint8_t)config->loop_filter;
    callback_data->eb_enc_parameters.use_default_me_hme = (uint8_t)config->use_default_me_hme;
    callback_data->eb_enc_parameters.enable_hme_flag = (uint8_t)config->enable_hme_flag;
    callback_data->eb_enc_parameters.search_area_width = config->search_area_width;
    callback_data->eb_enc_parameters.search_area_height = config->search_area_height;
    callback_data->eb_enc_parameters.tune = config->tune;
    callback_data->eb_enc_parameters.recon_file = (config->recon_file) ? (uint32_t)EB_TRUE : (uint32_t)EB_FALSE;
    callback_data->eb_enc_parameters.channel_id = config->channel_id;
    callback_data->eb_enc_parameters.active_channel_count = config->active_channel_count;
    callback_data->eb_enc_parameters.encoder_bit_depth = config->encoder_bit_depth;
    callback_data->eb_enc_parameters.profile = config->profile;
    callback_data->eb_enc_parameters.level = config->level;
    callback_data->eb_enc_parameters.injector_frame_rate = config->injector_frame_rate;
    callback_data->eb_enc_parameters.speed_control_flag = config->speed_control_flag;
    callback_data->eb_enc_parameters.asm_type = config->asm_type;
    callback_data->eb_enc_parameters.logical_processors = config->logical_processors;
    callback_data->eb_enc_parameters.target_socket = config->target_socket;

    return return_error;

}

static EbErrorType  allocate_frame_buffer(
    EbConfig *config,
    uint8_t  *p_buffer)
{
    EbErrorType    return_error = EB_ErrorNone;

    const int ten_bit_packed_mode = (config->encoder_bit_depth > 8) ? 1 : 0;

    // Determine size of each plane
    const size_t luma8bit_size =

        config->source_width    *
        config->source_height   *

        (1 << ten_bit_packed_mode);

    const size_t chroma8bit_size = luma8bit_size >> 2;
    const size_t luma10bit_size = (config->encoder_bit_depth > 8 && ten_bit_packed_mode == 0) ? luma8bit_size : 0;
    const size_t chroma10bit_size = (config->encoder_bit_depth > 8 && ten_bit_packed_mode == 0) ? chroma8bit_size : 0;

    // Determine
    EbSvtEncInput* input_ptr = (EbSvtEncInput*)p_buffer;

    input_ptr->y_stride = config->source_width;
    input_ptr->cr_stride = config->source_width >> 1;
    input_ptr->cb_stride = config->source_width >> 1;

    if (luma8bit_size) {
        EB_APP_MALLOC(unsigned char*, input_ptr->luma, luma8bit_size, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        input_ptr->luma = 0;
    }
    if (chroma8bit_size) {
        EB_APP_MALLOC(unsigned char*, input_ptr->cb, chroma8bit_size, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        input_ptr->cb = 0;
    }

    if (chroma8bit_size) {
        EB_APP_MALLOC(unsigned char*, input_ptr->cr, chroma8bit_size, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        input_ptr->cr = 0;
    }

    if (luma10bit_size) {
        EB_APP_MALLOC(unsigned char*, input_ptr->luma_ext, luma10bit_size, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        input_ptr->luma_ext = 0;
    }

    if (chroma10bit_size) {
        EB_APP_MALLOC(unsigned char*, input_ptr->cb_ext, chroma10bit_size, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        input_ptr->cb_ext = 0;
    }

    if (chroma10bit_size) {
        EB_APP_MALLOC(unsigned char*, input_ptr->cr_ext, chroma10bit_size, EB_N_PTR, EB_ErrorInsufficientResources);

    }
    else {
        input_ptr->cr_ext = 0;
    }

    return return_error;
}

EbErrorType AllocateInputBuffers(
    EbConfig     *config,
    EbAppContext *callback_data)
{
    EbErrorType   return_error = EB_ErrorNone;
    {
        EB_APP_MALLOC(EbBufferHeaderType*, callback_data->input_buffer_pool, sizeof(EbBufferHeaderType), EB_N_PTR, EB_ErrorInsufficientResources);

        // Initialize Header
        callback_data->input_buffer_pool->size                       = sizeof(EbBufferHeaderType);

        EB_APP_MALLOC(uint8_t*, callback_data->input_buffer_pool->p_buffer, sizeof(EbSvtEncInput), EB_N_PTR, EB_ErrorInsufficientResources);

        if (config->buffered_input == -1) {

            // Allocate frame buffer for the p_buffer
            allocate_frame_buffer(
                    config,
                    callback_data->input_buffer_pool->p_buffer);
        }

        // Assign the variables
        callback_data->input_buffer_pool->p_app_private = NULL;
        callback_data->input_buffer_pool->pic_type   = EB_INVALID_PICTURE;
    }

    return return_error;
}
EbErrorType allocate_output_recon_buffers(
    EbConfig     *config,
    EbAppContext *callback_data)
{

    EbErrorType   return_error = EB_ErrorNone;
    const size_t luma_size =
        config->source_width    *
        config->source_height;
    // both u and v
    const size_t chroma_size = luma_size >> 1;
    const size_t ten_bit = (config->encoder_bit_depth > 8);
    const size_t frame_size = (luma_size + chroma_size) << ten_bit;

// ... Recon Port
    EB_APP_MALLOC(EbBufferHeaderType*, callback_data->recon_buffer, sizeof(EbBufferHeaderType), EB_N_PTR, EB_ErrorInsufficientResources);

    // Initialize Header
    callback_data->recon_buffer->size = sizeof(EbBufferHeaderType);

    EB_APP_MALLOC(uint8_t*, callback_data->recon_buffer->p_buffer, frame_size, EB_N_PTR, EB_ErrorInsufficientResources);

    callback_data->recon_buffer->n_alloc_len = (uint32_t)frame_size;
    callback_data->recon_buffer->p_app_private = NULL;
    return return_error;
}

EbErrorType  preload_frames_into_ram(
    EbConfig *config)
{
    EbErrorType    return_error = EB_ErrorNone;
    int processed_frame_count;

    int filled_len;

    int input_padded_width = config->source_width;
    int input_padded_height = config->source_height;

    int read_size;
    unsigned char *eb_input_ptr;
    FILE *input_file = config->input_file;

    read_size = input_padded_width * input_padded_height * 3 * (config->encoder_bit_depth > 8 ? 2 : 1) / 2;

    EB_APP_MALLOC(unsigned char **, config->sequence_buffer, sizeof(unsigned char*) * config->buffered_input, EB_N_PTR, EB_ErrorInsufficientResources);

    for (processed_frame_count = 0; processed_frame_count < config->buffered_input; ++processed_frame_count) {
        EB_APP_MALLOC(unsigned char*, config->sequence_buffer[processed_frame_count], read_size, EB_N_PTR, EB_ErrorInsufficientResources);

        // Fill the buffer with a complete frame
        filled_len = 0;
        eb_input_ptr = config->sequence_buffer[processed_frame_count];
        filled_len += (uint32_t)fread(eb_input_ptr, 1, read_size, input_file);

        if (read_size != filled_len) {
            fseek(config->input_file, 0, SEEK_SET);
            // Fill the buffer with a complete frame
            filled_len = 0;
            eb_input_ptr = config->sequence_buffer[processed_frame_count];
            filled_len += (uint32_t)fread(eb_input_ptr, 1, read_size, input_file);
        }
    }

    return return_error;
}

/***************************************
* Functions Implementation
***************************************/

/***********************************
 * Initialize Core & Component
 ***********************************/
EbErrorType init_encoder(
    EbConfig     *config,
    EbAppContext *callback_data,
    uint32_t      instance_idx)
{
    EbErrorType        return_error = EB_ErrorNone;

    // Allocate a memory table hosting all allocated pointers
    allocate_memory_table(instance_idx);

    ///************************* LIBRARY INIT [START] *********************///
    // STEP 1: Call the library to construct a Component Handle
    return_error = eb_vp9_svt_init_handle(&callback_data->svt_encoder_handle, callback_data, &callback_data->eb_enc_parameters);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 3: Copy all configuration parameters into the callback structure
    return_error = copy_configuration_parameters(
                    config,
                    callback_data,
                    instance_idx);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 4: Send over all configuration parameters
    // Set the Parameters
    return_error = eb_vp9_svt_enc_set_parameter(
                       callback_data->svt_encoder_handle,
                       &callback_data->eb_enc_parameters);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 5: Init Encoder
    return_error = eb_vp9_init_encoder(callback_data->svt_encoder_handle);
    if (return_error != EB_ErrorNone) { return return_error; }

    ///************************* LIBRARY INIT [END] *********************///

    ///********************** APPLICATION INIT [START] ******************///

    // STEP 6: Allocate input buffers carrying the yuv frames in
    return_error = AllocateInputBuffers(
        config,
        callback_data);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 7: Allocate output Recon Buffer
    return_error = allocate_output_recon_buffers(
        config,
        callback_data);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // Allocate the Sequence Buffer
    if (config->buffered_input != -1) {

        // Preload frames into the ram for a faster yuv access time
        preload_frames_into_ram(
            config);
    }
    else {
        config->sequence_buffer = 0;
    }

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    ///********************** APPLICATION INIT [END] ******************////////

    return return_error;
}

/***********************************
 * Deinit Components
 ***********************************/
EbErrorType de_init_encoder(
    EbAppContext *callback_data_ptr,
    uint32_t        instance_index)
{
    EbErrorType return_error = EB_ErrorNone;
    int32_t              ptr_index        = 0;
    EbMemoryMapEntry*   memory_entry     = (EbMemoryMapEntry*)0;

    if (((EbComponentType*)(callback_data_ptr->svt_encoder_handle)) != NULL) {
            return_error = eb_vp9_deinit_encoder(callback_data_ptr->svt_encoder_handle);
    }

    // Destruct the buffer memory pool
    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // Loop through the ptr table and free all malloc'd pointers per channel
    for (ptr_index = app_memory_map_index_all_channels[instance_index] - 1; ptr_index >= 0; --ptr_index) {
        memory_entry = &app_memory_map_all_channels[instance_index][ptr_index];
        switch (memory_entry->ptr_type) {
        case EB_N_PTR:
            free(memory_entry->ptr);
            break;
        default:
            return_error = EB_ErrorMax;
            break;
        }
    }
    free(app_memory_map_all_channels[instance_index]);

    // Destruct the component
    eb_vp9_deinit_handle(callback_data_ptr->svt_encoder_handle);

    return return_error;
}
