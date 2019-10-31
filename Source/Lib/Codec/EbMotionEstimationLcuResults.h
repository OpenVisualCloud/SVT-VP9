/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationLcuResults_h
#define EbMotionEstimationLcuResults_h

#include "EbDefinitions.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_ME_PU_COUNT         85  // Sum of all the possible partitions which have both deminsions greater than 4.
    // i.e. no 4x4, 8x4, or 4x8 partitions
#define SQUARE_PU_COUNT          85
#define MAX_ME_CANDIDATE_PER_PU   3

typedef struct MeCandidate
{
    union {
        struct {
            signed short     x_mv_l0 ;  //Note: Do not change the order of these fields
            signed short     y_mv_l0 ;
            signed short     x_mv_l1 ;
            signed short     y_mv_l1 ;
        }mv;
        uint64_t MVs;
    };

    unsigned    distortion : 32;     // 20-bits holds maximum SAD of 64x64 PU

    unsigned    direction : 8;      // 0: uni-pred L0, 1: uni-pred L1, 2: bi-pred

} MeCandidate;

typedef struct  DistDir
{
    unsigned    distortion : 32;
    unsigned    direction  :  2;
} DistDir;

typedef struct MeCuResults
{
    union {
        struct {
            signed short     x_mv_l0;
            signed short     y_mv_l0;
            signed short     x_mv_l1;
            signed short     y_mv_l1;
        };
        uint64_t MVs;
    };

    DistDir      distortion_direction[3];

    uint8_t      total_me_candidate_index;

} MeCuResults;

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationLcuResults_h
