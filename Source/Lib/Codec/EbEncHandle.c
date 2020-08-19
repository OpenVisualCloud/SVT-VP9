/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// SUMMARY
//   Contains the API component functions

/**************************************
 * Includes
 **************************************/

// Defining _GNU_SOURCE is needed for CPU_ZERO macro and needs to be set before any other includes
#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>

#include "EbDefinitions.h"
#include "EbSvtVp9Enc.h"
#include "EbThreads.h"
#include "EbUtility.h"
#include "EbEncHandle.h"

#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"
#include "EbReferenceObject.h"

#include "EbResourceCoordinationProcess.h"
#include "EbPictureAnalysisProcess.h"
#include "EbPictureDecisionProcess.h"
#include "EbMotionEstimationProcess.h"
#include "EbInitialRateControlProcess.h"
#include "EbSourceBasedOperationsProcess.h"
#include "EbPictureManagerProcess.h"
#include "EbRateControlProcess.h"
#include "EbModeDecisionConfigurationProcess.h"
#include "EbEncDecProcess.h"
#include "EbEntropyCodingProcess.h"
#include "EbPacketizationProcess.h"

#include "EbResourceCoordinationResults.h"
#include "EbPictureAnalysisResults.h"
#include "EbPictureDecisionResults.h"
#include "EbMotionEstimationResults.h"
#include "EbInitialRateControlResults.h"
#include "EbPictureDemuxResults.h"
#include "EbRateControlTasks.h"
#include "EbRateControlResults.h"
#include "EbEncDecTasks.h"
#include "EbEncDecResults.h"
#include "EbEntropyCodingResults.h"

#include "EbPredictionStructure.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef __linux__
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define RTCD_C
#include "vpx_dsp_rtcd.h"
#include "vp9_rtcd.h"
 /**************************************
  * Defines
  **************************************/

#define EB_EncodeInstancesTotalCount                    1
#define EB_CandidateInitCount                           1
#define EB_ComputeSegmentInitCount                      1

  // Config Set Initial Count
#define EB_SequenceControlSetPoolInitCount              3

// Process Instantiation Initial Counts
#define EB_ResourceCoordinationProcessInitCount         1
#define EB_PictureDecisionProcessInitCount              1
#define EB_InitialRateControlProcessInitCount           1
#define EB_PictureManagerProcessInitCount               1
#define EB_RateControlProcessInitCount                  1
#define EB_PacketizationProcessInitCount                1

// Buffer Transfer Parameters
#define EB_INPUTVIDEOBUFFERSIZE                         0x10000//   832*480*3//      // Input Slice Size , must me a multiple of 2 in case of 10 bit video.
#define EB_OUTPUTSTREAMBUFFERSIZE                       0x2DC6C0   //0x7D00        // match MTU Size
#define EB_OUTPUTRECONBUFFERSIZE                        (MAX_PICTURE_WIDTH_SIZE*MAX_PICTURE_HEIGHT_SIZE*2)   // Recon Slice Size
#define EB_OUTPUTSTREAMQUANT                            27
#define EB_OUTPUTSTATISTICSBUFFERSIZE                   0x30            // 6X8 (8 Bytes for Y, U, V, number of bits, picture number, QP)
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)                ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0  )

static uint64_t max_luma_picture_size[TOTAL_LEVEL_COUNT] = { 36864U, 122880U, 245760U, 552960U, 983040U, 2228224U, 2228224U, 8912896U, 8912896U, 8912896U, 35651584U, 35651584U, 35651584U };
static uint64_t max_luma_sample_rate[TOTAL_LEVEL_COUNT] = { 552960U, 3686400U, 7372800U, 16588800U, 33177600U, 66846720U, 133693440U, 267386880U, 534773760U, 1069547520U, 1069547520U, 2139095040U, 4278190080U };

uint32_t eb_vp9_ASM_TYPES;
/**************************************
 * External Functions
 **************************************/
static EbErrorType  init_svt_vp9_encoder_handle(EbComponentType * h_component);
#include <immintrin.h>

/**************************************
 * Globals
 **************************************/

EbMemoryMapEntry  *memory_map;
uint32_t          *memory_map_index;
uint64_t          *total_lib_memory;

uint32_t           lib_malloc_count = 0;
uint32_t           lib_thread_count = 0;
uint32_t           lib_semaphore_count = 0;
uint32_t           lib_mutex_count = 0;

uint8_t                          eb_vp9_num_groups = 0;
#ifdef _WIN32
GROUP_AFFINITY                   eb_vp9_group_affinity;
EbBool                           eb_vp9_alternate_groups = 0;
#elif defined(__linux__)
cpu_set_t                        eb_vp9_group_affinity;
typedef struct logicalProcessorGroup {
    uint32_t num;
    uint32_t group[1024];
}processorGroup;
#define MAX_PROCESSOR_GROUP 16
processorGroup                   eb_vp9_lp_group[MAX_PROCESSOR_GROUP];
#endif

