/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbSystemResourceManager.h"

/**************************************
 * eb_fifo_ctor
 **************************************/
static EbErrorType eb_fifo_ctor(
    EbFifo          *fifo_ptr,
    uint32_t         initial_count,
    uint32_t         max_count,
    EbObjectWrapper *first_wrapper_ptr,
    EbObjectWrapper *last_wrapper_ptr,
    EbMuxingQueue   *queue_ptr)
{
    // Create Counting Semaphore
    EB_CREATESEMAPHORE(EbHandle, fifo_ptr->counting_semaphore, sizeof(EbHandle), EB_SEMAPHORE, initial_count, max_count);

    // Create Buffer Pool Mutex
    EB_CREATEMUTEX(EbHandle, fifo_ptr->lockout_mutex, sizeof(EbHandle), EB_MUTEX);

    // Initialize Fifo First & Last ptrs
    fifo_ptr->first_ptr           = first_wrapper_ptr;
    fifo_ptr->last_ptr            = last_wrapper_ptr;

    // Copy the Muxing Queue ptr this Fifo belongs to
    fifo_ptr->queue_ptr           = queue_ptr;

    return EB_ErrorNone;
}

/**************************************
 * eb_fifo_push_back
 **************************************/
static EbErrorType eb_fifo_push_back(
    EbFifo          *fifo_ptr,
    EbObjectWrapper *wrapper_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // If FIFO is empty
    if(fifo_ptr->first_ptr == (EbObjectWrapper*) EB_NULL) {
        fifo_ptr->first_ptr = wrapper_ptr;
        fifo_ptr->last_ptr  = wrapper_ptr;
    } else {
        fifo_ptr->last_ptr->next_ptr = wrapper_ptr;
        fifo_ptr->last_ptr = wrapper_ptr;
    }

    fifo_ptr->last_ptr->next_ptr = (EbObjectWrapper*) EB_NULL;

    return return_error;
}

/**************************************
 * eb_fifo_pop_front
 **************************************/
static EbErrorType eb_fifo_pop_front(
    EbFifo           *fifo_ptr,
    EbObjectWrapper **wrapper_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Set wrapper_ptr to head of BufferPool
    *wrapper_ptr = fifo_ptr->first_ptr;

    // Update tail of BufferPool if the BufferPool is now empty
    fifo_ptr->last_ptr = (fifo_ptr->first_ptr == fifo_ptr->last_ptr) ? (EbObjectWrapper*) EB_NULL: fifo_ptr->last_ptr;

    // Update head of BufferPool
    fifo_ptr->first_ptr = fifo_ptr->first_ptr->next_ptr;

    return return_error;
}

/**************************************
 * eb_circular_buffer_ctor
 **************************************/
static EbErrorType eb_circular_buffer_ctor(
    EbCircularBuffer **buffer_dbl_ptr,
    uint32_t           buffer_total_count)
{
    uint32_t buffer_index;
    EbCircularBuffer *buffer_ptr;

    EB_MALLOC(EbCircularBuffer*, buffer_ptr, sizeof(EbCircularBuffer), EB_N_PTR);

    *buffer_dbl_ptr = buffer_ptr;

    buffer_ptr->buffer_total_count = buffer_total_count;

    EB_MALLOC(EbPtr *, buffer_ptr->array_ptr, sizeof(EbPtr) * buffer_ptr->buffer_total_count, EB_N_PTR);

    for(buffer_index=0; buffer_index < buffer_ptr->buffer_total_count; ++buffer_index) {
        buffer_ptr->array_ptr[buffer_index] = EB_NULL;
    }

    buffer_ptr->head_index = 0;
    buffer_ptr->tail_index = 0;

    buffer_ptr->current_count = 0;

    return EB_ErrorNone;
}

/**************************************
 * eb_circular_buffer_empty_check
 **************************************/
