/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureManager_h
#define EbPictureManager_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/***************************************
 * Context
 ***************************************/
typedef struct PictureManagerContext
{
    EbFifo  *picture_input_fifo_ptr;
    EbFifo  *picture_manager_output_fifo_ptr;
    EbFifo **picture_control_set_fifo_ptr_array;

} PictureManagerContext;

/***************************************
 * Extern Function Declaration
 ***************************************/
extern EbErrorType eb_vp9_picture_manager_context_ctor(
    PictureManagerContext **context_dbl_ptr,
    EbFifo                 *picture_input_fifo_ptr,
    EbFifo                 *picture_manager_output_fifo_ptr,
    EbFifo                **picture_control_set_fifo_ptr_array);

extern void* eb_vp9_PictureManagerKernel(void *input_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureManager_h