/**************************************
* Instruction Set Support
**************************************/
#include <stdint.h>
#ifdef _WIN32
# include <intrin.h>
#endif
void run_cpuid(uint32_t eax, uint32_t ecx, int* abcd)
{
#ifdef _WIN32
    __cpuidex(abcd, eax, ecx);
#else
    uint32_t ebx, edx;
# if defined( __i386__ ) && defined ( __PIC__ )
    /* in case of PIC under 32-bit EBX cannot be clobbered */
    __asm__("movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
    __asm__("cpuid" : "=b" (ebx),
# endif
        "+a" (eax), "+c" (ecx), "=d" (edx));
    abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
#endif
}

int check_xcr0_ymm()
{
    uint32_t xcr0;
#ifdef _WIN32
    xcr0 = (uint32_t)_xgetbv(0);  /* min VS2010 SP1 compiler is required */
#else
    __asm__("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
#endif
    return ((xcr0 & 6) == 6); /* checking if xmm and ymm state are enabled in XCR0 */
}
int32_t check4th_gen_intel_core_features()
{
    int abcd[4];
    int fma_movbe_osxsave_mask = ((1 << 12) | (1 << 22) | (1 << 27));
    int avx2_bmi12_mask = (1 << 5) | (1 << 3) | (1 << 8);

    /* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1   &&
       CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 &&
       CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
    run_cpuid(1, 0, abcd);
    if ((abcd[2] & fma_movbe_osxsave_mask) != fma_movbe_osxsave_mask)
        return 0;

    if (!check_xcr0_ymm())
        return 0;

    /*  CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI2[bit 8]==1  */
    run_cpuid(7, 0, abcd);
    if ((abcd[1] & avx2_bmi12_mask) != avx2_bmi12_mask)
        return 0;
    /* CPUID.(EAX=80000001H):ECX.LZCNT[bit 5]==1 */
    run_cpuid(0x80000001, 0, abcd);
    if ((abcd[2] & (1 << 5)) == 0)
        return 0;
    return 1;
}
static int32_t can_use_intel_core4th_gen_features()
{
    static int the_4th_gen_features_available = -1;
    /* test is performed once */
    if (the_4th_gen_features_available < 0)
        the_4th_gen_features_available = check4th_gen_intel_core_features();
    return the_4th_gen_features_available;
}

int check_xcr0_zmm()
{
    uint32_t xcr0;
    uint32_t zmm_ymm_xmm = (7 << 5) | (1 << 2) | (1 << 1);
#ifdef _WIN32
    xcr0 = (uint32_t)_xgetbv(0);  /* min VS2010 SP1 compiler is required */
#else
    __asm__("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
#endif
    return ((xcr0 & zmm_ymm_xmm) == zmm_ymm_xmm); /* check if xmm, ymm and zmm state are enabled in XCR0 */
}

static int32_t can_use_intel_avx512()
{
    int abcd[4];

    /*  CPUID.(EAX=07H, ECX=0):EBX[bit 16]==1 AVX512F
        CPUID.(EAX=07H, ECX=0):EBX[bit 17] AVX512DQ
        CPUID.(EAX=07H, ECX=0):EBX[bit 28] AVX512CD
        CPUID.(EAX=07H, ECX=0):EBX[bit 30] AVX512BW
        CPUID.(EAX=07H, ECX=0):EBX[bit 31] AVX512VL */

    int avx512_ebx_mask =
        (1 << 16)  // AVX-512F
        | (1 << 17)  // AVX-512DQ
        | (1 << 28)  // AVX-512CD
        | (1 << 30)  // AVX-512BW
        | (1 << 31); // AVX-512VL

    // ensure OS supports ZMM registers (and YMM, and XMM)
    if (!check_xcr0_zmm())
        return 0;

    if (!check4th_gen_intel_core_features())
        return 0;

    run_cpuid(7, 0, abcd);
    if ((abcd[1] & avx512_ebx_mask) != avx512_ebx_mask)
        return 0;

    return 1;
}

// Returns ASM Type based on system configuration. AVX512 - 111, AVX2 - 011, NONAVX2 - 001, C - 000
// Using bit-fields, the fastest function will always be selected based on the available functions in the function arrays
uint32_t get_cpu_asm_type()
{
    uint32_t asm_type = 0;

    if (can_use_intel_avx512() == 1)
        asm_type = 7; // bit-field
    else
        if (can_use_intel_core4th_gen_features() == 1)
            asm_type = 3; // bit-field
        else
            asm_type = 1; // bit-field

    return asm_type;
}

//Get Number of logical processors
uint32_t get_num_cores() {
#ifdef _WIN32
   SYSTEM_INFO sysinfo;
   GetSystemInfo(&sysinfo);
   return eb_vp9_num_groups == 1 ? sysinfo.dwNumberOfProcessors : sysinfo.dwNumberOfProcessors << 1;
#else
   return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/*****************************************
 * Process Input Ports Config
 *****************************************/

 // Rate Control
static RateControlPorts  rate_control_ports[] = {
    {RATE_CONTROL_INPUT_PORT_PICTURE_MANAGER,    0},
    {RATE_CONTROL_INPUT_PORT_PACKETIZATION,      0},
    {RATE_CONTROL_INPUT_PORT_ENTROPY_CODING,     0},
    {RATE_CONTROL_INPUT_PORT_INVALID,            0}
};

typedef struct
{
    int32_t   type;
    uint32_t  count;
} EncDecPorts;

#define ENCDEC_INPUT_PORT_MDC      0
#define ENCDEC_INPUT_PORT_ENCDEC   1
#define ENCDEC_INPUT_PORT_INVALID -1

// EncDec
static EncDecPorts enc_dec_ports[] = {
    {ENCDEC_INPUT_PORT_MDC,        0},
    {ENCDEC_INPUT_PORT_ENCDEC,     0},
    {ENCDEC_INPUT_PORT_INVALID,    0}
};

/*****************************************
 * Input Port Lookup
 *****************************************/

 // Rate Control
static uint32_t rate_control_port_lookup(
    RateControlInputPortTypes type,
    uint32_t                  port_type_index)
{
    uint32_t port_index = 0;
    uint32_t port_count = 0;

    while ((type != rate_control_ports[port_index].type) && (type != RATE_CONTROL_INPUT_PORT_INVALID)) {
        port_count += rate_control_ports[port_index++].count;
    }

    return (port_count + port_type_index);
}

// EncDec
static uint32_t enc_dec_port_lookup(
    int32_t   type,
    uint32_t  port_type_index)
{
    uint32_t port_index = 0;
    uint32_t port_count = 0;

    while ((type != enc_dec_ports[port_index].type) && (type != ENCDEC_INPUT_PORT_INVALID)) {
        port_count += enc_dec_ports[port_index++].count;
    }

    return (port_count + port_type_index);
}

/*****************************************
 * Input Port Total Count
 *****************************************/

 // Rate Control
static uint32_t rate_control_port_total_count(void)
{
    uint32_t port_index = 0;
    uint32_t total_count = 0;

    while (rate_control_ports[port_index].type != RATE_CONTROL_INPUT_PORT_INVALID) {
        total_count += rate_control_ports[port_index++].count;
    }

    return total_count;
}

// EncDec
static uint32_t enc_dec_port_total_count(void)
{
    uint32_t port_index = 0;
    uint32_t total_count = 0;

    while (enc_dec_ports[port_index].type != ENCDEC_INPUT_PORT_INVALID) {
        total_count += enc_dec_ports[port_index++].count;
    }

    return total_count;
}

EbErrorType init_thread_managment_params() {
#ifdef _WIN32
    // Initialize eb_vp9_group_affinity structure with Current thread info
    GetThreadGroupAffinity(GetCurrentThread(), &eb_vp9_group_affinity);
    eb_vp9_num_groups = (uint8_t)GetActiveProcessorGroupCount();
#else
    const char* PROCESSORID = "processor";
    const char* PHYSICALID = "physical id";
    int processor_id_len = EB_STRLEN(PROCESSORID, 128);
    int physical_id_len = EB_STRLEN(PHYSICALID, 128);
    if (processor_id_len < 0 || processor_id_len >= 128)
        return EB_ErrorInsufficientResources;
    if (physical_id_len < 0 || physical_id_len >= 128)
        return EB_ErrorInsufficientResources;
    memset(eb_vp9_lp_group, 0, sizeof(eb_vp9_lp_group));

    FILE *fin = fopen("/proc/cpuinfo", "r");
    if (fin) {
        int processor_id = 0, socket_id = 0;
        char line[1024];
        while (fgets(line, sizeof(line), fin)) {
            if (strncmp(line, PROCESSORID, processor_id_len) == 0) {
                char* p = line + processor_id_len;
                while (*p < '0' || *p > '9') p++;
                processor_id = strtol(p, NULL, 0);
            }
            if (strncmp(line, PHYSICALID, physical_id_len) == 0) {
                char* p = line + physical_id_len;
                while (*p < '0' || *p > '9') p++;
                socket_id = strtol(p, NULL, 0);
                if (socket_id < 0 || socket_id > 15) {
                    fclose(fin);
                return EB_ErrorInsufficientResources;
            }
            if (socket_id + 1 > eb_vp9_num_groups)
                eb_vp9_num_groups = socket_id + 1;
            eb_vp9_lp_group[socket_id].group[eb_vp9_lp_group[socket_id].num++] = processor_id;
        }
    }
    fclose(fin);
    }
#endif
    return EB_ErrorNone;
}

/**********************************
* Encoder Error Handling
**********************************/
void lib_svt_vp9_encoder_send_error_exit(
    EbPtr    h_component,
    uint32_t error_code) {

    EbComponentType      *svt_enc_component = (EbComponentType*)h_component;
    EbEncHandle          *p_enc_comp_data   = (EbEncHandle*)svt_enc_component->p_component_private;
    EbObjectWrapper      *eb_wrapper_ptr    = NULL;
    EbBufferHeaderType   *output_packet;

    eb_vp9_get_empty_object(
        (p_enc_comp_data->output_stream_buffer_producer_fifo_ptr_dbl_array[0])[0],
        &eb_wrapper_ptr);

    output_packet = (EbBufferHeaderType*)eb_wrapper_ptr->object_ptr;

    output_packet->size   = 0;
    output_packet->flags  = error_code;
    output_packet->p_buffer = NULL;

    eb_vp9_post_full_object(eb_wrapper_ptr);
}

/**********************************
 * Encoder Library Handle Constructor
 **********************************/
static EbErrorType  eb_enc_handle_ctor(
    EbEncHandle **enc_handle_dbl_ptr,
    EbHandleType  eb_handle_ptr){

    EbErrorType  return_error = EB_ErrorNone;
    // Allocate Memory
    EbEncHandle *enc_handle_ptr = (EbEncHandle*)malloc(sizeof(EbEncHandle));
    *enc_handle_dbl_ptr = enc_handle_ptr;
    if (enc_handle_ptr == (EbEncHandle*)EB_NULL) {
        return EB_ErrorInsufficientResources;
    }
    enc_handle_ptr->memory_map = (EbMemoryMapEntry*)malloc(sizeof(EbMemoryMapEntry) * MAX_NUM_PTR);
    enc_handle_ptr->memory_map_index = 0;
    enc_handle_ptr->total_lib_memory = sizeof(EbEncHandle) + sizeof(EbMemoryMapEntry) * MAX_NUM_PTR;

    // Save Memory Map Pointers
    total_lib_memory = &enc_handle_ptr->total_lib_memory;
    memory_map = enc_handle_ptr->memory_map;
    memory_map_index = &enc_handle_ptr->memory_map_index;
    lib_malloc_count = 0;
    lib_thread_count = 0;
    lib_mutex_count = 0;
    lib_semaphore_count = 0;

    if (memory_map == (EbMemoryMapEntry*)EB_NULL) {
        return EB_ErrorInsufficientResources;
    }

    return_error = init_thread_managment_params();
    if (return_error == EB_ErrorInsufficientResources)
        return EB_ErrorInsufficientResources;

    enc_handle_ptr->encode_instance_total_count = EB_EncodeInstancesTotalCount;

    EB_MALLOC(uint32_t*, enc_handle_ptr->compute_segments_total_count_array, sizeof(uint32_t) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    enc_handle_ptr->compute_segments_total_count_array[0]                 = EB_ComputeSegmentInitCount;

    // Config Set Count
    enc_handle_ptr->sequence_control_set_pool_total_count                 = EB_SequenceControlSetPoolInitCount;

    // Sequence Control Set Buffers
    enc_handle_ptr->sequence_control_set_pool_ptr                         = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->sequence_control_set_pool_producer_fifo_ptr_array     = (EbFifo**)EB_NULL;

    // Picture Buffers
    enc_handle_ptr->reference_picture_pool_ptr_array                      = (EbSystemResource**)EB_NULL;
    enc_handle_ptr->pa_reference_picture_pool_ptr_array                   = (EbSystemResource**)EB_NULL;

    // Picture Buffer Producer Fifos
    enc_handle_ptr->reference_picture_pool_producer_fifo_ptr_dbl_array    = (EbFifo***)EB_NULL;
    enc_handle_ptr->pa_reference_picture_pool_producer_fifo_ptr_dbl_array = (EbFifo***)EB_NULL;

    // Threads
    enc_handle_ptr->resource_coordination_thread_handle             = (EbHandle)EB_NULL;
    enc_handle_ptr->picture_analysis_thread_handle_array            = (EbHandle*)EB_NULL;
    enc_handle_ptr->picture_decision_thread_handle                  = (EbHandle)EB_NULL;
    enc_handle_ptr->motion_estimation_thread_handle_array           = (EbHandle*)EB_NULL;
    enc_handle_ptr->initial_rate_control_thread_handle              = (EbHandle)EB_NULL;
    enc_handle_ptr->source_based_operations_thread_handle_array     = (EbHandle*)EB_NULL;
    enc_handle_ptr->picture_manager_thread_handle                   = (EbHandle)EB_NULL;
    enc_handle_ptr->rate_control_thread_handle                      = (EbHandle)EB_NULL;
    enc_handle_ptr->mode_decision_configuration_thread_handle_array = (EbHandle*)EB_NULL;
    enc_handle_ptr->enc_dec_thread_handle_array                     = (EbHandle*)EB_NULL;
    enc_handle_ptr->entropy_coding_thread_handle_array              = (EbHandle*)EB_NULL;
    enc_handle_ptr->packetization_thread_handle                     = (EbHandle)EB_NULL;

    // Contexts
    enc_handle_ptr->resource_coordination_context_ptr             = (EbPtr)EB_NULL;
    enc_handle_ptr->picture_analysis_context_ptr_array            = (EbPtr *)EB_NULL;
    enc_handle_ptr->picture_decision_context_ptr                  = (EbPtr)EB_NULL;
    enc_handle_ptr->motion_estimation_context_ptr_array           = (EbPtr *)EB_NULL;
    enc_handle_ptr->initial_rate_control_context_ptr              = (EbPtr)EB_NULL;
    enc_handle_ptr->source_based_operations_context_ptr_array     = (EbPtr *)EB_NULL;
    enc_handle_ptr->picture_manager_context_ptr                   = (EbPtr)EB_NULL;
    enc_handle_ptr->rate_control_context_ptr                      = (EbPtr)EB_NULL;
    enc_handle_ptr->mode_decision_configuration_context_ptr_array = (EbPtr *)EB_NULL;
    enc_handle_ptr->enc_dec_context_ptr_array                     = (EbPtr *)EB_NULL;
    enc_handle_ptr->entropy_coding_context_ptr_array              = (EbPtr *)EB_NULL;
    enc_handle_ptr->packetization_context_ptr                     = (EbPtr)EB_NULL;

    // System Resource Managers
    enc_handle_ptr->input_buffer_resource_ptr                             = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->output_stream_buffer_resource_ptr_array               = (EbSystemResource**)EB_NULL;
    enc_handle_ptr->resource_coordination_results_resource_ptr            = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->picture_analysis_results_resource_ptr                 = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->picture_decision_results_resource_ptr                 = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->motion_estimation_results_resource_ptr                = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->initial_rate_control_results_resource_ptr             = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->picture_demux_results_resource_ptr                    = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->rate_control_tasks_resource_ptr                       = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->rate_control_results_resource_ptr                     = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->enc_dec_tasks_resource_ptr                            = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->enc_dec_results_resource_ptr                          = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->entropy_coding_results_resource_ptr                   = (EbSystemResource*)EB_NULL;
    enc_handle_ptr->output_recon_buffer_resource_ptr_array                = (EbSystemResource**)EB_NULL;

    // Inter-Process Producer Fifos
    enc_handle_ptr->input_buffer_producer_fifo_ptr_array                  = (EbFifo**)EB_NULL;
    enc_handle_ptr->output_stream_buffer_producer_fifo_ptr_dbl_array      = (EbFifo***)EB_NULL;
    enc_handle_ptr->resource_coordination_results_producer_fifo_ptr_array = (EbFifo**)EB_NULL;
    enc_handle_ptr->picture_demux_results_producer_fifo_ptr_array         = (EbFifo**)EB_NULL;
    enc_handle_ptr->picture_manager_results_producer_fifo_ptr_array       = (EbFifo**)EB_NULL;
    enc_handle_ptr->rate_control_tasks_producer_fifo_ptr_array            = (EbFifo**)EB_NULL;
    enc_handle_ptr->rate_control_results_producer_fifo_ptr_array          = (EbFifo**)EB_NULL;
    enc_handle_ptr->enc_dec_tasks_producer_fifo_ptr_array                 = (EbFifo**)EB_NULL;
    enc_handle_ptr->enc_dec_results_producer_fifo_ptr_array               = (EbFifo**)EB_NULL;
    enc_handle_ptr->entropy_coding_results_producer_fifo_ptr_array        = (EbFifo**)EB_NULL;
    enc_handle_ptr->output_recon_buffer_producer_fifo_ptr_dbl_array       = (EbFifo***)EB_NULL;

    // Inter-Process Consumer Fifos
    enc_handle_ptr->input_buffer_consumer_fifo_ptr_array                  = (EbFifo**)EB_NULL;
    enc_handle_ptr->output_stream_buffer_consumer_fifo_ptr_dbl_array      = (EbFifo***)EB_NULL;
    enc_handle_ptr->resource_coordination_results_consumer_fifo_ptr_array = (EbFifo**)EB_NULL;
    enc_handle_ptr->picture_demux_results_consumer_fifo_ptr_array         = (EbFifo**)EB_NULL;
    enc_handle_ptr->rate_control_tasks_consumer_fifo_ptr_array            = (EbFifo**)EB_NULL;
    enc_handle_ptr->rate_control_results_consumer_fifo_ptr_array          = (EbFifo**)EB_NULL;
    enc_handle_ptr->enc_dec_tasks_consumer_fifo_ptr_array                 = (EbFifo**)EB_NULL;
    enc_handle_ptr->enc_dec_results_consumer_fifo_ptr_array               = (EbFifo**)EB_NULL;
    enc_handle_ptr->entropy_coding_results_consumer_fifo_ptr_array        = (EbFifo**)EB_NULL;
    enc_handle_ptr->output_recon_buffer_consumer_fifo_ptr_dbl_array       = (EbFifo***)EB_NULL;

    // Initialize Callbacks
    EB_MALLOC(EbCallback  **, enc_handle_ptr->app_callback_ptr_array, sizeof(EbCallback  *) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    EB_MALLOC(EbCallback  *, enc_handle_ptr->app_callback_ptr_array[0], sizeof(EbCallback), EB_N_PTR);

    enc_handle_ptr->app_callback_ptr_array[0]->error_handler = lib_svt_vp9_encoder_send_error_exit;
    enc_handle_ptr->app_callback_ptr_array[0]->app_private_data                     = EB_NULL;
    enc_handle_ptr->app_callback_ptr_array[0]->handle                               = eb_handle_ptr;

    // Initialize Sequence Control Set Instance Array
    EB_MALLOC(EbSequenceControlSetInstance  **, enc_handle_ptr->sequence_control_set_instance_array, sizeof(EbSequenceControlSetInstance  *) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    return_error = eb_vp9_sequence_control_set_instance_ctor(&enc_handle_ptr->sequence_control_set_instance_array[0]);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}

#ifdef _WIN32
static uint64_t get_affinity_mask(uint32_t lpnum) {
    uint64_t mask = 0x1;
    for (uint32_t i = lpnum - 1; i > 0; i--)
        mask += (uint64_t)1 << i;
    return mask;
}
#endif

static void eb_set_thread_management_parameters( EbSvtVp9EncConfiguration *config_ptr){

    uint32_t num_logical_processors = get_num_cores();
#ifdef _WIN32
    // For system with a single processor group(no more than 64 logic processors all together)
    // Affinity of the thread can be set to one or more logical processors
    if (eb_vp9_num_groups == 1) {
        uint32_t lps = config_ptr->logical_processors == 0 ? num_logical_processors :
            config_ptr->logical_processors < num_logical_processors ? config_ptr->logical_processors : num_logical_processors;
        eb_vp9_group_affinity.Mask = get_affinity_mask(lps);
    }
    else if (eb_vp9_num_groups > 1) { // For system with multiple processor group
        if (config_ptr->logical_processors == 0) {
            if (config_ptr->target_socket != -1)
                eb_vp9_group_affinity.Group = config_ptr->target_socket;
        }
        else {
            uint32_t num_lp_per_group = num_logical_processors / eb_vp9_num_groups;
            if (config_ptr->target_socket == -1) {
                if (config_ptr->logical_processors > num_lp_per_group) {
                    eb_vp9_alternate_groups = EB_TRUE;
                    SVT_LOG("SVT [WARNING]: -lp(logical processors) setting is ignored. Run on both sockets. \n");
                }
                else
                    eb_vp9_group_affinity.Mask = get_affinity_mask(config_ptr->logical_processors);
            }
            else {
                uint32_t lps = config_ptr->logical_processors == 0 ? num_lp_per_group :
                    config_ptr->logical_processors < num_lp_per_group ? config_ptr->logical_processors : num_lp_per_group;
                eb_vp9_group_affinity.Mask = get_affinity_mask(lps);
                eb_vp9_group_affinity.Group = config_ptr->target_socket;
            }
        }
    }
#elif defined(__linux__)

    CPU_ZERO(&eb_vp9_group_affinity);

    if (eb_vp9_num_groups == 1) {
        uint32_t lps = config_ptr->logical_processors == 0 ? num_logical_processors :
            config_ptr->logical_processors < num_logical_processors ? config_ptr->logical_processors : num_logical_processors;
        for (uint32_t i = 0; i < lps; i++)
            CPU_SET(eb_vp9_lp_group[0].group[i], &eb_vp9_group_affinity);
    }
    else if (eb_vp9_num_groups > 1) {
        uint32_t num_lp_per_group = num_logical_processors / eb_vp9_num_groups;
        if (config_ptr->logical_processors == 0) {
            if (config_ptr->target_socket != -1) {
                for (uint32_t i = 0; i < eb_vp9_lp_group[config_ptr->target_socket].num; i++)
                    CPU_SET(eb_vp9_lp_group[config_ptr->target_socket].group[i], &eb_vp9_group_affinity);
            }
        }
        else {
            if (config_ptr->target_socket == -1) {
                uint32_t lps = config_ptr->logical_processors == 0 ? num_logical_processors :
                    config_ptr->logical_processors < num_logical_processors ? config_ptr->logical_processors : num_logical_processors;
                if (lps > num_lp_per_group) {
                    for (uint32_t i = 0; i < eb_vp9_lp_group[0].num; i++)
                        CPU_SET(eb_vp9_lp_group[0].group[i], &eb_vp9_group_affinity);
                    for (uint32_t i = 0; i < (lps - eb_vp9_lp_group[0].num); i++)
                        CPU_SET(eb_vp9_lp_group[1].group[i], &eb_vp9_group_affinity);
                }
                else {
                    for (uint32_t i = 0; i < lps; i++)
                        CPU_SET(eb_vp9_lp_group[0].group[i], &eb_vp9_group_affinity);
                }
            }
            else {
                uint32_t lps = config_ptr->logical_processors == 0 ? num_lp_per_group :
                    config_ptr->logical_processors < num_lp_per_group ? config_ptr->logical_processors : num_lp_per_group;
                for (uint32_t i = 0; i < lps; i++)
                    CPU_SET(eb_vp9_lp_group[config_ptr->target_socket].group[i], &eb_vp9_group_affinity);
            }
        }
    }
#endif
}

EbErrorType lib_allocate_frame_buffer(
    SequenceControlSet *sequence_control_set_ptr,
    EbBufferHeaderType *input_buffer)
{
    EbErrorType   return_error = EB_ErrorNone;
    EbPictureBufferDescInitData input_picture_buffer_desc_init_data;
    EbSvtVp9EncConfiguration   *config = &sequence_control_set_ptr->static_config;
    uint8_t is16bit = config->encoder_bit_depth > 8 ? 1 : 0;
    // Init Picture Init data
    input_picture_buffer_desc_init_data.max_width = (uint16_t)sequence_control_set_ptr->max_input_luma_width;
    input_picture_buffer_desc_init_data.max_height = (uint16_t)sequence_control_set_ptr->max_input_luma_height;
    input_picture_buffer_desc_init_data.bit_depth = config->encoder_bit_depth;

    input_picture_buffer_desc_init_data.buffer_enable_mask = is16bit ? PICTURE_BUFFER_DESC_FULL_MASK : 0;

    input_picture_buffer_desc_init_data.left_padding = sequence_control_set_ptr->left_padding;
    input_picture_buffer_desc_init_data.right_padding = sequence_control_set_ptr->right_padding;
    input_picture_buffer_desc_init_data.top_padding = sequence_control_set_ptr->top_padding;
    input_picture_buffer_desc_init_data.bot_padding = sequence_control_set_ptr->bot_padding;

    input_picture_buffer_desc_init_data.split_mode = is16bit ? EB_TRUE : EB_FALSE;

    input_picture_buffer_desc_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;

    // Enhanced Picture Buffer
    return_error = eb_vp9_picture_buffer_desc_ctor(
        (EbPtr*) &(input_buffer->p_buffer),
        (EbPtr)&input_picture_buffer_desc_init_data);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    return return_error;
}

/**************************************
* EbBufferHeaderType Constructor
**************************************/
static EbErrorType eb_input_buffer_header_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    EbBufferHeaderType        *input_buffer;
    SequenceControlSet        *sequence_control_set_ptr = (SequenceControlSet*)object_init_data_ptr;
    EB_MALLOC(EbBufferHeaderType*, input_buffer, sizeof(EbBufferHeaderType), EB_N_PTR);
    *object_dbl_ptr = (EbPtr)input_buffer;
    // Initialize Header
    input_buffer->size = sizeof(EbBufferHeaderType);

    lib_allocate_frame_buffer(
        sequence_control_set_ptr,
        input_buffer);

    input_buffer->p_app_private = NULL;

    return EB_ErrorNone;
}
EbErrorType eb_output_recon_buffer_header_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    EbBufferHeaderType        *recon_buffer;
    SequenceControlSet        *sequence_control_set_ptr = (SequenceControlSet*)object_init_data_ptr;
    const uint32_t luma_size =
        sequence_control_set_ptr->luma_width    *
        sequence_control_set_ptr->luma_height;
    // both u and v
    const uint32_t chroma_size = luma_size >> 1;
    const uint32_t ten_bit = (sequence_control_set_ptr->static_config.encoder_bit_depth > 8);
    const uint32_t frame_size = (luma_size + chroma_size) << ten_bit;

    EB_MALLOC(EbBufferHeaderType*, recon_buffer, sizeof(EbBufferHeaderType), EB_N_PTR);
    *object_dbl_ptr = (EbPtr)recon_buffer;

    // Initialize Header
    recon_buffer->size = sizeof(EbBufferHeaderType);

    // Assign the variables
    EB_MALLOC(uint8_t*, recon_buffer->p_buffer, frame_size, EB_N_PTR);

    recon_buffer->n_alloc_len = frame_size;
    recon_buffer->p_app_private = NULL;

    return EB_ErrorNone;
}
/**************************************
* EbBufferHeaderType Constructor
**************************************/
static EbErrorType eb_output_buffer_header_ctor(
    EbPtr *object_dbl_ptr,
    EbPtr  object_init_data_ptr)
{
    EbSvtVp9EncConfiguration   * config = (EbSvtVp9EncConfiguration*)object_init_data_ptr;
    uint32_t size = (uint32_t)(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->source_width * config->source_height));  //TBC
    EbBufferHeaderType* out_buf_ptr;

    EB_MALLOC(EbBufferHeaderType*, out_buf_ptr, sizeof(EbBufferHeaderType), EB_N_PTR);
    *object_dbl_ptr = (EbPtr)out_buf_ptr;

    // Initialize Header
    out_buf_ptr->size = sizeof(EbBufferHeaderType);

    EB_MALLOC(uint8_t*, out_buf_ptr->p_buffer, size, EB_N_PTR);

    out_buf_ptr->n_alloc_len = size;
    out_buf_ptr->p_app_private = NULL;

    return EB_ErrorNone;
}

/**********************************
 * Initialize Encoder Library
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EbErrorType  eb_vp9_init_encoder(EbComponentType *svt_enc_component) {

    EbEncHandle *enc_handle_ptr = (EbEncHandle*)svt_enc_component->p_component_private;
    EbErrorType  return_error = EB_ErrorNone;
    uint32_t process_index;
    uint32_t maxpicture_width;
    uint32_t max_look_ahead_distance = 0;
    SequenceControlSet *scs_ptr = enc_handle_ptr->sequence_control_set_instance_array[0]->sequence_control_set_ptr;

    EB_BOOL is16bit = (EB_BOOL)(scs_ptr->static_config.encoder_bit_depth > EB_8BIT);

    /************************************
    * Platform detection
    ************************************/

    if (scs_ptr->static_config.asm_type == ASM_AVX2) {
        eb_vp9_ASM_TYPES = get_cpu_asm_type();
    }
    else if (scs_ptr->static_config.asm_type == ASM_NON_AVX2) {
        eb_vp9_ASM_TYPES = 0;
    }

    setup_rtcd_internal(eb_vp9_ASM_TYPES);
    setup_rtcd_internal_vp9(eb_vp9_ASM_TYPES);
    build_ep_block_stats();

    /************************************
     * Sequence Control Set
     ************************************/
    return_error = eb_vp9_system_resource_ctor(
        &enc_handle_ptr->sequence_control_set_pool_ptr,
        enc_handle_ptr->sequence_control_set_pool_total_count,
        1,
        0,
        &enc_handle_ptr->sequence_control_set_pool_producer_fifo_ptr_array,
        (EbFifo ***)EB_NULL,
        EB_FALSE,
        eb_vp9_sequence_control_set_ctor,
        EB_NULL);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
    /************************************
     * Picture Control Set: Parent
     ************************************/
    EB_MALLOC(EbSystemResource**, enc_handle_ptr->picture_parent_control_set_pool_ptr_array, sizeof(EbSystemResource*)  * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
    EB_MALLOC(EbFifo***, enc_handle_ptr->picture_parent_control_set_pool_producer_fifo_ptr_dbl_array, sizeof(EbSystemResource**) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    // Updating the picture_control_set_pool_total_count based on the maximum look ahead distance
    max_look_ahead_distance = MAX(max_look_ahead_distance, scs_ptr->look_ahead_distance);

    // The segment Width & Height Arrays are in units of LCUs, not samples
    PictureControlSetInitData input_data;

    input_data.picture_width = scs_ptr->max_input_luma_width;
    input_data.picture_height = scs_ptr->max_input_luma_height;
    input_data.left_padding = scs_ptr->left_padding;
    input_data.right_padding = scs_ptr->right_padding;
    input_data.top_padding = scs_ptr->top_padding;
    input_data.bot_padding = scs_ptr->bot_padding;
    input_data.bit_depth = scs_ptr->output_bitdepth;
    input_data.is16bit = is16bit;

    input_data.enc_mode = scs_ptr->static_config.enc_mode;
    input_data.speed_control = (uint8_t)scs_ptr->static_config.speed_control_flag;
    input_data.tune = scs_ptr->static_config.tune;
    return_error = eb_vp9_system_resource_ctor(
        &(enc_handle_ptr->picture_parent_control_set_pool_ptr_array[0]),
        scs_ptr->picture_control_set_pool_init_count,//enc_handle_ptr->picture_control_set_pool_total_count,
        1,
        0,
        &enc_handle_ptr->picture_parent_control_set_pool_producer_fifo_ptr_dbl_array[0],
        (EbFifo ***)EB_NULL,
        EB_FALSE,
        eb_vp9_picture_parent_control_set_ctor,
        &input_data);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    /************************************
     * Picture Control Set: Child
     ************************************/
    EB_MALLOC(EbSystemResource**, enc_handle_ptr->picture_control_set_pool_ptr_array, sizeof(EbSystemResource*)  * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
    EB_MALLOC(EbFifo***, enc_handle_ptr->picture_control_set_pool_producer_fifo_ptr_dbl_array, sizeof(EbSystemResource**) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    // The segment Width & Height Arrays are in units of LCUs, not samples
    {
        PictureControlSetInitData input_data;
        unsigned i;

        input_data.enc_dec_segment_col = 0;
        input_data.enc_dec_segment_row = 0;

        for (i = 0; i <= scs_ptr->hierarchical_levels; ++i) {
            input_data.enc_dec_segment_col = scs_ptr->enc_dec_segment_col_count_array[i] > input_data.enc_dec_segment_col ?
                (uint16_t)scs_ptr->enc_dec_segment_col_count_array[i] :
                input_data.enc_dec_segment_col;
            input_data.enc_dec_segment_row = scs_ptr->enc_dec_segment_row_count_array[i] > input_data.enc_dec_segment_row ?
                (uint16_t)scs_ptr->enc_dec_segment_row_count_array[i] :
                input_data.enc_dec_segment_row;
        }

        input_data.picture_width = scs_ptr->max_input_luma_width;
        input_data.picture_height = scs_ptr->max_input_luma_height;
        input_data.left_padding = scs_ptr->left_padding;
        input_data.right_padding = scs_ptr->right_padding;
        input_data.top_padding = scs_ptr->top_padding;
        input_data.bot_padding = scs_ptr->bot_padding;
        input_data.bit_depth = EB_8BIT;
        input_data.is16bit = is16bit;
        return_error = eb_vp9_system_resource_ctor(
            &(enc_handle_ptr->picture_control_set_pool_ptr_array[0]),
            scs_ptr->picture_control_set_pool_init_count_child, //EB_PictureControlSetPoolInitCountChild,
            1,
            0,
            &enc_handle_ptr->picture_control_set_pool_producer_fifo_ptr_dbl_array[0],
            (EbFifo ***)EB_NULL,
            EB_FALSE,
            eb_vp9_picture_control_set_ctor,
            &input_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    /************************************
     * Picture Buffers
     ************************************/

     // Allocate Resource Arrays
    EB_MALLOC(EbSystemResource**, enc_handle_ptr->reference_picture_pool_ptr_array, sizeof(EbSystemResource*) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
    EB_MALLOC(EbSystemResource**, enc_handle_ptr->pa_reference_picture_pool_ptr_array, sizeof(EbSystemResource*) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    // Allocate Producer Fifo Arrays
    EB_MALLOC(EbFifo***, enc_handle_ptr->reference_picture_pool_producer_fifo_ptr_dbl_array, sizeof(EbFifo**) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
    EB_MALLOC(EbFifo***, enc_handle_ptr->pa_reference_picture_pool_producer_fifo_ptr_dbl_array, sizeof(EbFifo**) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    // Rate Control
    rate_control_ports[0].count = EB_PictureManagerProcessInitCount;
    rate_control_ports[1].count = EB_PacketizationProcessInitCount;
    rate_control_ports[2].count = scs_ptr->entropy_coding_process_init_count;
    rate_control_ports[3].count = 0;

    enc_dec_ports[ENCDEC_INPUT_PORT_MDC].count = scs_ptr->mode_decision_configuration_process_init_count;
    enc_dec_ports[ENCDEC_INPUT_PORT_ENCDEC].count = scs_ptr->enc_dec_process_init_count;

    EbReferenceObjectDescInitData     eb_reference_object_desc_init_data_structure;
    EbPaReferenceObjectDescInitData   eb_pa_reference_object_desc_init_data_structure;
    EbPictureBufferDescInitData       reference_picture_buffer_desc_init_data;
    EbPictureBufferDescInitData       quarter_decim_picture_buffer_desc_init_data;
    EbPictureBufferDescInitData       sixteenth_decim_picture_buffer_desc_init_data;

    // Initialize the various Picture types
    reference_picture_buffer_desc_init_data.max_width = scs_ptr->max_input_luma_width;
    reference_picture_buffer_desc_init_data.max_height = scs_ptr->max_input_luma_height;
    reference_picture_buffer_desc_init_data.bit_depth = scs_ptr->input_bit_depth;
    reference_picture_buffer_desc_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_FULL_MASK;
    reference_picture_buffer_desc_init_data.left_padding = MAX_SB_SIZE + MCPXPaddingOffset;
    reference_picture_buffer_desc_init_data.right_padding = MAX_SB_SIZE + MCPXPaddingOffset;
    reference_picture_buffer_desc_init_data.top_padding = MAX_SB_SIZE + MCPYPaddingOffset;
    reference_picture_buffer_desc_init_data.bot_padding = MAX_SB_SIZE + MCPYPaddingOffset;
    reference_picture_buffer_desc_init_data.split_mode = EB_FALSE;

    if (is16bit) {
        reference_picture_buffer_desc_init_data.bit_depth = EB_10BIT;
    }

    eb_reference_object_desc_init_data_structure.reference_picture_desc_init_data = reference_picture_buffer_desc_init_data;

    // Reference Picture Buffers
    return_error = eb_vp9_system_resource_ctor(
        &enc_handle_ptr->reference_picture_pool_ptr_array[0],
        scs_ptr->reference_picture_buffer_init_count,//enc_handle_ptr->reference_picture_pool_total_count,
        EB_PictureManagerProcessInitCount,
        0,
        &enc_handle_ptr->reference_picture_pool_producer_fifo_ptr_dbl_array[0],
        (EbFifo ***)EB_NULL,
        EB_FALSE,
        eb_vp9_reference_object_ctor,
        &(eb_reference_object_desc_init_data_structure));

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // PA Reference Picture Buffers
    // Currently, only Luma samples are needed in the PA
    reference_picture_buffer_desc_init_data.max_width = scs_ptr->max_input_luma_width;
    reference_picture_buffer_desc_init_data.max_height = scs_ptr->max_input_luma_height;
    reference_picture_buffer_desc_init_data.bit_depth = scs_ptr->input_bit_depth;
    reference_picture_buffer_desc_init_data.buffer_enable_mask = 0;
    reference_picture_buffer_desc_init_data.left_padding = MAX_SB_SIZE + ME_FILTER_TAP;
    reference_picture_buffer_desc_init_data.right_padding = MAX_SB_SIZE + ME_FILTER_TAP;
    reference_picture_buffer_desc_init_data.top_padding = MAX_SB_SIZE + ME_FILTER_TAP;
    reference_picture_buffer_desc_init_data.bot_padding = MAX_SB_SIZE + ME_FILTER_TAP;
    reference_picture_buffer_desc_init_data.split_mode = EB_FALSE;

    quarter_decim_picture_buffer_desc_init_data.max_width = scs_ptr->max_input_luma_width >> 1;
    quarter_decim_picture_buffer_desc_init_data.max_height = scs_ptr->max_input_luma_height >> 1;
    quarter_decim_picture_buffer_desc_init_data.bit_depth = scs_ptr->input_bit_depth;
    quarter_decim_picture_buffer_desc_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_LUMA_MASK;
    quarter_decim_picture_buffer_desc_init_data.left_padding = MAX_SB_SIZE >> 1;
    quarter_decim_picture_buffer_desc_init_data.right_padding = MAX_SB_SIZE >> 1;
    quarter_decim_picture_buffer_desc_init_data.top_padding = MAX_SB_SIZE >> 1;
    quarter_decim_picture_buffer_desc_init_data.bot_padding = MAX_SB_SIZE >> 1;
    quarter_decim_picture_buffer_desc_init_data.split_mode = EB_FALSE;

    sixteenth_decim_picture_buffer_desc_init_data.max_width = scs_ptr->max_input_luma_width >> 2;
    sixteenth_decim_picture_buffer_desc_init_data.max_height = scs_ptr->max_input_luma_height >> 2;
    sixteenth_decim_picture_buffer_desc_init_data.bit_depth = scs_ptr->input_bit_depth;
    sixteenth_decim_picture_buffer_desc_init_data.buffer_enable_mask = PICTURE_BUFFER_DESC_LUMA_MASK;
    sixteenth_decim_picture_buffer_desc_init_data.left_padding = MAX_SB_SIZE >> 2;
    sixteenth_decim_picture_buffer_desc_init_data.right_padding = MAX_SB_SIZE >> 2;
    sixteenth_decim_picture_buffer_desc_init_data.top_padding = MAX_SB_SIZE >> 2;
    sixteenth_decim_picture_buffer_desc_init_data.bot_padding = MAX_SB_SIZE >> 2;
    sixteenth_decim_picture_buffer_desc_init_data.split_mode = EB_FALSE;

    eb_pa_reference_object_desc_init_data_structure.reference_picture_desc_init_data = reference_picture_buffer_desc_init_data;
    eb_pa_reference_object_desc_init_data_structure.quarter_picture_desc_init_data = quarter_decim_picture_buffer_desc_init_data;
    eb_pa_reference_object_desc_init_data_structure.sixteenth_picture_desc_init_data = sixteenth_decim_picture_buffer_desc_init_data;

    // Reference Picture Buffers
    return_error = eb_vp9_system_resource_ctor(
        &enc_handle_ptr->pa_reference_picture_pool_ptr_array[0],
        scs_ptr->pa_reference_picture_buffer_init_count,
        EB_PictureDecisionProcessInitCount,
        0,
        &enc_handle_ptr->pa_reference_picture_pool_producer_fifo_ptr_dbl_array[0],
        (EbFifo ***)EB_NULL,
        EB_FALSE,
        eb_vp9_pa_reference_object_ctor,
        &(eb_pa_reference_object_desc_init_data_structure));
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // Set the SequenceControlSet Picture Pool Fifo Ptrs
    scs_ptr->encode_context_ptr->reference_picture_pool_fifo_ptr = (enc_handle_ptr->reference_picture_pool_producer_fifo_ptr_dbl_array[0])[0];
    scs_ptr->encode_context_ptr->pa_reference_picture_pool_fifo_ptr = (enc_handle_ptr->pa_reference_picture_pool_producer_fifo_ptr_dbl_array[0])[0];

    /************************************
     * System Resource Managers & Fifos
     ************************************/

     // EbBufferHeaderType Input
    return_error = eb_vp9_system_resource_ctor(
        &enc_handle_ptr->input_buffer_resource_ptr,
        scs_ptr->input_output_buffer_fifo_init_count,
        1,
        EB_ResourceCoordinationProcessInitCount,
        &enc_handle_ptr->input_buffer_producer_fifo_ptr_array,
        &enc_handle_ptr->input_buffer_consumer_fifo_ptr_array,
        EB_TRUE,
        eb_input_buffer_header_ctor,
        scs_ptr);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
    // EbBufferHeaderType Output Stream
    EB_MALLOC(EbSystemResource**, enc_handle_ptr->output_stream_buffer_resource_ptr_array, sizeof(EbSystemResource*) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
    EB_MALLOC(EbFifo***, enc_handle_ptr->output_stream_buffer_producer_fifo_ptr_dbl_array, sizeof(EbFifo**)          * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
    EB_MALLOC(EbFifo***, enc_handle_ptr->output_stream_buffer_consumer_fifo_ptr_dbl_array, sizeof(EbFifo**)          * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

    return_error = eb_vp9_system_resource_ctor(
        &enc_handle_ptr->output_stream_buffer_resource_ptr_array[0],
        scs_ptr->input_output_buffer_fifo_init_count + 6,
        scs_ptr->total_process_init_count,
        1,
        &enc_handle_ptr->output_stream_buffer_producer_fifo_ptr_dbl_array[0],
        &enc_handle_ptr->output_stream_buffer_consumer_fifo_ptr_dbl_array[0],
        EB_TRUE,
        eb_output_buffer_header_ctor,
        &scs_ptr->static_config);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
    if (scs_ptr->static_config.recon_file){
        // EbBufferHeaderType Output Recon
        EB_MALLOC(EbSystemResource**, enc_handle_ptr->output_recon_buffer_resource_ptr_array, sizeof(EbSystemResource*) * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
        EB_MALLOC(EbFifo***, enc_handle_ptr->output_recon_buffer_producer_fifo_ptr_dbl_array, sizeof(EbFifo**)          * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);
        EB_MALLOC(EbFifo***, enc_handle_ptr->output_recon_buffer_consumer_fifo_ptr_dbl_array, sizeof(EbFifo**)          * enc_handle_ptr->encode_instance_total_count, EB_N_PTR);

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->output_recon_buffer_resource_ptr_array[0],
            scs_ptr->output_recon_buffer_fifo_init_count,
            EB_PacketizationProcessInitCount,
            1,
            &enc_handle_ptr->output_recon_buffer_producer_fifo_ptr_dbl_array[0],
            &enc_handle_ptr->output_recon_buffer_consumer_fifo_ptr_dbl_array[0],
            EB_TRUE,
            eb_output_recon_buffer_header_ctor,
            scs_ptr);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Resource Coordination Results
    {
        ResourceCoordinationResultInitData resource_coordination_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->resource_coordination_results_resource_ptr,
            scs_ptr->resource_coordination_fifo_init_count,
            EB_ResourceCoordinationProcessInitCount,
            scs_ptr->picture_analysis_process_init_count,
            &enc_handle_ptr->resource_coordination_results_producer_fifo_ptr_array,
            &enc_handle_ptr->resource_coordination_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_resource_coordination_result_ctor,
            &resource_coordination_result_init_data);

        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Analysis Results
    {
        PictureAnalysisResultInitData picture_analysis_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->picture_analysis_results_resource_ptr,
            scs_ptr->picture_analysis_fifo_init_count,
            scs_ptr->picture_analysis_process_init_count,
            EB_PictureDecisionProcessInitCount,
            &enc_handle_ptr->picture_analysis_results_producer_fifo_ptr_array,
            &enc_handle_ptr->picture_analysis_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_picture_analysis_result_ctor,
            &picture_analysis_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Decision Results
    {
        PictureDecisionResultInitData picture_decision_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->picture_decision_results_resource_ptr,
            scs_ptr->picture_decision_fifo_init_count,
            EB_PictureDecisionProcessInitCount,
            scs_ptr->motion_estimation_process_init_count,
            &enc_handle_ptr->picture_decision_results_producer_fifo_ptr_array,
            &enc_handle_ptr->picture_decision_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_picture_decision_result_ctor,
            &picture_decision_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Motion Estimation Results
    {
        MotionEstimationResultsInitData motion_estimation_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->motion_estimation_results_resource_ptr,
            scs_ptr->motion_estimation_fifo_init_count,
            scs_ptr->motion_estimation_process_init_count,
            EB_InitialRateControlProcessInitCount,
            &enc_handle_ptr->motion_estimation_results_producer_fifo_ptr_array,
            &enc_handle_ptr->motion_estimation_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_motion_estimation_results_ctor,
            &motion_estimation_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Initial Rate Control Results
    {
        InitialRateControlResultInitData initial_rate_control_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->initial_rate_control_results_resource_ptr,
            scs_ptr->initial_rate_control_fifo_init_count,
            EB_InitialRateControlProcessInitCount,
            scs_ptr->source_based_operations_process_init_count,
            &enc_handle_ptr->initial_rate_control_results_producer_fifo_ptr_array,
            &enc_handle_ptr->initial_rate_control_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_initial_eb_vp9_rate_control_results_ctor,
            &initial_rate_control_result_init_data);

        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Demux Results
    {
        PictureResultInitData picture_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->picture_demux_results_resource_ptr,
            scs_ptr->picture_demux_fifo_init_count,
            scs_ptr->source_based_operations_process_init_count + scs_ptr->enc_dec_process_init_count,
            EB_PictureManagerProcessInitCount,
            &enc_handle_ptr->picture_demux_results_producer_fifo_ptr_array,
            &enc_handle_ptr->picture_demux_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_picture_results_ctor,
            &picture_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Rate Control Tasks
    {
        RateControlTasksInitData rate_control_tasks_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->rate_control_tasks_resource_ptr,
            scs_ptr->rate_control_tasks_fifo_init_count,
            rate_control_port_total_count(),
            EB_RateControlProcessInitCount,
            &enc_handle_ptr->rate_control_tasks_producer_fifo_ptr_array,
            &enc_handle_ptr->rate_control_tasks_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_rate_control_tasks_ctor,
            &rate_control_tasks_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Rate Control Results
    {
        RateControlResultsInitData rate_control_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->rate_control_results_resource_ptr,
            scs_ptr->rate_control_fifo_init_count,
            EB_RateControlProcessInitCount,
            scs_ptr->mode_decision_configuration_process_init_count,
            &enc_handle_ptr->rate_control_results_producer_fifo_ptr_array,
            &enc_handle_ptr->rate_control_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_rate_control_results_ctor,
            &rate_control_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // EncDec Tasks
    {
        EncDecTasksInitData mode_decision_result_init_data;
        unsigned i;

        mode_decision_result_init_data.enc_dec_segment_row_count = 0;

        for (i = 0; i <= scs_ptr->hierarchical_levels; ++i) {
            mode_decision_result_init_data.enc_dec_segment_row_count = MAX(
                mode_decision_result_init_data.enc_dec_segment_row_count,
                scs_ptr->enc_dec_segment_row_count_array[i]);
        }

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->enc_dec_tasks_resource_ptr,
            scs_ptr->mode_decision_configuration_fifo_init_count,
            enc_dec_port_total_count(),
            scs_ptr->enc_dec_process_init_count,
            &enc_handle_ptr->enc_dec_tasks_producer_fifo_ptr_array,
            &enc_handle_ptr->enc_dec_tasks_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_enc_dec_tasks_ctor,
            &mode_decision_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // EncDec Results
    {
        EncDecResultsInitData enc_dec_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->enc_dec_results_resource_ptr,
            scs_ptr->enc_dec_fifo_init_count,
            scs_ptr->enc_dec_process_init_count,
            scs_ptr->entropy_coding_process_init_count,
            &enc_handle_ptr->enc_dec_results_producer_fifo_ptr_array,
            &enc_handle_ptr->enc_dec_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_enc_dec_results_ctor,
            &enc_dec_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Entropy Coding Results
    {
        EntropyCodingResultsInitData entropy_coding_result_init_data;

        return_error = eb_vp9_system_resource_ctor(
            &enc_handle_ptr->entropy_coding_results_resource_ptr,
            scs_ptr->entropy_coding_fifo_init_count,
            scs_ptr->entropy_coding_process_init_count,
            EB_PacketizationProcessInitCount,
            &enc_handle_ptr->entropy_coding_results_producer_fifo_ptr_array,
            &enc_handle_ptr->entropy_coding_results_consumer_fifo_ptr_array,
            EB_TRUE,
            eb_vp9_entropy_coding_results_ctor,
            &entropy_coding_result_init_data);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    /************************************
     * App Callbacks
     ************************************/
    scs_ptr->encode_context_ptr->app_callback_ptr = enc_handle_ptr->app_callback_ptr_array[0];

    // Output Buffer Fifo Ptrs
    scs_ptr->encode_context_ptr->stream_output_fifo_ptr = (enc_handle_ptr->output_stream_buffer_producer_fifo_ptr_dbl_array[0])[0];
    if (scs_ptr->static_config.recon_file)
        scs_ptr->encode_context_ptr->recon_output_fifo_ptr = (enc_handle_ptr->output_recon_buffer_producer_fifo_ptr_dbl_array[0])[0];
    /************************************
     * Contexts
     ************************************/

     // Resource Coordination Context
    return_error = eb_vp9_resource_coordination_context_ctor(
        (ResourceCoordinationContext**)&enc_handle_ptr->resource_coordination_context_ptr,
        enc_handle_ptr->input_buffer_consumer_fifo_ptr_array[0],
        enc_handle_ptr->resource_coordination_results_producer_fifo_ptr_array[0],
        enc_handle_ptr->picture_parent_control_set_pool_producer_fifo_ptr_dbl_array[0],//ResourceCoordination works with ParentPCS
        enc_handle_ptr->sequence_control_set_instance_array,
        enc_handle_ptr->sequence_control_set_pool_producer_fifo_ptr_array[0],
        enc_handle_ptr->app_callback_ptr_array,
        enc_handle_ptr->compute_segments_total_count_array,
        enc_handle_ptr->encode_instance_total_count);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
    // Picture Analysis Context
    EB_MALLOC(EbPtr*, enc_handle_ptr->picture_analysis_context_ptr_array, sizeof(EbPtr) * scs_ptr->picture_analysis_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->picture_analysis_process_init_count; ++process_index) {

        EbPictureBufferDescInitData  picture_buffer_desc_conf;
        picture_buffer_desc_conf.max_width = scs_ptr->max_input_luma_width;
        picture_buffer_desc_conf.max_height = scs_ptr->max_input_luma_height;
        picture_buffer_desc_conf.bit_depth = EB_8BIT;
        picture_buffer_desc_conf.buffer_enable_mask = PICTURE_BUFFER_DESC_Y_FLAG;
        picture_buffer_desc_conf.left_padding = 0;
        picture_buffer_desc_conf.right_padding = 0;
        picture_buffer_desc_conf.top_padding = 0;
        picture_buffer_desc_conf.bot_padding = 0;
        picture_buffer_desc_conf.split_mode = EB_FALSE;

        return_error = eb_vp9_picture_analysis_context_ctor(
            &picture_buffer_desc_conf,
            EB_TRUE,
            (PictureAnalysisContext**)&enc_handle_ptr->picture_analysis_context_ptr_array[process_index],
            enc_handle_ptr->resource_coordination_results_consumer_fifo_ptr_array[process_index],
            enc_handle_ptr->picture_analysis_results_producer_fifo_ptr_array[process_index],
            ((scs_ptr->max_input_luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE) *
            ((scs_ptr->max_input_luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE));

        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Decision Context
    return_error = eb_vp9_picture_decision_context_ctor(
        (PictureDecisionContext**)&enc_handle_ptr->picture_decision_context_ptr,
        enc_handle_ptr->picture_analysis_results_consumer_fifo_ptr_array[0],
        enc_handle_ptr->picture_decision_results_producer_fifo_ptr_array[0]);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // Motion Analysis Context
    EB_MALLOC(EbPtr*, enc_handle_ptr->motion_estimation_context_ptr_array, sizeof(EbPtr) * scs_ptr->motion_estimation_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->motion_estimation_process_init_count; ++process_index) {

        return_error = eb_vp9_motion_estimation_context_ctor(
            (MotionEstimationContext**)&enc_handle_ptr->motion_estimation_context_ptr_array[process_index],
            enc_handle_ptr->picture_decision_results_consumer_fifo_ptr_array[process_index],
            enc_handle_ptr->motion_estimation_results_producer_fifo_ptr_array[process_index]);

        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Initial Rate Control Context
    return_error = eb_vp9_initial_eb_vp9_rate_control_context_ctor(
        (InitialRateControlContext**)&enc_handle_ptr->initial_rate_control_context_ptr,
        enc_handle_ptr->motion_estimation_results_consumer_fifo_ptr_array[0],
        enc_handle_ptr->initial_rate_control_results_producer_fifo_ptr_array[0]);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // Source Based Operations Context
    EB_MALLOC(EbPtr*, enc_handle_ptr->source_based_operations_context_ptr_array, sizeof(EbPtr) * scs_ptr->source_based_operations_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->source_based_operations_process_init_count; ++process_index) {
        return_error = eb_vp9_source_based_operations_context_ctor(
            (SourceBasedOperationsContext**)&enc_handle_ptr->source_based_operations_context_ptr_array[process_index],
            enc_handle_ptr->initial_rate_control_results_consumer_fifo_ptr_array[process_index],
            enc_handle_ptr->picture_demux_results_producer_fifo_ptr_array[process_index]);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Picture Manager Context
    return_error = eb_vp9_picture_manager_context_ctor(
        (PictureManagerContext**)&enc_handle_ptr->picture_manager_context_ptr,
        enc_handle_ptr->picture_demux_results_consumer_fifo_ptr_array[0],
        enc_handle_ptr->rate_control_tasks_producer_fifo_ptr_array[rate_control_port_lookup(RATE_CONTROL_INPUT_PORT_PICTURE_MANAGER, 0)],
        enc_handle_ptr->picture_control_set_pool_producer_fifo_ptr_dbl_array[0]);//The Child PCS Pool here
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
    // Rate Control Context
    return_error = eb_vp9_rate_control_context_ctor(
        (RateControlContext**)&enc_handle_ptr->rate_control_context_ptr,
        enc_handle_ptr->rate_control_tasks_consumer_fifo_ptr_array[0],
        enc_handle_ptr->rate_control_results_producer_fifo_ptr_array[0],
        scs_ptr->intra_period);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // Mode Decision Configuration Contexts
    {
        // Mode Decision Configuration Contexts
        EB_MALLOC(EbPtr*, enc_handle_ptr->mode_decision_configuration_context_ptr_array, sizeof(EbPtr) * scs_ptr->mode_decision_configuration_process_init_count, EB_N_PTR);

        for (process_index = 0; process_index < scs_ptr->mode_decision_configuration_process_init_count; ++process_index) {
            return_error = eb_vp9_mode_decision_configuration_context_ctor(
                (ModeDecisionConfigurationContext**)&enc_handle_ptr->mode_decision_configuration_context_ptr_array[process_index],
                enc_handle_ptr->rate_control_results_consumer_fifo_ptr_array[process_index],

                enc_handle_ptr->enc_dec_tasks_producer_fifo_ptr_array[enc_dec_port_lookup(ENCDEC_INPUT_PORT_MDC, process_index)],
                ((scs_ptr->max_input_luma_width + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE) *
                ((scs_ptr->max_input_luma_height + MAX_SB_SIZE_MINUS_1) / MAX_SB_SIZE));

            if (return_error == EB_ErrorInsufficientResources) {
                return EB_ErrorInsufficientResources;
            }
        }
    }

    maxpicture_width = 0;
    if (maxpicture_width < scs_ptr->max_input_luma_width) {
        maxpicture_width = scs_ptr->max_input_luma_width;
    }

    // EncDec Contexts
    EB_MALLOC(EbPtr*, enc_handle_ptr->enc_dec_context_ptr_array, sizeof(EbPtr) * scs_ptr->enc_dec_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->enc_dec_process_init_count; ++process_index) {
        return_error = eb_vp9_enc_dec_context_ctor(
            (EncDecContext**)&enc_handle_ptr->enc_dec_context_ptr_array[process_index],
            enc_handle_ptr->enc_dec_tasks_consumer_fifo_ptr_array[process_index],
            enc_handle_ptr->enc_dec_results_producer_fifo_ptr_array[process_index],
            enc_handle_ptr->enc_dec_tasks_producer_fifo_ptr_array[enc_dec_port_lookup(ENCDEC_INPUT_PORT_ENCDEC, process_index)],
            enc_handle_ptr->picture_demux_results_producer_fifo_ptr_array[1 + process_index]);

        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Entropy Coding Contexts
    EB_MALLOC(EbPtr*, enc_handle_ptr->entropy_coding_context_ptr_array, sizeof(EbPtr) * scs_ptr->entropy_coding_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->entropy_coding_process_init_count; ++process_index) {
        return_error = eb_vp9_entropy_coding_context_ctor(
            (EntropyCodingContext**)&enc_handle_ptr->entropy_coding_context_ptr_array[process_index],
            enc_handle_ptr->enc_dec_results_consumer_fifo_ptr_array[process_index],
            enc_handle_ptr->entropy_coding_results_producer_fifo_ptr_array[process_index],
            enc_handle_ptr->rate_control_tasks_producer_fifo_ptr_array[rate_control_port_lookup(RATE_CONTROL_INPUT_PORT_ENTROPY_CODING, process_index)],
            is16bit);
        if (return_error == EB_ErrorInsufficientResources) {
            return EB_ErrorInsufficientResources;
        }
    }

    // Packetization Context
    return_error = eb_vp9_packetization_context_ctor(
        (PacketizationContext**)&enc_handle_ptr->packetization_context_ptr,
        enc_handle_ptr->entropy_coding_results_consumer_fifo_ptr_array[0],
        enc_handle_ptr->rate_control_tasks_producer_fifo_ptr_array[rate_control_port_lookup(RATE_CONTROL_INPUT_PORT_PACKETIZATION, 0)]);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }
    /************************************
     * Thread Handles
     ************************************/
    EbSvtVp9EncConfiguration   *config_ptr = &scs_ptr->static_config;
    eb_set_thread_management_parameters(config_ptr);

    // Resource Coordination
    EB_CREATETHREAD(EbHandle, enc_handle_ptr->resource_coordination_thread_handle, sizeof(EbHandle), EB_THREAD, eb_vp9_resource_coordination_kernel, enc_handle_ptr->resource_coordination_context_ptr);

    // Picture Analysis
    EB_MALLOC(EbHandle*, enc_handle_ptr->picture_analysis_thread_handle_array, sizeof(EbHandle) * scs_ptr->picture_analysis_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->picture_analysis_process_init_count; ++process_index) {
        EB_CREATETHREAD(EbHandle, enc_handle_ptr->picture_analysis_thread_handle_array[process_index], sizeof(EbHandle), EB_THREAD, eb_vp9_picture_analysis_kernel, enc_handle_ptr->picture_analysis_context_ptr_array[process_index]);
    }

    // Picture Decision
    EB_CREATETHREAD(EbHandle, enc_handle_ptr->picture_decision_thread_handle, sizeof(EbHandle), EB_THREAD, eb_vp9_picture_decision_kernel, enc_handle_ptr->picture_decision_context_ptr);

    // Motion Estimation
    EB_MALLOC(EbHandle*, enc_handle_ptr->motion_estimation_thread_handle_array, sizeof(EbHandle) * scs_ptr->motion_estimation_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->motion_estimation_process_init_count; ++process_index) {
        EB_CREATETHREAD(EbHandle, enc_handle_ptr->motion_estimation_thread_handle_array[process_index], sizeof(EbHandle), EB_THREAD, eb_vp9_motion_estimation_kernel, enc_handle_ptr->motion_estimation_context_ptr_array[process_index]);
    }

    // Initial Rate Control
    EB_CREATETHREAD(EbHandle, enc_handle_ptr->initial_rate_control_thread_handle, sizeof(EbHandle), EB_THREAD, eb_vp9_initial_eb_vp9_rate_control_kernel, enc_handle_ptr->initial_rate_control_context_ptr);

    // Source Based Oprations
    EB_MALLOC(EbHandle*, enc_handle_ptr->source_based_operations_thread_handle_array, sizeof(EbHandle) * scs_ptr->source_based_operations_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->source_based_operations_process_init_count; ++process_index) {
        EB_CREATETHREAD(EbHandle, enc_handle_ptr->source_based_operations_thread_handle_array[process_index], sizeof(EbHandle), EB_THREAD, eb_vp9_source_based_operations_kernel, enc_handle_ptr->source_based_operations_context_ptr_array[process_index]);
    }

    // Picture Manager
    EB_CREATETHREAD(EbHandle, enc_handle_ptr->picture_manager_thread_handle, sizeof(EbHandle), EB_THREAD, eb_vp9_PictureManagerKernel, enc_handle_ptr->picture_manager_context_ptr);

    // Rate Control
    EB_CREATETHREAD(EbHandle, enc_handle_ptr->rate_control_thread_handle, sizeof(EbHandle), EB_THREAD, eb_vp9_rate_control_kernel, enc_handle_ptr->rate_control_context_ptr);

    // Mode Decision Configuration Process
    EB_MALLOC(EbHandle*, enc_handle_ptr->mode_decision_configuration_thread_handle_array, sizeof(EbHandle) * scs_ptr->mode_decision_configuration_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->mode_decision_configuration_process_init_count; ++process_index) {
        EB_CREATETHREAD(EbHandle, enc_handle_ptr->mode_decision_configuration_thread_handle_array[process_index], sizeof(EbHandle), EB_THREAD, eb_vp9_mode_decision_configuration_kernel, enc_handle_ptr->mode_decision_configuration_context_ptr_array[process_index]);
    }

    // EncDec Process
    EB_MALLOC(EbHandle*, enc_handle_ptr->enc_dec_thread_handle_array, sizeof(EbHandle) * scs_ptr->enc_dec_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->enc_dec_process_init_count; ++process_index) {
        EB_CREATETHREAD(EbHandle, enc_handle_ptr->enc_dec_thread_handle_array[process_index], sizeof(EbHandle), EB_THREAD, eb_vp9_enc_dec_kernel, enc_handle_ptr->enc_dec_context_ptr_array[process_index]);
    }

    // Entropy Coding Process
    EB_MALLOC(EbHandle*, enc_handle_ptr->entropy_coding_thread_handle_array, sizeof(EbHandle) * scs_ptr->entropy_coding_process_init_count, EB_N_PTR);

    for (process_index = 0; process_index < scs_ptr->entropy_coding_process_init_count; ++process_index) {
        EB_CREATETHREAD(EbHandle, enc_handle_ptr->entropy_coding_thread_handle_array[process_index], sizeof(EbHandle), EB_THREAD, eb_vp9_entropy_coding_kernel, enc_handle_ptr->entropy_coding_context_ptr_array[process_index]);
    }

    // Packetization
    EB_CREATETHREAD(EbHandle, enc_handle_ptr->packetization_thread_handle, sizeof(EbHandle), EB_THREAD, eb_vp9_packetization_kernel, enc_handle_ptr->packetization_context_ptr);

#if DISPLAY_MEMORY
    EB_MEMORY();
#endif
    return return_error;
}

/**********************************
 * DeInitialize Encoder Library
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EbErrorType  eb_vp9_deinit_encoder(EbComponentType *svt_enc_component)
{
    EbEncHandle *enc_handle_ptr = (EbEncHandle*)svt_enc_component->p_component_private;
    EbErrorType  return_error = EB_ErrorNone;
    int32_t              ptr_index = 0;
    EbMemoryMapEntry*   memory_entry = (EbMemoryMapEntry*)EB_NULL;
    if (enc_handle_ptr) {
        if (enc_handle_ptr->memory_map_index) {
            // Loop through the ptr table and free all malloc'd pointers per channel
            for (ptr_index = (enc_handle_ptr->memory_map_index) - 1; ptr_index >= 0; --ptr_index) {
                memory_entry = &enc_handle_ptr->memory_map[ptr_index];
                switch (memory_entry->ptr_type) {
                case EB_N_PTR:
                    free(memory_entry->ptr);
                    break;
                case EB_A_PTR:
#ifdef _WIN32
                    _aligned_free(memory_entry->ptr);
#else
                    free(memory_entry->ptr);
#endif
                    break;
                case EB_SEMAPHORE:
                    eb_vp9_destroy_semaphore(memory_entry->ptr);
                    break;
                case EB_THREAD:
                    eb_vp9_destroy_thread(memory_entry->ptr);
                    break;
                case EB_MUTEX:
                    eb_vp9_destroy_mutex(memory_entry->ptr);
                    break;
                default:
                    return_error = EB_ErrorMax;
                    break;
                }
            }
            if (enc_handle_ptr->memory_map != (EbMemoryMapEntry*)NULL) {
                free(enc_handle_ptr->memory_map);
            }

            //(void)(enc_handle_ptr);
        }
    }
    return return_error;
}

/**********************************
 * GetHandle
 **********************************/
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API void eb_vp9_svt_release_out_buffer(
    EbBufferHeaderType  **p_buffer)
{
    if (p_buffer && (*p_buffer)->wrapper_ptr)
        // Release out put buffer back into the pool
        eb_vp9_release_object((EbObjectWrapper  *)(*p_buffer)->wrapper_ptr);
    return;
}

/**********************************
Set Default Library Params
**********************************/
EbErrorType eb_vp9_svt_enc_init_parameter(
    EbSvtVp9EncConfiguration * config_ptr){

    EbErrorType                  return_error = EB_ErrorNone;

    if (!config_ptr) {
        SVT_LOG("SVT [ERROR]: The EbSvtVp9EncConfiguration structure is empty! \n");
        return EB_ErrorBadParameter;
    }

    config_ptr->frame_rate = 30 << 16;
    config_ptr->frame_rate_numerator = 0;
    config_ptr->frame_rate_denominator = 0;
    config_ptr->encoder_bit_depth = 8;
    config_ptr->source_width = 0;
    config_ptr->source_height = 0;

    config_ptr->qp = 50;
    config_ptr->use_qp_file = EB_FALSE;
    config_ptr->rate_control_mode = 0;
    config_ptr->target_bit_rate = 7000000;
    config_ptr->max_qp_allowed = MAX_QP_VALUE;
    config_ptr->min_qp_allowed = 0;
    config_ptr->base_layer_switch_mode = 0;
    config_ptr->enc_mode = 3;
    config_ptr->intra_period = 31;
    config_ptr->pred_structure = EB_PRED_RANDOM_ACCESS;
    config_ptr->loop_filter = EB_TRUE;
    config_ptr->use_default_me_hme = EB_TRUE;
    config_ptr->enable_hme_flag = EB_TRUE;
    config_ptr->search_area_width = 16;
    config_ptr->search_area_height = 7;

    // Bitstream options
    //config_ptr->codeVpsSpsPps = 0;
    //config_ptr->codeEosNal = 0;

    // Annex A parameters
    config_ptr->profile = 0;
    config_ptr->level = 0;

    // Latency
    config_ptr->injector_frame_rate = 60 << 16;
    config_ptr->speed_control_flag = 0;

    // ASM Type
    config_ptr->asm_type = 1;

    // Channel info
    //config_ptr->logicalProcessors = 0;
    //config_ptr->target_socket = -1;
    config_ptr->channel_id = 0;
    config_ptr->active_channel_count = 1;

    // Debug info
    config_ptr->recon_file = 0;

    return return_error;
}
/**********************************
* GetHandle
**********************************/
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_svt_init_handle(
    EbComponentType          **p_handle,      // Function to be called in the future for manipulating the component
    void                      *p_app_data,
    EbSvtVp9EncConfiguration  *config_ptr) {   // Pointer passed back to the client during callbacks

    EbErrorType           return_error = EB_ErrorNone;

    *p_handle = (EbComponentType*)malloc(sizeof(EbComponentType));

    if (*p_handle != (EbComponentType*)NULL) {

        // Init Component OS objects (threads, semaphores, etc.)
        // also links the various Component control functions
        return_error = init_svt_vp9_encoder_handle(*p_handle);

        if (return_error == EB_ErrorNone) {
            ((EbComponentType*)(*p_handle))->p_application_private = p_app_data;

        }
        else if (return_error == EB_ErrorInsufficientResources) {
            eb_vp9_deinit_encoder((EbComponentType*)NULL);
            *p_handle = (EbComponentType*)NULL;
        }
        else {
            return_error = EB_ErrorInvalidComponent;
        }
    }
    else {
        SVT_LOG("Error: Component Struct Malloc Failed\n");
        return_error = EB_ErrorInsufficientResources;
    }
    return_error = eb_vp9_svt_enc_init_parameter(config_ptr);

    return return_error;
}

/**********************************
* Encoder Componenet DeInit
**********************************/
static EbErrorType eb_svt_enc_component_de_init(
        EbComponentType  *svt_enc_component)
{
    EbErrorType       return_error = EB_ErrorNone;

    if (svt_enc_component->p_component_private) {
        free((EbEncHandle *)svt_enc_component->p_component_private);
    }
    else {
        return_error = EB_ErrorUndefined;
    }

    return return_error;
}

/**********************************
* eb_deinit_handle
**********************************/
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_deinit_handle(
    EbComponentType  *svt_enc_component)
{
    EbErrorType return_error = EB_ErrorNone;

    if (svt_enc_component) {
        return_error = eb_svt_enc_component_de_init(svt_enc_component);

        free(svt_enc_component);
    }
    else {
        return_error = EB_ErrorInvalidComponent;
    }

    return return_error;
}

uint32_t eb_vp9_set_parent_pcs(EbSvtVp9EncConfiguration*   config) {

    uint32_t fps = (uint32_t)((config->frame_rate > 1000) ? config->frame_rate >> 16 : config->frame_rate);

    fps = fps > 120 ? 120 : fps;
    fps = fps < 24 ? 24 : fps;

    return ((fps * 5) >> 2); // 1.25 sec worth of internal buffering
}

void eb_vp9_load_default_buffer_configuration_settings(
    SequenceControlSet         *sequence_control_set_ptr){

    uint32_t me_seg_h       = (((sequence_control_set_ptr->max_input_luma_height + 32) / MAX_SB_SIZE) < 6) ? 1 : 6;
    uint32_t me_seg_w       = (((sequence_control_set_ptr->max_input_luma_width + 32) / MAX_SB_SIZE) < 10) ? 1 : 10;

    uint32_t enc_dec_seg_h  = ((sequence_control_set_ptr->max_input_luma_height + 32) / MAX_SB_SIZE);
    uint32_t enc_dec_seg_w  = ((sequence_control_set_ptr->max_input_luma_width + 32) / MAX_SB_SIZE);
    uint32_t input_pic      = eb_vp9_set_parent_pcs(&sequence_control_set_ptr->static_config);

    unsigned int lp_count = get_num_cores();
    unsigned int core_count = lp_count;
#if defined(_WIN32) || defined(__linux__)
    if (sequence_control_set_ptr->static_config.target_socket != -1)
        core_count /= eb_vp9_num_groups;
    if (sequence_control_set_ptr->static_config.logical_processors != 0)
        core_count = sequence_control_set_ptr->static_config.logical_processors < core_count ?
        sequence_control_set_ptr->static_config.logical_processors : core_count;
#endif

#ifdef _WIN32
    //Handle special case on Windows
    //By default, on Windows an application is constrained to a single group
    if (sequence_control_set_ptr->static_config.target_socket == -1 &&
        sequence_control_set_ptr->static_config.logical_processors == 0)
        core_count /= eb_vp9_num_groups;

    //Affininty can only be set by group on Windows.
    //Run on both sockets if -lp is larger than logical processor per group.
    if (sequence_control_set_ptr->static_config.target_socket == -1 &&
        sequence_control_set_ptr->static_config.logical_processors > lp_count / eb_vp9_num_groups)
        core_count = lp_count;
#endif

    // ME segments
    sequence_control_set_ptr->me_segment_row_count_array[0] = me_seg_h;
    sequence_control_set_ptr->me_segment_row_count_array[1] = me_seg_h;
    sequence_control_set_ptr->me_segment_row_count_array[2] = me_seg_h;
    sequence_control_set_ptr->me_segment_row_count_array[3] = me_seg_h;
    sequence_control_set_ptr->me_segment_row_count_array[4] = me_seg_h;
    sequence_control_set_ptr->me_segment_row_count_array[5] = me_seg_h;

    sequence_control_set_ptr->me_segment_column_count_array[0] = me_seg_w;
    sequence_control_set_ptr->me_segment_column_count_array[1] = me_seg_w;
    sequence_control_set_ptr->me_segment_column_count_array[2] = me_seg_w;
    sequence_control_set_ptr->me_segment_column_count_array[3] = me_seg_w;
    sequence_control_set_ptr->me_segment_column_count_array[4] = me_seg_w;
    sequence_control_set_ptr->me_segment_column_count_array[5] = me_seg_w;
    // EncDec segments
    sequence_control_set_ptr->enc_dec_segment_row_count_array[0] = enc_dec_seg_h;
    sequence_control_set_ptr->enc_dec_segment_row_count_array[1] = enc_dec_seg_h;
    sequence_control_set_ptr->enc_dec_segment_row_count_array[2] = enc_dec_seg_h;
    sequence_control_set_ptr->enc_dec_segment_row_count_array[3] = enc_dec_seg_h;
    sequence_control_set_ptr->enc_dec_segment_row_count_array[4] = enc_dec_seg_h;
    sequence_control_set_ptr->enc_dec_segment_row_count_array[5] = enc_dec_seg_h;

    sequence_control_set_ptr->enc_dec_segment_col_count_array[0] = enc_dec_seg_w;
    sequence_control_set_ptr->enc_dec_segment_col_count_array[1] = enc_dec_seg_w;
    sequence_control_set_ptr->enc_dec_segment_col_count_array[2] = enc_dec_seg_w;
    sequence_control_set_ptr->enc_dec_segment_col_count_array[3] = enc_dec_seg_w;
    sequence_control_set_ptr->enc_dec_segment_col_count_array[4] = enc_dec_seg_w;
    sequence_control_set_ptr->enc_dec_segment_col_count_array[5] = enc_dec_seg_w;

    //#====================== Data Structures and Picture Buffers ======================
    sequence_control_set_ptr->input_output_buffer_fifo_init_count       = input_pic + SCD_LAD + sequence_control_set_ptr->look_ahead_distance;
    sequence_control_set_ptr->picture_control_set_pool_init_count       = sequence_control_set_ptr->input_output_buffer_fifo_init_count;
    sequence_control_set_ptr->picture_control_set_pool_init_count_child = MAX(MIN(2, core_count / 2), core_count / 6);
    sequence_control_set_ptr->reference_picture_buffer_init_count       = MAX((uint32_t)(input_pic >> 1),
        (uint32_t)((1 << sequence_control_set_ptr->hierarchical_levels) + 2)) +
        sequence_control_set_ptr->look_ahead_distance + SCD_LAD;
    sequence_control_set_ptr->pa_reference_picture_buffer_init_count    = MAX((uint32_t)(input_pic >> 1),
        (uint32_t)((1 << sequence_control_set_ptr->hierarchical_levels) + 2)) +
        sequence_control_set_ptr->look_ahead_distance + SCD_LAD;
    sequence_control_set_ptr->output_recon_buffer_fifo_init_count       = sequence_control_set_ptr->reference_picture_buffer_init_count;

    //#====================== Inter process Fifos ======================
    sequence_control_set_ptr->resource_coordination_fifo_init_count       = 300;
    sequence_control_set_ptr->picture_analysis_fifo_init_count            = 300;
    sequence_control_set_ptr->picture_decision_fifo_init_count            = 300;
    sequence_control_set_ptr->initial_rate_control_fifo_init_count        = 300;
    sequence_control_set_ptr->picture_demux_fifo_init_count               = 300;
    sequence_control_set_ptr->rate_control_tasks_fifo_init_count          = 300;
    sequence_control_set_ptr->rate_control_fifo_init_count                = 301;
    sequence_control_set_ptr->mode_decision_configuration_fifo_init_count = 300;
    sequence_control_set_ptr->motion_estimation_fifo_init_count           = 300;
    sequence_control_set_ptr->entropy_coding_fifo_init_count              = 300;
    sequence_control_set_ptr->enc_dec_fifo_init_count                     = 300;

    //#====================== Processes number ======================
    sequence_control_set_ptr->total_process_init_count = 0;
    sequence_control_set_ptr->total_process_init_count += (sequence_control_set_ptr->picture_analysis_process_init_count            = MAX(MIN(15, core_count), core_count / 6));
    sequence_control_set_ptr->total_process_init_count += (sequence_control_set_ptr->motion_estimation_process_init_count           = MAX(MIN(20, core_count), core_count / 3));
    sequence_control_set_ptr->total_process_init_count += (sequence_control_set_ptr->source_based_operations_process_init_count     = MAX(MIN(3, core_count), core_count / 12));
    sequence_control_set_ptr->total_process_init_count += (sequence_control_set_ptr->mode_decision_configuration_process_init_count = MAX(MIN(3, core_count), core_count / 12));
    sequence_control_set_ptr->total_process_init_count += (sequence_control_set_ptr->enc_dec_process_init_count                     = MAX(MIN(40, core_count), core_count));
    sequence_control_set_ptr->total_process_init_count += (sequence_control_set_ptr->entropy_coding_process_init_count              = MAX(MIN(3, core_count), core_count / 12));

    sequence_control_set_ptr->total_process_init_count += 6; // single processes count
    SVT_LOG("Number of logical cores available: %u\nNumber of PPCS %u\n", core_count, input_pic);

    return;

}

// Sets the default intra period the closest possible to 1 second without breaking the minigop
static int32_t compute_default_intra_period(
    SequenceControlSet       *sequence_control_set_ptr) {

    int32_t intra_period = 0;
    EbSvtVp9EncConfiguration   *config = &sequence_control_set_ptr->static_config;
    int32_t fps = config->frame_rate < 1000 ?
        config->frame_rate :
        config->frame_rate >> 16;
    int32_t mini_gop_size = (1 << (sequence_control_set_ptr->hierarchical_levels));
    int32_t min_ip = ((int)((fps) / mini_gop_size)*(mini_gop_size));
    int32_t max_ip = ((int)((fps + mini_gop_size) / mini_gop_size)*(mini_gop_size));

    intra_period = (ABS((fps - max_ip)) > ABS((fps - min_ip))) ? min_ip : max_ip;

    return intra_period;
}

static void set_default_configuration_parameters(
    SequenceControlSet         *sequence_control_set_ptr) {

    // No Cropping Window
    sequence_control_set_ptr->cropping_right_offset = 0;
    sequence_control_set_ptr->cropping_bottom_offset = 0;

    // Coding Structure
    sequence_control_set_ptr->enable_qp_scaling_flag = EB_TRUE;

    //Denoise
    sequence_control_set_ptr->enable_denoise_flag = EB_TRUE;

    return;
}

static uint32_t compute_default_look_ahead(
    EbSvtVp9EncConfiguration*   config,
    SequenceControlSet*         scs_ptr){

    int32_t lad = 0;
    if (config->rate_control_mode == 0)
        lad = (2 << scs_ptr->hierarchical_levels) + 1;
    else
        lad = config->intra_period;

    return lad;
}

static void copy_api_from_app(
    SequenceControlSet         *sequence_control_set_ptr,
    EbSvtVp9EncConfiguration     *p_component_parameter_structure) {

    sequence_control_set_ptr->max_input_luma_width = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->source_width;
    sequence_control_set_ptr->max_input_luma_height = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->source_height;
    sequence_control_set_ptr->frame_rate = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->frame_rate;
    sequence_control_set_ptr->chroma_width = sequence_control_set_ptr->max_input_luma_width >> 1;
    sequence_control_set_ptr->chroma_height = sequence_control_set_ptr->max_input_luma_height >> 1;

    sequence_control_set_ptr->video_usability_info_ptr->field_seq_flag = EB_FALSE;
    sequence_control_set_ptr->video_usability_info_ptr->frame_field_info_present_flag = EB_FALSE;

    // Coding Structure
    sequence_control_set_ptr->static_config.intra_period = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->intra_period;
    sequence_control_set_ptr->static_config.pred_structure = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->pred_structure;
    sequence_control_set_ptr->static_config.base_layer_switch_mode = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->base_layer_switch_mode;
    sequence_control_set_ptr->static_config.tune = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->tune;
    sequence_control_set_ptr->static_config.enc_mode = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->enc_mode;

    sequence_control_set_ptr->intra_period = sequence_control_set_ptr->static_config.intra_period;
    sequence_control_set_ptr->max_ref_count = 1;

    // Quantization
    sequence_control_set_ptr->qp = sequence_control_set_ptr->static_config.qp = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->qp;
    sequence_control_set_ptr->static_config.use_qp_file = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->use_qp_file;

    // Loop Filter
    sequence_control_set_ptr->static_config.loop_filter = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->loop_filter;

    // ME Tools
    sequence_control_set_ptr->static_config.use_default_me_hme = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->use_default_me_hme;

    // Default HME/ME settings
    sequence_control_set_ptr->static_config.enable_hme_flag = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->enable_hme_flag;
    sequence_control_set_ptr->static_config.search_area_width = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->search_area_width;
    sequence_control_set_ptr->static_config.search_area_height = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->search_area_height;

    sequence_control_set_ptr->static_config.recon_file = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->recon_file;
    sequence_control_set_ptr->encode_context_ptr->recon_port_active = (EB_BOOL)sequence_control_set_ptr->static_config.recon_file;

    // Rate Control
    sequence_control_set_ptr->static_config.rate_control_mode = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->rate_control_mode;
    sequence_control_set_ptr->static_config.vbv_max_rate = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->vbv_max_rate;
    sequence_control_set_ptr->static_config.vbv_buf_size = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->vbv_buf_size;
    sequence_control_set_ptr->static_config.frames_to_be_encoded = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->frames_to_be_encoded;
    sequence_control_set_ptr->frame_rate = sequence_control_set_ptr->static_config.frame_rate = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->frame_rate;
    sequence_control_set_ptr->static_config.target_bit_rate = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->target_bit_rate;
    sequence_control_set_ptr->encode_context_ptr->available_target_bitrate = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->target_bit_rate;
    sequence_control_set_ptr->static_config.max_qp_allowed = (sequence_control_set_ptr->static_config.rate_control_mode) ?
        ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->max_qp_allowed :
#if VP9_RC
        MAX_QP_VALUE;
#else
        51;
#endif

    sequence_control_set_ptr->static_config.min_qp_allowed = (sequence_control_set_ptr->static_config.rate_control_mode) ?
        ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->min_qp_allowed :
        0;

    // Misc
    sequence_control_set_ptr->static_config.encoder_bit_depth = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->encoder_bit_depth;

    // Annex A parameters
    sequence_control_set_ptr->static_config.profile = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->profile;
    sequence_control_set_ptr->static_config.level = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->level;

    sequence_control_set_ptr->static_config.injector_frame_rate = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->injector_frame_rate;
    sequence_control_set_ptr->static_config.speed_control_flag = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->speed_control_flag;

    sequence_control_set_ptr->static_config.asm_type = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->asm_type;

    sequence_control_set_ptr->static_config.channel_id = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->channel_id;
    sequence_control_set_ptr->static_config.active_channel_count = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->active_channel_count;

    sequence_control_set_ptr->static_config.logical_processors = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->logical_processors;
    sequence_control_set_ptr->static_config.target_socket = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->target_socket;

    sequence_control_set_ptr->static_config.frame_rate_denominator = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->frame_rate_denominator;
    sequence_control_set_ptr->static_config.frame_rate_numerator = ((EbSvtVp9EncConfiguration*)p_component_parameter_structure)->frame_rate_numerator;

    // Set default hierarchical-levels
    if (sequence_control_set_ptr->static_config.tune != 0 && sequence_control_set_ptr->static_config.rate_control_mode == 0) {
        sequence_control_set_ptr->hierarchical_levels = 4;
        sequence_control_set_ptr->max_temporal_layers = 4;
    }
    else {
        sequence_control_set_ptr->hierarchical_levels = 3;
        sequence_control_set_ptr->max_temporal_layers = 3;
    }

    // Extract frame rate from Numerator and Denominator if not 0
    if (sequence_control_set_ptr->static_config.frame_rate_numerator != 0 && sequence_control_set_ptr->static_config.frame_rate_denominator != 0) {
        sequence_control_set_ptr->frame_rate = sequence_control_set_ptr->static_config.frame_rate = (((sequence_control_set_ptr->static_config.frame_rate_numerator << 8) / (sequence_control_set_ptr->static_config.frame_rate_denominator)) << 8);
    }

    // Get Default Intra Period if not specified
    if (sequence_control_set_ptr->static_config.intra_period == -2) {
        sequence_control_set_ptr->intra_period = (sequence_control_set_ptr->static_config.intra_period = compute_default_intra_period(sequence_control_set_ptr));
    }

    sequence_control_set_ptr->look_ahead_distance = compute_default_look_ahead(&sequence_control_set_ptr->static_config, sequence_control_set_ptr);

    return;
}

/******************************************
* Verify Settings
******************************************/
#define PowerOfTwoCheck(x) (((x) != 0) && (((x) & (~(x) + 1)) == (x)))

static EbErrorType  verify_settings(
    SequenceControlSet *sequence_control_set_ptr)
{
    EbErrorType   return_error = EB_ErrorNone;
    const char   *level_idc;
    unsigned int  level_idx;
    EbSvtVp9EncConfiguration *config = &sequence_control_set_ptr->static_config;
    unsigned int channel_number = config->channel_id;

    switch (config->level) {
    case 10:
        level_idc = "1";
        level_idx = 0;

        break;
    case 20:
        level_idc = "2";
        level_idx = 1;

        break;
    case 21:
        level_idc = "2.1";
        level_idx = 2;

        break;
    case 30:
        level_idc = "3";
        level_idx = 3;

        break;
    case 31:
        level_idc = "3.1";
        level_idx = 4;

        break;
    case 40:
        level_idc = "4";
        level_idx = 5;

        break;
    case 41:
        level_idc = "4.1";
        level_idx = 6;

        break;
    case 50:
        level_idc = "5";
        level_idx = 7;

        break;
    case 51:
        level_idc = "5.1";
        level_idx = 8;

        break;
    case 52:
        level_idc = "5.2";
        level_idx = 9;

        break;
    case 60:
        level_idc = "6";
        level_idx = 10;

        break;
    case 61:
        level_idc = "6.1";
        level_idx = 11;

        break;
    case 62:
        level_idc = "6.2";
        level_idx = 12;

        break;

    case 0: // Level determined by the encoder
        level_idc = "0";
        level_idx = TOTAL_LEVEL_COUNT;

        break;

    default:
        level_idc = "unknown";
        level_idx = TOTAL_LEVEL_COUNT + 1;

        break;
    }

    if (level_idx > TOTAL_LEVEL_COUNT) {
        SVT_LOG("Error Instance %u: Unsupported level\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (sequence_control_set_ptr->max_input_luma_width < 64) {
        SVT_LOG("Error instance %u: Source Width must be at least 64\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (sequence_control_set_ptr->max_input_luma_height < 64) {
        SVT_LOG("Error instance %u: Source Height must be at least 64\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->pred_structure != 2) {
        SVT_LOG("Error instance %u: Pred Structure must be [2]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->base_layer_switch_mode == 1 && config->pred_structure != 2) {
        SVT_LOG("Error Instance %u: Base Layer Switch Mode 1 only when Prediction Structure is Random Access\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (sequence_control_set_ptr->max_input_luma_width % 2) {
        SVT_LOG("Error Instance %u: Source Width must be even for YUV_420 colorspace \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    else if (sequence_control_set_ptr->max_input_luma_height % 2) {
        SVT_LOG("Error Instance %u: Source Height must be even for YUV_420 colorspace\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (sequence_control_set_ptr->max_input_luma_width > 8192) {
        SVT_LOG("Error instance %u: Source Width must be less than 8192\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (sequence_control_set_ptr->max_input_luma_width % 8 != 0) {
        SVT_LOG("Error instance %u: Source Width must be a multiple of 8\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (sequence_control_set_ptr->max_input_luma_height > 4320) {
        SVT_LOG("Error instance %u: Source Height must be less than 4320\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (sequence_control_set_ptr->max_input_luma_height % 8 != 0) {
        SVT_LOG("Error instance %u: Source Height must be a multiple of 8\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    uint32_t input_size = sequence_control_set_ptr->max_input_luma_width * sequence_control_set_ptr->max_input_luma_height;

    uint8_t input_resolution = (input_size < INPUT_SIZE_1080i_TH) ? INPUT_SIZE_576p_RANGE_OR_LOWER :
        (input_size < INPUT_SIZE_1080p_TH) ? INPUT_SIZE_1080i_RANGE :
        (input_size < INPUT_SIZE_4K_TH) ? INPUT_SIZE_1080p_RANGE :
        INPUT_SIZE_4K_RANGE;

    if (input_resolution <= INPUT_SIZE_1080i_RANGE) {
        if (config->enc_mode > 9) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - 9] for this resolution\n", channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }

    }
    else if (input_resolution == INPUT_SIZE_1080p_RANGE) {
        if (config->enc_mode > 10) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - 10] for this resolution\n", channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }

    }
    else {
        if (config->enc_mode > 12 && config->tune == 0) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - 12] for this resolution\n", channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }
        else if (config->enc_mode > 10 && config->tune >= 1) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - 10] for this resolution\n", channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }
    }

    // enc_mode
    sequence_control_set_ptr->max_enc_mode = MAX_SUPPORTED_MODES;
    if (input_resolution <= INPUT_SIZE_1080i_RANGE) {
        sequence_control_set_ptr->max_enc_mode = MAX_SUPPORTED_MODES_SUB1080P - 1;
        if (config->enc_mode > MAX_SUPPORTED_MODES_SUB1080P - 1) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - %d]\n", channel_number + 1, MAX_SUPPORTED_MODES_SUB1080P - 1);
            return_error = EB_ErrorBadParameter;
        }
    }
    else if (input_resolution == INPUT_SIZE_1080p_RANGE) {
        sequence_control_set_ptr->max_enc_mode = MAX_SUPPORTED_MODES_1080P - 1;
        if (config->enc_mode > MAX_SUPPORTED_MODES_1080P - 1) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - %d]\n", channel_number + 1, MAX_SUPPORTED_MODES_1080P - 1);
            return_error = EB_ErrorBadParameter;
        }
    }
    else {
        if (config->tune == 0)
            sequence_control_set_ptr->max_enc_mode = MAX_SUPPORTED_MODES_4K_SQ - 1;
        else
            sequence_control_set_ptr->max_enc_mode = MAX_SUPPORTED_MODES_4K_OQ - 1;

        if (config->enc_mode > MAX_SUPPORTED_MODES_4K_SQ - 1 && config->tune == 0) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - %d]\n", channel_number + 1, MAX_SUPPORTED_MODES_4K_SQ - 1);
            return_error = EB_ErrorBadParameter;
        }
        else if (config->enc_mode > MAX_SUPPORTED_MODES_4K_OQ - 1 && config->tune >= 1) {
            SVT_LOG("Error instance %u: enc_mode must be [0 - %d]\n", channel_number + 1, MAX_SUPPORTED_MODES_4K_OQ - 1);
            return_error = EB_ErrorBadParameter;
        }
    }

    if (config->qp > MAX_QP_VALUE) {
        SVT_LOG("Error instance %u: QP must be [0 - %d]\n", channel_number + 1, MAX_QP_VALUE);
        return_error = EB_ErrorBadParameter;
    }

    if (config->intra_period < -2 || config->intra_period > 255) {
        SVT_LOG("Error Instance %u: The intra period must be [-2 - 255] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->base_layer_switch_mode > 1) {
        SVT_LOG("Error Instance %u: Invalid Base Layer Switch Mode [0-1] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->loop_filter > 1) {
        SVT_LOG("Error Instance %u: Invalid LoopFilterDisable. LoopFilterDisable must be [0 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->use_default_me_hme > 1) {
        SVT_LOG("Error Instance %u: invalid use_default_me_hme. use_default_me_hme must be [0 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->enable_hme_flag > 1) {
        SVT_LOG("Error Instance %u: invalid HME. HME must be [0 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if ((config->search_area_width > 256) || (config->search_area_width == 0)) {
        SVT_LOG("Error Instance %u: Invalid search_area_width. search_area_width must be [1 - 256]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;

    }

    if ((config->search_area_height > 256) || (config->search_area_height == 0)) {
        SVT_LOG("Error Instance %u: Invalid search_area_height. search_area_height must be [1 - 256]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;

    }

    if (level_idx < 13) {
        // Check if the current input video is conformant with the Level constraint
        if (config->level != 0 && ((uint64_t)sequence_control_set_ptr->max_input_luma_width * (uint64_t)sequence_control_set_ptr->max_input_luma_height > max_luma_picture_size[level_idx])) {
            SVT_LOG("Error Instance %u: The input luma picture size exceeds the maximum luma picture size allowed for level %s\n", channel_number + 1, level_idc);
            return_error = EB_ErrorBadParameter;
        }

        // Check if the current input video is conformant with the Level constraint
        if (config->level != 0 && ((uint64_t)config->frame_rate * (uint64_t)sequence_control_set_ptr->max_input_luma_width * (uint64_t)sequence_control_set_ptr->max_input_luma_height > (max_luma_sample_rate[level_idx] << 16))) {
            SVT_LOG("Error Instance %u: The input luma sample rate exceeds the maximum input sample rate allowed for level %s\n", channel_number + 1, level_idc);
            return_error = EB_ErrorBadParameter;
        }

    }

    // Check if the current input video is conformant with the Level constraint
    if (config->frame_rate > (240 << 16)) {
        SVT_LOG("Error Instance %u: The maximum allowed frame rate is 240 fps\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the frame_rate is non-zero
    if (config->frame_rate <= 0) {
        SVT_LOG("Error Instance %u: The frame rate should be greater than 0 fps \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->intra_period < -2 || config->intra_period > 255) {
        SVT_LOG("Error Instance %u: The intra period must be [-2 - 255] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->rate_control_mode > 2) {
        SVT_LOG("Error Instance %u: The rate control mode must be [0 - 2] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
#if !VP9_RC
    if (config->rate_control_mode == 1 && config->tune > 0) {
        SVT_LOG("Error Instance %u: The rate control is not supported for OQ mode (Tune = 1 ) and VMAF mode (Tune = 2)\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
#endif

#if VP9_RC
    if (config->max_qp_allowed > 63) {
        SVT_LOG("Error instance %u: max_qp_allowed must be [0 - 63]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    else if (config->min_qp_allowed > 62) {
        SVT_LOG("Error instance %u: min_qp_allowed must be [0 - 62]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
#else
    if (config->max_qp_allowed > 51) {
        SVT_LOG("Error instance %u: max_qp_allowed must be [0 - 51]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    else if (config->min_qp_allowed > 50) {
        SVT_LOG("Error instance %u: min_qp_allowed must be [0 - 50]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
#endif
    else if ((config->min_qp_allowed) > (config->max_qp_allowed)) {
        SVT_LOG("Error Instance %u:  min_qp_allowed must be smaller than max_qp_allowed\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 2) {
        SVT_LOG("Error instance %u : Invalid Tune. Tune must be [0 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if ((config->encoder_bit_depth != 8)) {
        SVT_LOG("Error instance %u: Only 8 bit is supported in this build \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    // Check if the encoder_bit_depth is conformant with the Profile constraint
    if (config->profile != 0) {
        SVT_LOG("Error instance %u: Only 420 8 bit is supported in this build\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->speed_control_flag > 1) {
        SVT_LOG("Error Instance %u: Invalid Speed Control flag [0 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (((int32_t)(config->asm_type) < 0) || ((int32_t)(config->asm_type) > 1)) {
        SVT_LOG("Error Instance %u: Invalid asm type value [0: C Only, 1: Auto] .\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->target_socket != -1 && config->target_socket != 0 && config->target_socket != 1) {
        SVT_LOG("Error instance %u: Invalid target_socket. target_socket must be [-1 - 1] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    return return_error;
}

/**********************************
* Set Parameter
**********************************/
static void set_param_based_on_input(
    SequenceControlSet       *sequence_control_set_ptr) {

    // Update picture width, and picture height
    if (sequence_control_set_ptr->max_input_luma_width % MIN_CU_SIZE) {

        sequence_control_set_ptr->max_input_pad_right = MIN_CU_SIZE - (sequence_control_set_ptr->max_input_luma_width % MIN_CU_SIZE);
        sequence_control_set_ptr->max_input_luma_width = sequence_control_set_ptr->max_input_luma_width + sequence_control_set_ptr->max_input_pad_right;
    }
    else {

        sequence_control_set_ptr->max_input_pad_right = 0;
    }
    if (sequence_control_set_ptr->max_input_luma_height % MIN_CU_SIZE) {

        sequence_control_set_ptr->max_input_pad_bottom = MIN_CU_SIZE - (sequence_control_set_ptr->max_input_luma_height % MIN_CU_SIZE);
        sequence_control_set_ptr->max_input_luma_height = sequence_control_set_ptr->max_input_luma_height + sequence_control_set_ptr->max_input_pad_bottom;
    }
    else {
        sequence_control_set_ptr->max_input_pad_bottom = 0;
    }

    sequence_control_set_ptr->max_input_chroma_width = sequence_control_set_ptr->max_input_luma_width >> 1;
    sequence_control_set_ptr->max_input_chroma_height = sequence_control_set_ptr->max_input_luma_height >> 1;

    // Configure the padding
    sequence_control_set_ptr->left_padding = MAX_CU_SIZE + 4;
    sequence_control_set_ptr->top_padding = MAX_CU_SIZE + 4;
    sequence_control_set_ptr->right_padding = MAX_CU_SIZE + 4;
    sequence_control_set_ptr->bot_padding = MAX_CU_SIZE + 4;

    sequence_control_set_ptr->chroma_width = sequence_control_set_ptr->max_input_luma_width >> 1;
    sequence_control_set_ptr->chroma_height = sequence_control_set_ptr->max_input_luma_height >> 1;
    sequence_control_set_ptr->luma_width = sequence_control_set_ptr->max_input_luma_width;
    sequence_control_set_ptr->luma_height = sequence_control_set_ptr->max_input_luma_height;
    sequence_control_set_ptr->static_config.source_width = sequence_control_set_ptr->max_input_luma_width;
    sequence_control_set_ptr->static_config.source_height = sequence_control_set_ptr->max_input_luma_height;

    eb_vp9_derive_input_resolution(
        sequence_control_set_ptr,
        sequence_control_set_ptr->luma_width*sequence_control_set_ptr->luma_height);

}
static void print_lib_params(
    SequenceControlSet* scs) {

    EbSvtVp9EncConfiguration*   config = &scs->static_config;

    SVT_LOG("------------------------------------------- ");
    SVT_LOG("\nSVT [config]: Profile [0]\t");

    if (config->level != 0)
        SVT_LOG("Level %.1f\t", (float)(config->level / 10));
    else {
        SVT_LOG("Level (auto)\t");
    }
    SVT_LOG("\nSVT [config]: EncoderMode / Tune \t\t\t\t\t\t: %d / %d ", config->enc_mode, config->tune);
    SVT_LOG("\nSVT [config]: EncoderBitDepth \t\t\t\t\t\t\t: %d ", config->encoder_bit_depth);
    SVT_LOG("\nSVT [config]: SourceWidth / SourceHeight\t\t\t\t\t: %d / %d ", config->source_width, config->source_height);
    if (config->frame_rate_denominator != 0 && config->frame_rate_numerator != 0)
        SVT_LOG("\nSVT [config]: Fps_Numerator / Fps_Denominator / Gop Size\t\t: %d / %d / %d", config->frame_rate_numerator > (1 << 16) ? config->frame_rate_numerator >> 16 : config->frame_rate_numerator,
            config->frame_rate_denominator > (1 << 16) ? config->frame_rate_denominator >> 16 : config->frame_rate_denominator,
            config->intra_period + 1);
    else
        SVT_LOG("\nSVT [config]: FrameRate / Gop Size\t\t\t\t\t\t: %d / %d ", config->frame_rate > 1000 ? config->frame_rate >> 16 : config->frame_rate, config->intra_period + 1);
    SVT_LOG("\nSVT [config]: HierarchicalLevels / BaseLayerSwitchMode / PredStructure\t\t: %d / %d / %d ", scs->hierarchical_levels, config->base_layer_switch_mode, config->pred_structure);
    if (config->rate_control_mode == 2)
        SVT_LOG("\nSVT [config]: RCMode / TargetBitrate\t\t\t\t\t\t: CBR / %d ", config->target_bit_rate);
    else if (config->rate_control_mode == 1)
        SVT_LOG("\nSVT [config]: RCMode / TargetBitrate\t\t\t\t\t\t: VBR / %d ", config->target_bit_rate);
    else
        SVT_LOG("\nSVT [config]: BRC Mode / QP \t\t\t\t\t\t\t: CQP / %d ", scs->qp);
#ifdef DEBUG_BUFFERS
    SVT_LOG("\nSVT [config]: INPUT / OUTPUT \t\t\t\t\t\t\t: %d / %d", scs->input_buffer_fifo_init_count, scs->output_stream_buffer_fifo_init_count);
    SVT_LOG("\nSVT [config]: CPCS / PAREF / REF \t\t\t\t\t\t: %d / %d / %d", scs->picture_control_set_pool_init_count_child, scs->pa_reference_picture_buffer_init_count, scs->reference_picture_buffer_init_count);
    SVT_LOG("\nSVT [config]: ME_SEG_W0 / ME_SEG_W1 / ME_SEG_W2 / ME_SEG_W3 \t\t\t: %d / %d / %d / %d ",
        scs->me_segment_column_count_array[0],
        scs->me_segment_column_count_array[1],
        scs->me_segment_column_count_array[2],
        scs->me_segment_column_count_array[3]);
    SVT_LOG("\nSVT [config]: ME_SEG_H0 / ME_SEG_H1 / ME_SEG_H2 / ME_SEG_H3 \t\t\t: %d / %d / %d / %d ",
        scs->me_segment_row_count_array[0],
        scs->me_segment_row_count_array[1],
        scs->me_segment_row_count_array[2],
        scs->me_segment_row_count_array[3]);
    SVT_LOG("\nSVT [config]: ME_SEG_W0 / ME_SEG_W1 / ME_SEG_W2 / ME_SEG_W3 \t\t\t: %d / %d / %d / %d ",
        scs->enc_dec_segment_col_count_array[0],
        scs->enc_dec_segment_col_count_array[1],
        scs->enc_dec_segment_col_count_array[2],
        scs->enc_dec_segment_col_count_array[3]);
    SVT_LOG("\nSVT [config]: ME_SEG_H0 / ME_SEG_H1 / ME_SEG_H2 / ME_SEG_H3 \t\t\t: %d / %d / %d / %d ",
        scs->enc_dec_segment_row_count_array[0],
        scs->enc_dec_segment_row_count_array[1],
        scs->enc_dec_segment_row_count_array[2],
        scs->enc_dec_segment_row_count_array[3]);
    SVT_LOG("\nSVT [config]: PA_P / ME_P / SBO_P / MDC_P / ED_P / EC_P \t\t\t: %d / %d / %d / %d / %d / %d ",
        scs->picture_analysis_process_init_count,
        scs->motion_estimation_process_init_count,
        scs->source_based_operations_process_init_count,
        scs->mode_decision_configuration_process_init_count,
        scs->enc_dec_process_init_count,
        scs->entropy_coding_process_init_count);
#endif
    SVT_LOG("\n------------------------------------------- ");
    SVT_LOG("\n");

    fflush(stdout);
}

/**********************************
 * Set Parameter
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EbErrorType  eb_vp9_svt_enc_set_parameter(
    EbComponentType             *svt_enc_component,
    EbSvtVp9EncConfiguration    *p_component_parameter_structure){

    EbErrorType            return_error;
    EbEncHandle           *p_enc_comp_data;
    SequenceControlSet    *scs_ptr;

    if (svt_enc_component == (EbComponentType*)EB_NULL) {
        return EB_ErrorBadParameter;
    }

    return_error        = EB_ErrorNone;
    p_enc_comp_data     = (EbEncHandle*)svt_enc_component->p_component_private;
    scs_ptr             = p_enc_comp_data->sequence_control_set_instance_array[0]->sequence_control_set_ptr;

    // Acquire Config Mutex
    eb_vp9_block_on_mutex(p_enc_comp_data->sequence_control_set_instance_array[0]->config_mutex);

    set_default_configuration_parameters(scs_ptr);

    copy_api_from_app(
        scs_ptr,
        (EbSvtVp9EncConfiguration*)p_component_parameter_structure);

    return_error = (EbErrorType)verify_settings(scs_ptr);

    if (return_error == EB_ErrorBadParameter) {
        return EB_ErrorBadParameter;
    }

    set_param_based_on_input(scs_ptr);

    // Initialize the Prediction Structure Group
    return_error = (EbErrorType)eb_vp9_prediction_structure_group_ctor(
        &p_enc_comp_data->sequence_control_set_instance_array[0]->encode_context_ptr->prediction_structure_group_ptr,
        scs_ptr->static_config.base_layer_switch_mode);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // Set the Prediction Structure
    scs_ptr->pred_struct_ptr = eb_vp9_get_prediction_structure(
        p_enc_comp_data->sequence_control_set_instance_array[0]->encode_context_ptr->prediction_structure_group_ptr,
        scs_ptr->static_config.pred_structure,
        scs_ptr->max_ref_count,
        scs_ptr->max_temporal_layers);

    eb_vp9_load_default_buffer_configuration_settings(scs_ptr);

    print_lib_params(scs_ptr);

    // Release Config Mutex
    eb_vp9_release_mutex(p_enc_comp_data->sequence_control_set_instance_array[0]->config_mutex);

    return return_error;
}

/***********************************************
**** Copy the input buffer from the
**** sample application to the library buffers
************************************************/
static EbErrorType copy_frame_buffer(
    SequenceControlSet               *sequence_control_set_ptr,
    uint8_t                          *dst,
    uint8_t                          *src){

    EbSvtVp9EncConfiguration        *config = &sequence_control_set_ptr->static_config;
    EbErrorType                      return_error = EB_ErrorNone;

    EbPictureBufferDesc             *input_picture_ptr = (EbPictureBufferDesc*)dst;
    EbSvtEncInput                   *input_ptr = (EbSvtEncInput*)src;
    uint16_t                         input_row_index;
    EbBool                           is16_bit_input = (EbBool)(config->encoder_bit_depth > EB_8BIT);

    // Need to include for Interlacing on the fly with pictureScanType = 1

    if (!is16_bit_input) {

        uint32_t     luma_buffer_offset = (input_picture_ptr->stride_y*sequence_control_set_ptr->top_padding + sequence_control_set_ptr->left_padding) << is16_bit_input;
        uint32_t     chroma_buffer_offset = (input_picture_ptr->stride_cr*(sequence_control_set_ptr->top_padding >> 1) + (sequence_control_set_ptr->left_padding >> 1)) << is16_bit_input;
        uint16_t     luma_stride = input_picture_ptr->stride_y << is16_bit_input;
        uint16_t     chroma_stride = input_picture_ptr->stride_cb << is16_bit_input;
        uint16_t     luma_width = (uint16_t)(input_picture_ptr->width - sequence_control_set_ptr->max_input_pad_right) << is16_bit_input;
        uint16_t     chroma_width = (luma_width >> 1) << is16_bit_input;
        uint16_t     luma_height = (uint16_t)(input_picture_ptr->height - sequence_control_set_ptr->max_input_pad_bottom);

        uint16_t     source_luma_stride = (uint16_t)(input_ptr->y_stride);
        uint16_t     source_cr_stride = (uint16_t)(input_ptr->cr_stride);
        uint16_t     source_cb_stride = (uint16_t)(input_ptr->cb_stride);

        //uint16_t     luma_height  = input_picture_ptr->maxHeight;
        // Y
        for (input_row_index = 0; input_row_index < luma_height; input_row_index++) {

            EB_MEMCPY((input_picture_ptr->buffer_y + luma_buffer_offset + luma_stride * input_row_index),
                (input_ptr->luma + source_luma_stride * input_row_index),
                luma_width);
        }

        // U
        for (input_row_index = 0; input_row_index < luma_height >> 1; input_row_index++) {
            EB_MEMCPY((input_picture_ptr->buffer_cb + chroma_buffer_offset + chroma_stride * input_row_index),
                (input_ptr->cb + (source_cb_stride*input_row_index)),
                chroma_width);
        }

        // V
        for (input_row_index = 0; input_row_index < luma_height >> 1; input_row_index++) {
            EB_MEMCPY((input_picture_ptr->buffer_cr + chroma_buffer_offset + chroma_stride * input_row_index),
                (input_ptr->cr + (source_cr_stride*input_row_index)),
                chroma_width);
        }

    }

    return return_error;
}

static void copy_input_buffer(
    SequenceControlSet*     sequenceControlSet,
    EbBufferHeaderType*     dst,
    EbBufferHeaderType*     src){

    // Copy the higher level structure
    dst->n_alloc_len = src->n_alloc_len;
    dst->n_filled_len = src->n_filled_len;
    dst->flags = src->flags;
    dst->pts = src->pts;
    dst->n_tick_count = src->n_tick_count;
    dst->size = src->size;
    dst->qp = src->qp;
    dst->pic_type = src->pic_type;

    // Copy the picture buffer
    if (src->p_buffer != NULL)
        copy_frame_buffer(sequenceControlSet, dst->p_buffer, src->p_buffer);
}

static void copy_output_recon_buffer(
    EbBufferHeaderType   *dst,
    EbBufferHeaderType   *src){

    // copy output bitstream fileds
    dst->size = src->size;
    dst->n_alloc_len = src->n_alloc_len;
    dst->n_filled_len = src->n_filled_len;
    dst->p_app_private = src->p_app_private;
    dst->n_tick_count = src->n_tick_count;
    dst->pts = src->pts;
    dst->dts = src->dts;
    dst->flags = src->flags;
    dst->pic_type = src->pic_type;
    if (src->p_buffer)
        EB_MEMCPY(dst->p_buffer, src->p_buffer, src->n_filled_len);

    return;
}
/**********************************
* Fill This Buffer
**********************************/
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_svt_get_recon(
    EbComponentType      *svt_enc_component,
    EbBufferHeaderType   *p_buffer){

    EbErrorType           return_error    = EB_ErrorNone;
    EbEncHandle          *p_enc_comp_data = (EbEncHandle*)svt_enc_component->p_component_private;
    EbObjectWrapper      *eb_wrapper_ptr  = NULL;
    SequenceControlSet   *scs_ptr         = p_enc_comp_data->sequence_control_set_instance_array[0]->sequence_control_set_ptr;

    if (scs_ptr->static_config.recon_file) {

        eb_vp9_get_full_object_non_blocking(
            (p_enc_comp_data->output_recon_buffer_consumer_fifo_ptr_dbl_array[0])[0],
            &eb_wrapper_ptr);

        if (eb_wrapper_ptr) {
            EbBufferHeaderType* obj_ptr = (EbBufferHeaderType*)eb_wrapper_ptr->object_ptr;
            copy_output_recon_buffer(
                p_buffer,
                obj_ptr);

            if (p_buffer->flags != EB_BUFFERFLAG_EOS && p_buffer->flags != 0)
                return_error = EB_ErrorMax;

            eb_vp9_release_object((EbObjectWrapper  *)eb_wrapper_ptr);
        }
        else {
            return_error = EB_NoErrorEmptyQueue;
        }
    }
    else {
        // recon is not enabled
        return_error = EB_ErrorMax;
    }

    return return_error;
}

/**********************************
* Empty This Buffer
**********************************/
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_svt_enc_send_picture(
    EbComponentType      *svt_enc_component,
    EbBufferHeaderType   *p_buffer)
{
    EbEncHandle          *enc_handle_ptr = (EbEncHandle*)svt_enc_component->p_component_private;
    EbObjectWrapper      *eb_wrapper_ptr;

    // Take the buffer and put it into our internal queue structure
    eb_vp9_get_empty_object(
        enc_handle_ptr->input_buffer_producer_fifo_ptr_array[0],
        &eb_wrapper_ptr);

    if (p_buffer != NULL) {
        copy_input_buffer(
            enc_handle_ptr->sequence_control_set_instance_array[0]->sequence_control_set_ptr,
            (EbBufferHeaderType*)eb_wrapper_ptr->object_ptr,
            p_buffer);
    }

    eb_vp9_post_full_object(eb_wrapper_ptr);

    return EB_ErrorNone;
}

/**********************************
* eb_svt_get_packet sends out packet
**********************************/
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_svt_get_packet(
    EbComponentType      *svt_enc_component,
    EbBufferHeaderType  **p_buffer,
    unsigned char         pic_send_done){

    EbErrorType             return_error = EB_ErrorNone;
    EbEncHandle            *p_enc_comp_data = (EbEncHandle*)svt_enc_component->p_component_private;
    EbObjectWrapper        *eb_wrapper_ptr = NULL;
    EbBufferHeaderType     *packet;
    if (pic_send_done)
        eb_vp9_get_full_object(
        (p_enc_comp_data->output_stream_buffer_consumer_fifo_ptr_dbl_array[0])[0],
            &eb_wrapper_ptr);
    else
        eb_vp9_get_full_object_non_blocking(
        (p_enc_comp_data->output_stream_buffer_consumer_fifo_ptr_dbl_array[0])[0],
            &eb_wrapper_ptr);

    if (eb_wrapper_ptr) {

        packet = (EbBufferHeaderType*)eb_wrapper_ptr->object_ptr;

        if (packet->flags != EB_BUFFERFLAG_EOS &&
            packet->flags != EB_BUFFERFLAG_SHOW_EXT &&
            packet->flags != (EB_BUFFERFLAG_SHOW_EXT | EB_BUFFERFLAG_EOS) &&
            packet->flags != 0) {
            return_error = EB_ErrorMax;
        }

        // return the output stream buffer
        *p_buffer = packet;

        // save the wrapper pointer for the release
        (*p_buffer)->wrapper_ptr = (void*)eb_wrapper_ptr;
    }
    else
        return_error = EB_NoErrorEmptyQueue;

    return return_error;
}

static void switch_to_real_time() {

#if  __linux__

    struct sched_param schedParam = {
        .sched_priority = 99
    };

    int retValue = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);
    if (retValue == EPERM)
        SVT_LOG("\n[WARNING] For best speed performance, run with sudo privileges !\n\n");

#endif
}

/**************************************
 * EbBufferHeaderType Constructor
 **************************************/
EbErrorType  eb_buffer_header_ctor(
    EbPtr  *object_dbl_ptr,
    EbPtr  object_init_data_ptr){

    *object_dbl_ptr = (EbPtr)EB_NULL;
    object_init_data_ptr = (EbPtr)EB_NULL;

    (void)object_dbl_ptr;
    (void)object_init_data_ptr;

    return EB_ErrorNone;
}

#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_svt_enc_stream_header(
    EbComponentType           *svt_enc_component,
    EbBufferHeaderType        **output_stream_ptr) {

    EbErrorType             return_error = EB_ErrorNone;
    UNUSED(svt_enc_component);
    UNUSED(output_stream_ptr);
    return return_error;
}
//
#if defined(__linux__) || defined(__APPLE__)
__attribute__((visibility("default")))
#endif
EB_API EbErrorType eb_vp9_svt_enc_eos_nal(
    EbComponentType           *svt_enc_component,
    EbBufferHeaderType       **output_stream_ptr
)
{
    EbErrorType           return_error = EB_ErrorNone;
    UNUSED(svt_enc_component);
    UNUSED(output_stream_ptr);
    return return_error;
}

/**********************************
* Encoder Handle Initialization
**********************************/
static EbErrorType init_svt_vp9_encoder_handle(
    EbComponentType * h_component){

    EbErrorType       return_error = EB_ErrorNone;
    EbComponentType  *svt_enc_component = (EbComponentType*)h_component;

    SVT_LOG("-------------------------------------------\n");
    SVT_LOG("SVT [version]\t: SVT-VP9 Encoder Lib v%d.%d.%d\n", SVT_VERSION_MAJOR, SVT_VERSION_MINOR, SVT_VERSION_PATCHLEVEL);
#if ( defined( _MSC_VER ) && (_MSC_VER < 1910) )
    SVT_LOG("SVT [build]\t: Visual Studio 2013");
#elif ( defined( _MSC_VER ) && (_MSC_VER >= 1910) )
    SVT_LOG("SVT [build]\t: Visual Studio 2017");
#elif defined(__GNUC__)
    SVT_LOG("SVT [build]\t: GCC %d.%d.%d\t", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
    SVT_LOG("SVT [build]\t: unknown compiler");
#endif
    SVT_LOG(" %u bit\n", (unsigned) sizeof(void*) * 8);
    SVT_LOG("LIB Build date: %s %s\n", __DATE__, __TIME__);
    SVT_LOG("-------------------------------------------\n");

    switch_to_real_time();

    // Set Component Size & Version
    svt_enc_component->n_size = sizeof(EbComponentType);

    // Encoder Private Handle Ctor
    return_error = (EbErrorType)eb_enc_handle_ctor(
        (EbEncHandle**) &(svt_enc_component->p_component_private),
        svt_enc_component);

    return return_error;
}

/* SAFE STRING LIBRARY */

#ifndef EOK
#define EOK             ( 0 )
#endif

#ifndef ESZEROL
#define ESZEROL         ( 401 )       /* length is zero              */
#endif

#ifndef ESLEMIN
#define ESLEMIN         ( 402 )       /* length is below min         */
#endif

#ifndef ESLEMAX
#define ESLEMAX         ( 403 )       /* length exceeds max          */
#endif

#ifndef ESNULLP
#define ESNULLP         ( 400 )       /* null ptr                    */
#endif

#ifndef ESOVRLP
#define ESOVRLP         ( 404 )       /* overlap undefined           */
#endif

#ifndef ESEMPTY
#define ESEMPTY         ( 405 )       /* empty string                */
#endif

#ifndef ESNOSPC
#define ESNOSPC         ( 406 )       /* not enough space for s2     */
#endif

#ifndef ESUNTERM
#define ESUNTERM        ( 407 )       /* unterminated string         */
#endif

#ifndef ESNODIFF
#define ESNODIFF        ( 408 )       /* no difference               */
#endif

#ifndef ESNOTFND
#define ESNOTFND        ( 409 )       /* not found                   */
#endif

#define RSIZE_MAX_MEM      ( 256UL << 20 )     /* 256MB */

#define RCNEGATE(x)  (x)
#define RSIZE_MAX_STR      ( 4UL << 10 )      /* 4KB */

#ifndef sldebug_printf
#define sldebug_printf(...)
#endif

/*
* Function used by the libraries to invoke the registered
* runtime-constraint handler. Always needed.
*/

typedef void(*ConstraintHandler) (
    const char * /* msg */,
    void *       /* ptr */,
    Errno      /* error */);
/*
* Function used by the libraries to invoke the registered
* runtime-constraint handler. Always needed.
*/
static inline void invoke_safe_str_constraint_handler(
    const char *msg,
    void *ptr,
    Errno error);

static inline void invoke_safe_str_constraint_handler(
    const char *msg,
    void *ptr,
    Errno error)
{
    (void)ptr;
    sldebug_printf("IGNORE CONSTRAINT HANDLER: (%u) %s\n", error,
    (msg) ? msg : "Null message");
}

EB_API rsize_t eb_vp9_strnlen_ss(const char *dest, rsize_t dmax)
{
    rsize_t count = 0;

    if (dest == NULL) {
        return RCNEGATE(0);
    }

    if (dmax == 0) {
        invoke_safe_str_constraint_handler("strnlen_ss: dmax is 0",
            NULL, ESZEROL);
        return RCNEGATE(0);
    }

    if (dmax > RSIZE_MAX_STR) {
        invoke_safe_str_constraint_handler("strnlen_ss: dmax exceeds max",
            NULL, ESLEMAX);
        return RCNEGATE(0);
    }

    while (*dest && dmax) {
        count++;
        dmax--;
        dest++;
    }

    return RCNEGATE(count);
}

/* SAFE STRING LIBRARY */
