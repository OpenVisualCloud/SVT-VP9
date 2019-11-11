/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbModeDecisionConfigurationProcess_h
#define EbModeDecisionConfigurationProcess_h

#include "EbSystemResourceManager.h"
#include "EbDefinitions.h"
#include "EbRateControlProcess.h"
#include "EbSequenceControlSet.h"
#include "EbModeDecision.h"
#include "vpx_convolve.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Defines
 **************************************/
#define Pred        0x01
#define Predp1      0x02
#define Predp2      0x04
#define Predp3      0x08
#define Predm1      0x10
#define Predm2      0x20
#define Predm3      0x40
#define ALL64       0x0F
#define ALL32       0x17
#define ALL16       0x33
#define ALL8        0x71
#define AllD        0x80

EB_ALIGN(16) static const uint8_t ndp_refinement_control_nref[MAX_TEMPORAL_LAYERS/*temporal layer*/][4/*cu Size*/] =
{
    //   64                         32                              16                  8
    /* layer-0 */      { Pred + Predp1 + Predp2     , Pred + Predp1                  ,Pred + Predp1          ,Pred + Predm1         },
    /* layer-1 */      { Pred + Predp1 + Predp2     , Pred + Predp1                  ,Pred + Predp1          ,Pred + Predm1         },
    /* layer-2 */      { Pred + Predp1 + Predp2     , Pred + Predp1                  ,Pred + Predp1          ,Pred + Predm1         },
    /* layer-3 */      { Pred + Predp1            , Pred + Predp1                  , Pred + Predp1        , Pred + Predm1      },
    /* layer-4 */      { Pred + Predp1            , Pred + Predp1                  , Pred + Predp1        , Pred + Predm1      },
    /* layer-5 */      { Pred + Predp1            , Pred + Predp1                  , Pred + Predp1        , Pred + Predm1      },
};

EB_ALIGN(16) static const uint8_t ndp_refinement_control_fast[MAX_TEMPORAL_LAYERS/*temporal layer*/][4/*cu Size*/] =
{
    //   64                         32                              16                  8
    /* layer-0 */      { Pred + Predp1 + Predp2       , Pred + Predp1                  ,Pred + Predp1          ,Pred + Predm1         },
    /* layer-1 */      { Pred                       , Pred + Predp1                  ,Pred + Predp1          ,Pred + Predm1         },
    /* layer-2 */      { Pred                       , Pred + Predp1                  ,Pred + Predp1          ,Pred + Predm1         },
    /* layer-3 */       { Pred                       , Pred                           , Pred                 , Pred + Predm1      },
    /* layer-4 */       { Pred                       , Pred                           , Pred                 , Pred + Predm1      },
    /* layer-5 */       { Pred                       , Pred                           , Pred                 , Pred + Predm1      },
};

EB_ALIGN(16) static const uint8_t ndp_refinement_control_islice[4/*cu Size*/] =
{
    Predp1, Pred + Predp1 + Predp2, Predm1 + Pred + Predp1, Predm1 + Pred
};

// Max FullCandidates Settings For Intra Frame
EB_ALIGN(16) static const uint8_t ndp_refinement_control_islice_sub4_k[4/*cu Size*/] =
{
    Predp1 + Predp2             ,Pred + Predp1 + Predp2         ,Predm1 + Pred + Predp1     ,Predm1 + Pred
};

typedef struct MdcpLocalCodingUnit
{
    uint64_t                        early_cost;
    EB_BOOL                         early_split_flag;
    EB_BOOL                         slected_cu;
    EB_BOOL                         stop_split;
#if 0 // Hsan: partition rate not helping @ open loop partitioning
    int                             partition_context;
#endif

} MdcpLocalCodingUnit;

typedef struct ModeDecisionConfigurationContext
{
    EbFifo                     *rate_control_input_fifo_ptr;
    EbFifo                     *mode_decision_configuration_output_fifo_ptr;

    uint8_t                     qp;
    MdcpLocalCodingUnit         local_cu_array[PA_BLOCK_MAX_COUNT];

    // Inter depth decision
    uint8_t                     group_of8x8_blocks_count;
    uint8_t                     group_of16x16_blocks_count;

    // Budgeting
    uint32_t                   *sb_score_array;
    uint8_t                     cost_depth_mode[SB_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE];
    uint8_t                    *sb_cost_array;
    uint32_t                    predicted_cost;
    uint32_t                    budget;
    int8_t                      score_th[MAX_SUPPORTED_SEGMENTS];
    uint8_t                     interval_cost[MAX_SUPPORTED_SEGMENTS];
    uint8_t                     number_of_segments;
    uint32_t                    sb_min_score;
    uint32_t                    sb_max_score;

    EbAdpDepthSensitivePicClass adp_depth_sensitive_picture_class;

    EbAdpRefinementMode         adp_refinement_mode;

    // Multi - Mode signal(s)
    uint8_t                     adp_level;

    PaBlockStats               *block_stats;

#if VP9_RD
    MbModeInfoExt              *mbmi_ext;
    uint32_t                    ref_costs_single[MAX_REF_FRAMES];
    uint32_t                    ref_costs_comp[MAX_REF_FRAMES];
    vpx_prob                    comp_mode_p;
    ModeDecisionCandidate      *candidate_ptr;
    MACROBLOCKD                *e_mbd;
#endif
#if SEG_SUPPORT
    int                         qindex_delta[MAX_SEGMENTS];
#endif
    int                         rd_mult_sad;

} ModeDecisionConfigurationContext;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EbErrorType eb_vp9_mode_decision_configuration_context_ctor(
    ModeDecisionConfigurationContext **context_dbl_ptr,
    EbFifo                            *rate_control_input_fifo_ptr,
    EbFifo                            *mode_decision_configuration_output_fifo_ptr,
    uint16_t                           sb_total_count);

extern void* eb_vp9_mode_decision_configuration_kernel(void *input_ptr);

#ifdef __cplusplus
}
#endif
#endif // EbModeDecisionConfigurationProcess_h