static EB_BOOL eb_circular_buffer_empty_check(
    EbCircularBuffer *buffer_ptr)
{
    return ((buffer_ptr->head_index == buffer_ptr->tail_index) && (buffer_ptr->array_ptr[buffer_ptr->head_index] == EB_NULL)) ? EB_TRUE : EB_FALSE;
}

/**************************************
 * eb_circular_buffer_pop_front
 **************************************/
static EbErrorType eb_circular_buffer_pop_front(
    EbCircularBuffer *buffer_ptr,
    EbPtr            *object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Copy the head of the buffer into the object_ptr
    *object_ptr = buffer_ptr->array_ptr[buffer_ptr->head_index];
    buffer_ptr->array_ptr[buffer_ptr->head_index] = EB_NULL;

    // Increment the head & check for rollover
    buffer_ptr->head_index = (buffer_ptr->head_index == buffer_ptr->buffer_total_count - 1) ? 0 : buffer_ptr->head_index + 1;

    // Decrement the Current Count
    --buffer_ptr->current_count;

    return return_error;
}

/**************************************
 * eb_circular_buffer_push_back
 **************************************/
static EbErrorType eb_circular_buffer_push_back(
    EbCircularBuffer *buffer_ptr,
    EbPtr             object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Copy the pointer into the array
    buffer_ptr->array_ptr[buffer_ptr->tail_index] = object_ptr;

    // Increment the tail & check for rollover
    buffer_ptr->tail_index = (buffer_ptr->tail_index == buffer_ptr->buffer_total_count - 1) ? 0 : buffer_ptr->tail_index + 1;

    // Increment the Current Count
    ++buffer_ptr->current_count;

    return return_error;
}

/**************************************
 * eb_circular_buffer_push_front
 **************************************/
static EbErrorType eb_circular_buffer_push_front(
    EbCircularBuffer *buffer_ptr,
    EbPtr             object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Decrement the head_index
    buffer_ptr->head_index = (buffer_ptr->head_index == 0) ? buffer_ptr->buffer_total_count - 1 : buffer_ptr->head_index - 1;

    // Copy the pointer into the array
    buffer_ptr->array_ptr[buffer_ptr->head_index] = object_ptr;

    // Increment the Current Count
    ++buffer_ptr->current_count;

    return return_error;
}

/**************************************
 * eb_muxing_queue_ctor
 **************************************/
