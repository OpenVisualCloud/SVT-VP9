/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBSEI_h
#define EBSEI_h

#include "EbDefinitions.h"
#include "EbApiSei.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
extern EbErrorType eb_video_usability_info_ctor(
    AppVideoUsabilityInfo    *vui_ptr);

extern void eb_video_usability_info_copy(
    AppVideoUsabilityInfo    *dst_vui_ptr,
    AppVideoUsabilityInfo    *src_vui_ptr);

extern void eb_picture_timeing_sei_ctor(
    AppPictureTimingSei      *pic_timing_ptr);

extern void eb_buffering_period_sei_ctor(
    AppBufferingPeriodSei  *buffering_period_ptr);

extern void eb_recovery_point_sei_ctor(
    AppRecoveryPoint       *recovery_point_sei_ptr);

extern uint32_t get_picture_timing_sei_length(
    AppPictureTimingSei      *pic_timing_sei_ptr,
    AppVideoUsabilityInfo    *vui_ptr);

extern uint32_t get_buf_period_sei_length(
    AppBufferingPeriodSei  *buffering_period_ptr,
    AppVideoUsabilityInfo    *vui_ptr);

extern uint32_t get_recovery_point_sei_length(
    AppRecoveryPoint       *recovery_point_sei_ptr);
#ifdef __cplusplus
}
#endif
#endif // EBSEI_h
