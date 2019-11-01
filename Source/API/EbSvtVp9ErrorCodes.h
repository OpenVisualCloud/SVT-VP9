/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSvtVp9ErrorCodes_h
#define EbSvtVp9ErrorCodes_h

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define   CHECK_REPORT_ERROR(cond, app_callback_ptr, error_code)  { if(!(cond)){(app_callback_ptr)->error_handler(((app_callback_ptr)->handle),(error_code));while(1);}  }
#define   CHECK_REPORT_ERROR_NC(app_callback_ptr, error_code)     { {(app_callback_ptr)->error_handler(((app_callback_ptr)->handle),(error_code));while(1);} }
//#define   CHECK_REPORT_ERROR(cond, app_callback_ptr, error_code)  { (void)app_callback_ptr,(void)error_code;  }
//#define   CHECK_REPORT_ERROR_NC(app_callback_ptr, error_code)     { (void)app_callback_ptr,(void)error_code; }

typedef enum EncoderErrorCodes
{

    //EB_ENC_PM_ERRORS                  = 0x0000,
    EB_ENC_PM_ERROR0                    = 0x0000,
    EB_ENC_PM_ERROR1                    = 0x0001,
    EB_ENC_PM_ERROR2                    = 0x0002,
    EB_ENC_PM_ERROR3                    = 0x0003,
    EB_ENC_PM_ERROR4                    = 0x0004,
    EB_ENC_PM_ERROR5                    = 0x0005,
    EB_ENC_PM_ERROR6                    = 0x0006,
    EB_ENC_PM_ERROR7                    = 0x0007,
    EB_ENC_PM_ERROR8                    = 0x0008,

    //EB_ENC_RC_ERRORS                  = 0x0100
    EB_ENC_RC_ERROR0                    = 0x0100,

    //EB_ENC_ERRORS                        = 0x0200,
    EB_ENC_ROB_OF_ERROR                    = 0x0200,

    //EB_ENC_PD_ERRORS                  = 0x0300,
    EB_ENC_PD_ERROR1                    = 0x0300,
    EB_ENC_PD_ERROR2                    = 0x0301,
    EB_ENC_PD_ERROR3                    = 0x0302,
    EB_ENC_PD_ERROR4                    = 0x0303,
    EB_ENC_PD_ERROR5                    = 0x0304,
    EB_ENC_PD_ERROR6                    = 0x0306,
    EB_ENC_PD_ERROR7                    = 0x0307,

} EncoderErrorCodes;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbSvtVp9ErrorCodes_h