static EbErrorType eb_muxing_queue_ctor(
    EbMuxingQueue **queue_dbl_ptr,
    uint32_t        object_total_count,
    uint32_t        process_total_count,
    EbFifo       ***process_fifo_ptr_array_ptr)
{
    EbMuxingQueue *queue_ptr;
    uint32_t process_index;
    EbErrorType     return_error = EB_ErrorNone;

    EB_MALLOC(EbMuxingQueue *, queue_ptr, sizeof(EbMuxingQueue), EB_N_PTR);
    *queue_dbl_ptr = queue_ptr;

    queue_ptr->process_total_count = process_total_count;

    // Lockout Mutex
    EB_CREATEMUTEX(EbHandle, queue_ptr->lockout_mutex, sizeof(EbHandle), EB_MUTEX);

    // Construct Object Circular Buffer
    return_error = eb_circular_buffer_ctor(
        &queue_ptr->object_queue,
        object_total_count);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Construct Process Circular Buffer
    return_error = eb_circular_buffer_ctor(
        &queue_ptr->process_queue,
        queue_ptr->process_total_count);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Construct the Process Fifos
    EB_MALLOC(EbFifo**, queue_ptr->process_fifo_ptr_array, sizeof(EbFifo*) * queue_ptr->process_total_count, EB_N_PTR);

    for(process_index=0; process_index < queue_ptr->process_total_count; ++process_index) {
        EB_MALLOC(EbFifo*, queue_ptr->process_fifo_ptr_array[process_index], sizeof(EbFifo) * queue_ptr->process_total_count, EB_N_PTR);
        return_error = eb_fifo_ctor(
            queue_ptr->process_fifo_ptr_array[process_index],
            0,
            object_total_count,
            (EbObjectWrapper *)EB_NULL,
            (EbObjectWrapper *)EB_NULL,
            queue_ptr);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    *process_fifo_ptr_array_ptr = queue_ptr->process_fifo_ptr_array;

    return return_error;
}

/**************************************
 * eb_muxing_queue_assignation
 **************************************/
static EbErrorType eb_muxing_queue_assignation(
    EbMuxingQueue *queue_ptr)
{
    EbErrorType return_error = EB_ErrorNone;
    EbFifo *process_fifo_ptr;
    EbObjectWrapper *wrapper_ptr;

    // while loop
    while((eb_circular_buffer_empty_check(queue_ptr->object_queue)  == EB_FALSE) &&
            (eb_circular_buffer_empty_check(queue_ptr->process_queue) == EB_FALSE)) {
        // Get the next process
        eb_circular_buffer_pop_front(
            queue_ptr->process_queue,
            (void **) &process_fifo_ptr);

        // Get the next object
        eb_circular_buffer_pop_front(
            queue_ptr->object_queue,
            (void **) &wrapper_ptr);

        // Block on the Process Fifo's Mutex
        eb_vp9_block_on_mutex(process_fifo_ptr->lockout_mutex);

        // Put the object on the fifo
        eb_fifo_push_back(
            process_fifo_ptr,
            wrapper_ptr);

        // Release the Process Fifo's Mutex
        eb_vp9_release_mutex(process_fifo_ptr->lockout_mutex);

        // Post the semaphore
        eb_vp9_post_semaphore(process_fifo_ptr->counting_semaphore);
    }

    return return_error;
}

/**************************************
 * eb_muxing_queue_object_push_back
 **************************************/
static EbErrorType eb_muxing_queue_object_push_back(
    EbMuxingQueue   *queue_ptr,
    EbObjectWrapper *object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_circular_buffer_push_back(
        queue_ptr->object_queue,
        object_ptr);

    eb_muxing_queue_assignation(queue_ptr);

    return return_error;
}

/**************************************
* eb_muxing_queue_object_push_front
**************************************/
static EbErrorType eb_muxing_queue_object_push_front(
    EbMuxingQueue   *queue_ptr,
    EbObjectWrapper *object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_circular_buffer_push_front(
        queue_ptr->object_queue,
        object_ptr);

    eb_muxing_queue_assignation(queue_ptr);

    return return_error;
}

/*********************************************************************
 * eb_vp9_object_release_enable
 *   Enables the release_enable member of EbObjectWrapper.  Used by
 *   certain objects (e.g. SequenceControlSet) to control whether
 *   EbObjectWrappers are allowed to be released or not.
 *
 *   resource_ptr
 *      pointer to the SystemResource that manages the EbObjectWrapper.
 *      The emptyFifo's lockout_mutex is used to write protect the
 *      modification of the EbObjectWrapper.
 *
 *   wrapper_ptr
 *      pointer to the EbObjectWrapper to be modified.
 *********************************************************************/
EbErrorType eb_vp9_object_release_enable(
    EbObjectWrapper *wrapper_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_vp9_block_on_mutex(wrapper_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    wrapper_ptr->release_enable = EB_TRUE;

    eb_vp9_release_mutex(wrapper_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * eb_vp9_object_release_disable
 *   Disables the release_enable member of EbObjectWrapper.  Used by
 *   certain objects (e.g. SequenceControlSet) to control whether
 *   EbObjectWrappers are allowed to be released or not.
 *
 *   resource_ptr
 *      pointer to the SystemResource that manages the EbObjectWrapper.
 *      The emptyFifo's lockout_mutex is used to write protect the
 *      modification of the EbObjectWrapper.
 *
 *   wrapper_ptr
 *      pointer to the EbObjectWrapper to be modified.
 *********************************************************************/
EbErrorType eb_vp9_object_release_disable(
    EbObjectWrapper *wrapper_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_vp9_block_on_mutex(wrapper_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    wrapper_ptr->release_enable = EB_FALSE;

    eb_vp9_release_mutex(wrapper_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * eb_vp9_object_inc_live_count
 *   Increments the live_count member of EbObjectWrapper.  Used by
 *   certain objects (e.g. SequenceControlSet) to count the number of active
 *   pointers of a EbObjectWrapper in pipeline at any point in time.
 *
 *   resource_ptr
 *      pointer to the SystemResource that manages the EbObjectWrapper.
 *      The emptyFifo's lockout_mutex is used to write protect the
 *      modification of the EbObjectWrapper.
 *
 *   wrapper_ptr
 *      pointer to the EbObjectWrapper to be modified.
 *********************************************************************/
EbErrorType eb_vp9_object_inc_live_count(
    EbObjectWrapper *wrapper_ptr,
    uint32_t         increment_number)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_vp9_block_on_mutex(wrapper_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    wrapper_ptr->live_count += increment_number;

    eb_vp9_release_mutex(wrapper_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * eb_vp9_system_resource_ctor
 *   Constructor for EbSystemResource.  Fully constructs all members
 *   of EbSystemResource including the object with the passed
 *   object_ctor function.
 *
 *   resource_ptr
 *     pointer that will contain the SystemResource to be constructed.
 *
 *   object_total_count
 *     Number of objects to be managed by the SystemResource.
 *
 *   full_fifo_enabled
 *     Bool that describes if the SystemResource is to have an output
 *     fifo.  An outputFifo is not used by certain objects (e.g.
 *     SequenceControlSet).
 *
 *   object_ctor
 *     Function pointer to the constructor of the object managed by
 *     SystemResource referenced by resource_ptr. No object level
 *     construction is performed if object_ctor is NULL.
 *
 *   object_init_data_ptr

 *     pointer to data block to be used during the construction of
 *     the object. object_init_data_ptr is passed to object_ctor when
 *     object_ctor is called.
 *********************************************************************/
EbErrorType eb_vp9_system_resource_ctor(
    EbSystemResource **resource_dbl_ptr,
    uint32_t           object_total_count,
    uint32_t           producer_process_total_count,
    uint32_t           consumer_process_total_count,
    EbFifo          ***producer_fifo_ptr_array_ptr,
    EbFifo          ***consumer_fifo_ptr_array_ptr,
    EB_BOOL            full_fifo_enabled,
    EB_CTOR            object_ctor,
    EbPtr              object_init_data_ptr)
{
    uint32_t wrapperIndex;
    EbErrorType return_error = EB_ErrorNone;
    // Allocate the System Resource
    EbSystemResource *resource_ptr;

    EB_MALLOC(EbSystemResource*, resource_ptr, sizeof(EbSystemResource), EB_N_PTR);
    *resource_dbl_ptr = resource_ptr;

    resource_ptr->object_total_count = object_total_count;

    // Allocate array for wrapper pointers
    EB_MALLOC(EbObjectWrapper**, resource_ptr->wrapper_ptr_pool, sizeof(EbObjectWrapper*) * resource_ptr->object_total_count, EB_N_PTR);

    // Initialize each wrapper
    for (wrapperIndex=0; wrapperIndex < resource_ptr->object_total_count; ++wrapperIndex) {
        EB_MALLOC(EbObjectWrapper*, resource_ptr->wrapper_ptr_pool[wrapperIndex], sizeof(EbObjectWrapper), EB_N_PTR);
        resource_ptr->wrapper_ptr_pool[wrapperIndex]->live_count            = 0;
        resource_ptr->wrapper_ptr_pool[wrapperIndex]->release_enable        = EB_TRUE;
        resource_ptr->wrapper_ptr_pool[wrapperIndex]->system_resource_ptr    = resource_ptr;

        // Call the Constructor for each element
        if(object_ctor) {
            return_error = object_ctor(
                &resource_ptr->wrapper_ptr_pool[wrapperIndex]->object_ptr,
                object_init_data_ptr);
            if (return_error == EB_ErrorInsufficientResources){
                return EB_ErrorInsufficientResources;
            }
        }
    }

    // Initialize the Empty Queue
    return_error = eb_muxing_queue_ctor(
        &resource_ptr->empty_queue,
        resource_ptr->object_total_count,
        producer_process_total_count,
        producer_fifo_ptr_array_ptr);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Fill the Empty Fifo with every ObjectWrapper
    for (wrapperIndex=0; wrapperIndex < resource_ptr->object_total_count; ++wrapperIndex) {
        eb_muxing_queue_object_push_back(
            resource_ptr->empty_queue,
            resource_ptr->wrapper_ptr_pool[wrapperIndex]);
    }

    // Initialize the Full Queue
    if (full_fifo_enabled == EB_TRUE) {
        return_error = eb_muxing_queue_ctor(
            &resource_ptr->full_queue,
            resource_ptr->object_total_count,
            consumer_process_total_count,
            consumer_fifo_ptr_array_ptr);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    } else {
        resource_ptr->full_queue  = (EbMuxingQueue *)EB_NULL;
        consumer_fifo_ptr_array_ptr = (EbFifo ***)EB_NULL;
    }

    return return_error;
}

/*********************************************************************
 * EbSystemResourceReleaseProcess
 *********************************************************************/
static EbErrorType eb_release_process(
    EbFifo *process_fifo_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_vp9_block_on_mutex(process_fifo_ptr->queue_ptr->lockout_mutex);

    eb_circular_buffer_push_front(
        process_fifo_ptr->queue_ptr->process_queue,
        process_fifo_ptr);

    eb_muxing_queue_assignation(process_fifo_ptr->queue_ptr);

    eb_vp9_release_mutex(process_fifo_ptr->queue_ptr->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * EbSystemResourcePostObject
 *   Queues a full EbObjectWrapper to the SystemResource. This
 *   function posts the SystemResource fullFifo counting_semaphore.
 *   This function is write protected by the SystemResource fullFifo
 *   lockout_mutex.
 *
 *   resource_ptr
 *      pointer to the SystemResource that the EbObjectWrapper is
 *      posted to.
 *
 *   wrapper_ptr
 *      pointer to EbObjectWrapper to be posted.
 *********************************************************************/
EbErrorType eb_vp9_post_full_object(
    EbObjectWrapper *object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_vp9_block_on_mutex(object_ptr->system_resource_ptr->full_queue->lockout_mutex);

    eb_muxing_queue_object_push_back(
        object_ptr->system_resource_ptr->full_queue,
        object_ptr);

    eb_vp9_release_mutex(object_ptr->system_resource_ptr->full_queue->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * EbSystemResourceReleaseObject
 *   Queues an empty EbObjectWrapper to the SystemResource. This
 *   function posts the SystemResource emptyFifo counting_semaphore.
 *   This function is write protected by the SystemResource emptyFifo
 *   lockout_mutex.
 *
 *   object_ptr
 *      pointer to EbObjectWrapper to be released.
 *********************************************************************/
EbErrorType eb_vp9_release_object(
    EbObjectWrapper *object_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    eb_vp9_block_on_mutex(object_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    // Decrement live_count
    object_ptr->live_count = (object_ptr->live_count == 0) ? object_ptr->live_count : object_ptr->live_count - 1;

    if((object_ptr->release_enable == EB_TRUE) && (object_ptr->live_count == 0)) {

        // Set live_count to EB_ObjectWrapperReleasedValue
        object_ptr->live_count = EB_ObjectWrapperReleasedValue;

        eb_muxing_queue_object_push_front(
            object_ptr->system_resource_ptr->empty_queue,
            object_ptr);

    }

    eb_vp9_release_mutex(object_ptr->system_resource_ptr->empty_queue->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * EbSystemResourceGetEmptyObject
 *   Dequeues an empty EbObjectWrapper from the SystemResource.  This
 *   function blocks on the SystemResource emptyFifo counting_semaphore.
 *   This function is write protected by the SystemResource emptyFifo
 *   lockout_mutex.
 *
 *   resource_ptr
 *      pointer to the SystemResource that provides the empty
 *      EbObjectWrapper.
 *
 *   wrapper_dbl_ptr
 *      Double pointer used to pass the pointer to the empty
 *      EbObjectWrapper pointer.
 *********************************************************************/
EbErrorType eb_vp9_get_empty_object(
    EbFifo           *empty_fifo_ptr,
    EbObjectWrapper **wrapper_dbl_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Queue the Fifo requesting the empty fifo
    eb_release_process(empty_fifo_ptr);

    // Block on the counting Semaphore until an empty buffer is available
    eb_vp9_block_on_semaphore(empty_fifo_ptr->counting_semaphore);

    // Acquire lockout Mutex
    eb_vp9_block_on_mutex(empty_fifo_ptr->lockout_mutex);

    // Get the empty object
    eb_fifo_pop_front(
        empty_fifo_ptr,
        wrapper_dbl_ptr);

    // Reset the wrapper's live_count
    (*wrapper_dbl_ptr)->live_count = 0;

    // Object release enable
    (*wrapper_dbl_ptr)->release_enable = EB_TRUE;

    // Release Mutex
    eb_vp9_release_mutex(empty_fifo_ptr->lockout_mutex);

    return return_error;
}

/*********************************************************************
 * EbSystemResourceGetFullObject
 *   Dequeues an full EbObjectWrapper from the SystemResource. This
 *   function blocks on the SystemResource fullFifo counting_semaphore.
 *   This function is write protected by the SystemResource fullFifo
 *   lockout_mutex.
 *
 *   resource_ptr
 *      pointer to the SystemResource that provides the full
 *      EbObjectWrapper.
 *
 *   wrapper_dbl_ptr
 *      Double pointer used to pass the pointer to the full
 *      EbObjectWrapper pointer.
 *********************************************************************/
EbErrorType eb_vp9_get_full_object(
    EbFifo           *full_fifo_ptr,
    EbObjectWrapper **wrapper_dbl_ptr)
{
    EbErrorType return_error = EB_ErrorNone;

    // Queue the Fifo requesting the full fifo
    eb_release_process(full_fifo_ptr);

    // Block on the counting Semaphore until an empty buffer is available
    eb_vp9_block_on_semaphore(full_fifo_ptr->counting_semaphore);

    // Acquire lockout Mutex
    eb_vp9_block_on_mutex(full_fifo_ptr->lockout_mutex);

    eb_fifo_pop_front(
        full_fifo_ptr,
        wrapper_dbl_ptr);

    // Release Mutex
    eb_vp9_release_mutex(full_fifo_ptr->lockout_mutex);

    return return_error;
}
/**************************************
* EbFifoPopFront
**************************************/
static EB_BOOL eb_fifo_peak_front(
    EbFifo            *fifo_ptr){

    // Set wrapper_ptr to head of BufferPool
    if (fifo_ptr->first_ptr == (EbObjectWrapper*)EB_NULL)
        return EB_TRUE;
    else
        return EB_FALSE;
}

EbErrorType eb_vp9_get_full_object_non_blocking(
    EbFifo          *full_fifo_ptr,
    EbObjectWrapper **wrapper_dbl_ptr){

    EbErrorType return_error = EB_ErrorNone;
    EB_BOOL     fifo_empty;
    // Queue the Fifo requesting the full fifo
    eb_release_process(full_fifo_ptr);

    // Acquire lockout Mutex
    eb_vp9_block_on_mutex(full_fifo_ptr->lockout_mutex);

    fifo_empty = eb_fifo_peak_front(
        full_fifo_ptr);

    // Release Mutex
    eb_vp9_release_mutex(full_fifo_ptr->lockout_mutex);

    if (fifo_empty == EB_FALSE)
        eb_vp9_get_full_object(
            full_fifo_ptr,
            wrapper_dbl_ptr);
    else
        *wrapper_dbl_ptr = (EbObjectWrapper*)EB_NULL;

    return return_error;
}
