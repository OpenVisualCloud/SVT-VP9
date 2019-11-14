/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSystemResource_h
#define EbSystemResource_h

#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif
/*********************************
 * Defines
 *********************************/
#define EB_ObjectWrapperReleasedValue   ~0u

/*********************************************************************
 * Object Wrapper
 *   Provides state information for each type of object in the
 *   encoder system (i.e. SequenceControlSet, PictureBufferDesc,
 *   ProcessResults, and GracefulDegradation)
 *********************************************************************/
typedef struct EbObjectWrapper
{
    // object_ptr - pointer to the object being managed.
    void                    *object_ptr;

    // live_count - a count of the number of pictures actively being
    //   encoded in the pipeline at any given time.  Modification
    //   of this value by any process must be protected by a mutex.
    uint32_t                 live_count;

    // release_enable - a flag that enables the release of
    //   EbObjectWrapper for reuse in the encoding of subsequent
    //   pictures in the encoder pipeline.
    EB_BOOL                  release_enable;

    // system_resource_ptr - a pointer to the SystemResourceManager
    //   that the object belongs to.
    struct EbSystemResource *system_resource_ptr;

    // next_ptr - a pointer to a different EbObjectWrapper.  Used
    //   only in the implemenation of a single-linked Fifo.
    struct EbObjectWrapper *next_ptr;

} EbObjectWrapper;

/*********************************************************************
 * Fifo
 *   Defines a static (i.e. no dynamic memory allocation) single
 *   linked-list, constant time fifo implmentation. The fifo uses
 *   the EbObjectWrapper member next_ptr to create the linked-list.
 *   The Fifo also contains a counting_semaphore for OS thread-blocking
 *   and dynamic EbObjectWrapper counting.
 *********************************************************************/
typedef struct EbFifo
{
    // counting_semaphore - used for OS thread-blocking & dynamically
    //   counting the number of EbObjectWrappers currently in the
    //   EbFifo.
    EbHandle             counting_semaphore;

    // lockout_mutex - used to prevent more than one thread from
    //   modifying EbFifo simultaneously.
    EbHandle             lockout_mutex;

    // first_ptr - pointer the the head of the Fifo
    EbObjectWrapper     *first_ptr;

    // last_ptr - pointer to the tail of the Fifo
    EbObjectWrapper     *last_ptr;

    // queue_ptr - pointer to MuxingQueue that the EbFifo is
    //   associated with.
    struct EbMuxingQueue *queue_ptr;

} EbFifo;

/*********************************************************************
 * CircularBuffer
 *********************************************************************/
typedef struct EbCircularBuffer
{
    EbPtr    *array_ptr;
    uint32_t  head_index;
    uint32_t  tail_index;
    uint32_t  buffer_total_count;
    uint32_t  current_count;

} EbCircularBuffer;

/*********************************************************************
 * MuxingQueue
 *********************************************************************/
typedef struct EbMuxingQueue
{
    EbHandle         lockout_mutex;
    EbCircularBuffer *object_queue;
    EbCircularBuffer *process_queue;
    uint32_t          process_total_count;
    EbFifo          **process_fifo_ptr_array;

} EbMuxingQueue;

/*********************************************************************
 * SystemResource
 *   Defines a complete solution for managing objects in the encoder
 *   system (i.e. SequenceControlSet, PictureBufferDesc, ProcessResults, and
 *   GracefulDegradation).  The object_total_count and wrapper_ptr_pool are
 *   only used to construct and destruct the SystemResource.  The
 *   fullFifo provides downstream pipeline data flow control.  The
 *   emptyFifo provides upstream pipeline backpressure flow control.
 *********************************************************************/
typedef struct EbSystemResource
{
    // object_total_count - A count of the number of objects contained in the
    //   System Resoruce.
    uint32_t          object_total_count;

    // wrapper_ptr_pool - An array of pointers to the EbObjectWrappers used
    //   to construct and destruct the SystemResource.
    EbObjectWrapper **wrapper_ptr_pool;

    // The empty FIFO contains a queue of empty buffers
    EbMuxingQueue    *empty_queue;

    // The full FIFO contains a queue of completed buffers
    EbMuxingQueue    *full_queue;

} EbSystemResource;

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
extern EbErrorType eb_vp9_object_release_enable(
    EbObjectWrapper *wrapper_ptr);

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
extern EbErrorType eb_vp9_object_release_disable(
    EbObjectWrapper *wrapper_ptr);

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
 *
 *   increment_number
 *      The number to increment the live count by.
 *********************************************************************/
extern EbErrorType eb_vp9_object_inc_live_count(
    EbObjectWrapper *wrapper_ptr,
    uint32_t         increment_number);

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
extern EbErrorType eb_vp9_system_resource_ctor(
    EbSystemResource **resource_dbl_ptr,
    uint32_t           object_total_count,
    uint32_t           producer_process_total_count,
    uint32_t           consumer_process_total_count,
    EbFifo          ***producer_fifo_ptr_array_ptr,
    EbFifo          ***consumer_fifo_ptr_array_ptr,
    EB_BOOL            full_fifo_enabled,
    EB_CTOR            object_ctor,
    EbPtr              object_init_data_ptr);

/*********************************************************************
 * eb_system_resource_dtor
 *   Destructor for EbSystemResource.  Fully destructs all members
 *   of EbSystemResource including the object with the passed
 *   object_dtor function.
 *
 *   resource_ptr
 *     pointer to the SystemResource to be destructed.
 *
 *   object_dtor
 *     Function pointer to the destructor of the object managed by
 *     SystemResource referenced by resource_ptr. No object level
 *     destruction is performed if object_dtor is NULL.
 *********************************************************************/
extern void eb_system_resource_dtor(
    EbSystemResource *resource_ptr,
    EbDtor            object_dtor);

/*********************************************************************
 * EbSystemResourceGetEmptyObject
 *   Dequeues an empty EbObjectWrapper from the SystemResource.  The
 *   new EbObjectWrapper will be populated with the contents of the
 *   wrapperCopyPtr if wrapperCopyPtr is not NULL. This function blocks
 *   on the SystemResource emptyFifo counting_semaphore. This function
 *   is write protected by the SystemResource emptyFifo lockout_mutex.
 *
 *   resource_ptr
 *      pointer to the SystemResource that provides the empty
 *      EbObjectWrapper.
 *
 *   wrapper_dbl_ptr
 *      Double pointer used to pass the pointer to the empty
 *      EbObjectWrapper pointer.
 *********************************************************************/
extern EbErrorType eb_vp9_get_empty_object(
    EbFifo           *empty_fifo_ptr,
    EbObjectWrapper **wrapper_dbl_ptr);

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
extern EbErrorType eb_vp9_post_full_object(
    EbObjectWrapper *object_ptr);

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
extern EbErrorType eb_vp9_get_full_object(
    EbFifo           *full_fifo_ptr,
    EbObjectWrapper **wrapper_dbl_ptr);

extern EbErrorType eb_vp9_get_full_object_non_blocking(
    EbFifo           *full_fifo_ptr,
    EbObjectWrapper **wrapper_dbl_ptr);

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
extern EbErrorType eb_vp9_release_object(
    EbObjectWrapper *object_ptr);
#ifdef __cplusplus
}
#endif
#endif //EbSystemResource_h
