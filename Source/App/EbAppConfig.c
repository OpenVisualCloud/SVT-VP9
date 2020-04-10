/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EbApiVersion.h"
#include "EbAppConfig.h"

#ifdef _WIN32
#else
#include <unistd.h>
#endif

/**********************************
 * Defines
 **********************************/
#define HELP_TOKEN                      "--help"
#define VERSION_TOKEN                   "--version"
#define CHANNEL_NUMBER_TOKEN            "-nch"
#define COMMAND_LINE_MAX_SIZE           2048
#define CONFIG_FILE_TOKEN               "-c"
#define INPUT_FILE_TOKEN                "-i"
#define OUTPUT_BITSTREAM_TOKEN          "-b"
#define OUTPUT_RECON_TOKEN              "-o"
#define ERROR_FILE_TOKEN                "-errlog"
#define QP_FILE_TOKEN                   "-qp-file"
#define WIDTH_TOKEN                     "-w"
#define HEIGHT_TOKEN                    "-h"
#define NUMBER_OF_PICTURES_TOKEN        "-n"
#define BUFFERED_INPUT_TOKEN            "-nb"
#define BASE_LAYER_SWITCH_MODE_TOKEN    "-base-layer-switch-mode" // no Eval
#define QP_TOKEN                        "-q"
#define USE_QP_FILE_TOKEN               "-use-q-file"
#define TUNE_TOKEN                      "-tune"
#define FRAME_RATE_TOKEN                "-fps"
#define FRAME_RATE_NUMERATOR_TOKEN      "-fps-num"
#define FRAME_RATE_DENOMINATOR_TOKEN    "-fps-denom"
#define ENCODER_BIT_DEPTH               "-bit-depth"
#define INPUT_COMPRESSED_TEN_BIT_FORMAT "-compressed-ten-bit-format"
#define ENCMODE_TOKEN                   "-enc-mode"
#define PRED_STRUCT_TOKEN               "-pred-struct"
#define INTRA_PERIOD_TOKEN              "-intra-period"
#define PROFILE_TOKEN                   "-profile"
#define LEVEL_TOKEN                     "-level"
#define INTERLACED_VIDEO_TOKEN          "-interlaced-video"
#define SEPERATE_FILDS_TOKEN            "-separate-fields"
#define LOOP_FILTER_TOKEN               "-loop-filter"
#define USE_DEFAULT_ME_HME_TOKEN        "-use-default-me-hme"
#define HME_ENABLE_TOKEN                "-hme"      // no Eval
#define SEARCH_AREA_WIDTH_TOKEN         "-search-w" // no Eval
#define SEARCH_AREA_HEIGHT_TOKEN        "-search-h" // no Eval
#define IMPROVE_SHARPNESS_TOKEN         "-sharp"
#define BITRATE_REDUCTION_TOKEN         "-brr"
#define VIDEO_USE_INFO_TOKEN            "-vid-info"
#define HDR_INPUT_TOKEN                 "-hdr"
#define RATE_CONTROL_ENABLE_TOKEN       "-rc"
#define TARGET_BIT_RATE_TOKEN           "-tbr"
#define VBV_MAX_RATE_TOKEN              "-vbv-maxrate"
#define VBV_BUFFER_SIZE_TOKEN           "-vbv-bufsize"
#define MAX_QP_TOKEN                    "-max-qp"
#define MIN_QP_TOKEN                    "-min-qp"
#define INJECTOR_TOKEN                  "-inj"  // no Eval
#define INJECTOR_FRAMERATE_TOKEN        "-inj-frm-rt" // no Eval
#define SPEED_CONTROL_TOKEN             "-speed-ctrl"
#define ASM_TYPE_TOKEN                  "-asm" // no Eval
#define THREAD_MGMNT                    "-lp"
#define TARGET_SOCKET                   "-ss"
#define CONFIG_FILE_COMMENT_CHAR        '#'
#define CONFIG_FILE_NEWLINE_CHAR        '\n'
#define CONFIG_FILE_RETURN_CHAR         '\r'
#define CONFIG_FILE_VALUE_SPLIT         ':'
#define CONFIG_FILE_SPACE_CHAR          ' '
#define CONFIG_FILE_ARRAY_SEP_CHAR      CONFIG_FILE_SPACE_CHAR
#define CONFIG_FILE_TAB_CHAR            '\t'
#define CONFIG_FILE_NULL_CHAR           '\0'
#define CONFIG_FILE_MAX_ARG_COUNT       256
#define CONFIG_FILE_MAX_VAR_LEN         128
#define EVENT_FILE_MAX_ARG_COUNT        20
#define EVENT_FILE_MAX_VAR_LEN          256
#define BUFFER_FILE_MAX_ARG_COUNT       320
#define BUFFER_FILE_MAX_VAR_LEN         128

/**********************************
 * Set Cfg Functions
 **********************************/
static void set_cfg_input_file (const char *value, EbConfig *cfg)
{
    if (cfg->input_file && cfg->input_file != stdin) {
        fclose(cfg->input_file);
    }
    if (!strcmp(value, "stdin")) {
        cfg->input_file = stdin;
    }
    else {
        FOPEN(cfg->input_file, value, "rb");
    }
};
static void set_cfg_stream_file (const char *value, EbConfig *cfg)
{
    if (cfg->bitstream_file) {
        fclose(cfg->bitstream_file);
    }
    FOPEN(cfg->bitstream_file,value, "wb");
};
static void set_cfg_recon_file(const char *value, EbConfig *cfg)
{
    if (cfg->recon_file) {
        fclose(cfg->recon_file);
    }
    FOPEN(cfg->recon_file,value, "wb");
};
static void set_cfg_error_file (const char *value, EbConfig *cfg)
{
    if (cfg->error_log_file) {
        fclose(cfg->error_log_file);
    }
    FOPEN(cfg->error_log_file,value, "w+");
};

static void set_cfg_qp_file (const char *value, EbConfig *cfg)
{
    if (cfg->qp_file) {
        fclose(cfg->qp_file);
    }
    FOPEN(cfg->qp_file,value, "r");
};
static void set_cfg_source_width                        (const char *value, EbConfig *cfg) {cfg->source_width         = strtoul(value, NULL, 0);};
static void set_cfg_source_height                       (const char *value, EbConfig *cfg) {cfg->source_height        = strtoul(value, NULL, 0) >> 0;};
static void set_cfg_frames_to_be_encoded                (const char *value, EbConfig *cfg) {cfg->frames_to_be_encoded = strtoll(value,  NULL, 0) << 0;};
static void set_buffered_input                          (const char *value, EbConfig *cfg) { cfg->buffered_input      = strtol(value, NULL, 0);};

static void set_frame_rate(const char *value, EbConfig *cfg) {
    cfg->frame_rate = strtoul(value, NULL, 0);
    if (cfg->frame_rate <= 1000 ){
        cfg->frame_rate = cfg->frame_rate << 16;
    }
}
static void set_frame_rate_numerator                   (const char *value, EbConfig *cfg) { cfg->frame_rate_numerator               = strtoul(value, NULL, 0);};
static void set_frame_rate_denominator                 (const char *value, EbConfig *cfg) { cfg->frame_rate_denominator             = strtoul(value, NULL, 0);};
static void set_encoder_bit_depth                      (const char *value, EbConfig *cfg) {cfg->encoder_bit_depth                   = strtoul(value, NULL, 0);}
static void set_base_layer_switch_mode                 (const char *value, EbConfig *cfg) {cfg->base_layer_switch_mode              = (uint8_t) strtoul(value, NULL, 0);};
static void set_enc_mode                               (const char *value, EbConfig *cfg) {cfg->enc_mode                            = (uint8_t)strtoul(value, NULL, 0);};
static void set_cfg_intra_period                       (const char *value, EbConfig *cfg) {cfg->intra_period                        = strtol(value,  NULL, 0);};
static void set_cfg_pred_structure                     (const char *value, EbConfig *cfg) { cfg->pred_structure                     = strtol(value, NULL, 0); };
static void set_cfg_qp                                 (const char *value, EbConfig *cfg) {cfg->qp                                  = strtoul(value, NULL, 0);};
static void set_cfg_use_qp_file                        (const char *value, EbConfig *cfg) {cfg->use_qp_file                         = (uint8_t)strtol(value, NULL, 0); };
static void set_loop_filter                            (const char *value, EbConfig *cfg) {cfg->loop_filter                         = (uint8_t)strtoul(value, NULL, 0);};
static void set_enable_hme_flag                        (const char *value, EbConfig *cfg) {cfg->enable_hme_flag                     = (uint8_t)strtoul(value, NULL, 0);};
static void set_rate_control_mode                      (const char *value, EbConfig *cfg) {cfg->rate_control_mode                   = strtoul(value, NULL, 0);};
static void set_vbv_maxrate                            (const char *value, EbConfig *cfg) { cfg->vbv_max_rate = strtoul(value, NULL, 0); };
static void set_vbv_bufsize                              (const char *value, EbConfig *cfg) { cfg->vbv_buf_size = strtoul(value, NULL, 0); };
static void set_target_bit_rate                        (const char *value, EbConfig *cfg) {cfg->target_bit_rate                     = strtoul(value, NULL, 0);};
static void set_max_qp_allowed                         (const char *value, EbConfig *cfg) {cfg->max_qp_allowed                      = strtoul(value, NULL, 0);};
static void set_min_qp_allowed                         (const char *value, EbConfig *cfg) {cfg->min_qp_allowed                      = strtoul(value, NULL, 0);};
static void set_cfg_search_area_width                  (const char *value, EbConfig *cfg) {cfg->search_area_width                   = strtoul(value, NULL, 0);};
static void set_cfg_search_area_height                 (const char *value, EbConfig *cfg) {cfg->search_area_height                  = strtoul(value, NULL, 0);};
static void set_cfg_use_default_me_hme                 (const char *value, EbConfig *cfg) {cfg->use_default_me_hme                  = (uint8_t)strtol(value, NULL, 0); };
static void set_cfg_tune                               (const char *value, EbConfig *cfg) { cfg->tune                               = (uint8_t)strtoul(value, NULL, 0); };
static void set_profile                                (const char *value, EbConfig *cfg) {cfg->profile                             = strtol(value,  NULL, 0);};

static void set_level (const char *value, EbConfig *cfg) {
    if (strtoul( value, NULL,0) != 0 || EB_STRCMP(value, "0") == 0 )
        cfg->level = (uint32_t)(10*strtod(value,  NULL));
    else
        cfg->level = 9999999;
};

static void             set_injector                         (const char *value, EbConfig *cfg) {cfg->injector = strtol(value,  NULL, 0);};

static void             speed_control_flag                    (const char *value, EbConfig *cfg) { cfg->speed_control_flag = strtol(value, NULL, 0); };

static void set_injector_frame_rate(const char *value, EbConfig *cfg) {
    cfg->injector_frame_rate = strtoul(value, NULL, 0);
    if (cfg->injector_frame_rate <= 1000 ){
        cfg->injector_frame_rate = cfg->injector_frame_rate << 16;
    }
}

static void set_asm_type                           (const char *value, EbConfig *cfg)  { cfg->asm_type                          = (uint8_t)strtoul(value, NULL, 0); };
static void set_target_socket                      (const char *value, EbConfig *cfg)  { cfg->target_socket                     = (int32_t)strtoul(value, NULL, 0); };
static void set_logical_processors                 (const char *value, EbConfig *cfg)  { cfg->logical_processors                = (uint32_t)strtoul(value, NULL, 0); };
enum CfgType{
    SINGLE_INPUT,   // Configuration parameters that have only 1 value input
    ARRAY_INPUT     // Configuration parameters that have multiple values as input
};

/**********************************
 * Config Entry Struct
 **********************************/
typedef struct ConfigEntry
{
    enum  CfgType type;
    const char   *token;
    const char   *name;
    void (*scf)(const char *, EbConfig *);
} ConfigEntry;

/**********************************
 * Config Entry Array
 **********************************/
ConfigEntry config_entry[] = {

    // File I/O
    { SINGLE_INPUT, INPUT_FILE_TOKEN, "InputFile", set_cfg_input_file },
    { SINGLE_INPUT, OUTPUT_BITSTREAM_TOKEN,   "StreamFile",       set_cfg_stream_file },
    { SINGLE_INPUT, OUTPUT_RECON_TOKEN, "ReconFile", set_cfg_recon_file },
    { SINGLE_INPUT, ERROR_FILE_TOKEN, "ErrorFile", set_cfg_error_file },
    { SINGLE_INPUT, QP_FILE_TOKEN, "QpFile", set_cfg_qp_file },

    // Picture Dimensions
    { SINGLE_INPUT, WIDTH_TOKEN, "SourceWidth", set_cfg_source_width },
    { SINGLE_INPUT, HEIGHT_TOKEN, "SourceHeight", set_cfg_source_height },

    // Prediction Structure
    { SINGLE_INPUT, NUMBER_OF_PICTURES_TOKEN, "FrameToBeEncoded", set_cfg_frames_to_be_encoded },
    { SINGLE_INPUT, BUFFERED_INPUT_TOKEN, "BufferedInput", set_buffered_input },
    { SINGLE_INPUT, BASE_LAYER_SWITCH_MODE_TOKEN, "BaseLayerSwitchMode", set_base_layer_switch_mode },
    { SINGLE_INPUT, ENCMODE_TOKEN, "EncoderMode", set_enc_mode},
    { SINGLE_INPUT, INTRA_PERIOD_TOKEN, "IntraPeriod", set_cfg_intra_period },
    { SINGLE_INPUT, FRAME_RATE_TOKEN, "FrameRate", set_frame_rate },
    { SINGLE_INPUT, FRAME_RATE_NUMERATOR_TOKEN, "FrameRateNumerator", set_frame_rate_numerator },
    { SINGLE_INPUT, FRAME_RATE_DENOMINATOR_TOKEN, "FrameRateDenominator", set_frame_rate_denominator },
    { SINGLE_INPUT, ENCODER_BIT_DEPTH, "EncoderBitDepth", set_encoder_bit_depth },
    { SINGLE_INPUT, PRED_STRUCT_TOKEN, "PredStructure", set_cfg_pred_structure },

    // Rate Control
    { SINGLE_INPUT, QP_TOKEN, "QP", set_cfg_qp },

    { SINGLE_INPUT, USE_QP_FILE_TOKEN, "UseQpFile", set_cfg_use_qp_file },

    { SINGLE_INPUT, RATE_CONTROL_ENABLE_TOKEN, "RateControlMode", set_rate_control_mode },
    { SINGLE_INPUT, TARGET_BIT_RATE_TOKEN, "TargetBitRate", set_target_bit_rate },
    { SINGLE_INPUT, MAX_QP_TOKEN, "MaxQpAllowed", set_max_qp_allowed },
    { SINGLE_INPUT, MIN_QP_TOKEN, "MinQpAllowed", set_min_qp_allowed },
    { SINGLE_INPUT, VBV_MAX_RATE_TOKEN, "vbvMaxRate", set_vbv_maxrate },
    { SINGLE_INPUT, VBV_BUFFER_SIZE_TOKEN, "vbvBufsize", set_vbv_bufsize },

    // Loop Filter
    { SINGLE_INPUT, LOOP_FILTER_TOKEN, "LoopFilter", set_loop_filter },

    // ME Tools
    { SINGLE_INPUT, USE_DEFAULT_ME_HME_TOKEN, "UseDefaultMeHme", set_cfg_use_default_me_hme },
    { SINGLE_INPUT, HME_ENABLE_TOKEN, "HME", set_enable_hme_flag },

    // ME Parameters
    { SINGLE_INPUT, SEARCH_AREA_WIDTH_TOKEN, "SearchAreaWidth", set_cfg_search_area_width },
    { SINGLE_INPUT, SEARCH_AREA_HEIGHT_TOKEN, "SearchAreaHeight", set_cfg_search_area_height },

    // Tune
    { SINGLE_INPUT, TUNE_TOKEN, "Tune", set_cfg_tune },

    // Thread Management
    { SINGLE_INPUT, TARGET_SOCKET, "TargetSocket", set_target_socket },
    { SINGLE_INPUT, THREAD_MGMNT, "LogicalProcessors", set_logical_processors },

    // Latency
    { SINGLE_INPUT, INJECTOR_TOKEN, "Injector", set_injector },
    { SINGLE_INPUT, INJECTOR_FRAMERATE_TOKEN, "InjectorFrameRate", set_injector_frame_rate },
    { SINGLE_INPUT, SPEED_CONTROL_TOKEN, "SpeedControlFlag", speed_control_flag },

    // Annex A parameters
    { SINGLE_INPUT, PROFILE_TOKEN, "Profile", set_profile },
    { SINGLE_INPUT, LEVEL_TOKEN, "Level", set_level },

    // Asm Type
    { SINGLE_INPUT, ASM_TYPE_TOKEN, "AsmType", set_asm_type },

    // Termination
    {SINGLE_INPUT,NULL,  NULL,                                NULL}
};

/**********************************
 * Constructor
 **********************************/
void eb_config_ctor(EbConfig *config_ptr)
{
    config_ptr->config_file                                      = NULL;
    config_ptr->input_file                                       = NULL;
    config_ptr->bitstream_file                                   = NULL;;
    config_ptr->recon_file                                       = NULL;
    config_ptr->error_log_file                                   = stderr;
    config_ptr->qp_file                                          = NULL;

    config_ptr->frame_rate                                       = 60;
    config_ptr->frame_rate_numerator                             = 0;
    config_ptr->frame_rate_denominator                           = 0;
    config_ptr->encoder_bit_depth                                = 8;
    config_ptr->source_width                                     = 0;
    config_ptr->source_height                                    = 0;

    config_ptr->frames_to_be_encoded                             = 0;
    config_ptr->buffered_input                                   = -1;
    config_ptr->sequence_buffer                                  = 0;

    config_ptr->qp                                               = 45;
    config_ptr->use_qp_file                                      = EB_FALSE;

    config_ptr->rate_control_mode                                = 0;
    config_ptr->target_bit_rate                                  = 7000000;
    config_ptr->vbv_max_rate                                     = 0;
    config_ptr->vbv_buf_size                                     = 0;
    config_ptr->max_qp_allowed                                   = 63;
    config_ptr->min_qp_allowed                                   = 10;
    config_ptr->base_layer_switch_mode                           = 0;

    config_ptr->enc_mode                                         = 9;
    config_ptr->intra_period                                     = -2;

    config_ptr->pred_structure                                   = 2;

    config_ptr->loop_filter                                      = EB_TRUE;
    config_ptr->use_default_me_hme                               = EB_TRUE;
    config_ptr->enable_hme_flag                                  = EB_TRUE;
    config_ptr->search_area_width                                = 16;
    config_ptr->search_area_height                               = 7;

    config_ptr->tune                                             = 1;

    // Annex A parameters
    config_ptr->profile                                          = 0;
    config_ptr->level                                            = 0;

    // Latency
    config_ptr->injector                                         = 0;
    config_ptr->injector_frame_rate                              = 60;
    config_ptr->speed_control_flag                               = 0;

    // Testing
    config_ptr->eos_flag                                         = EB_FALSE;

// Computational Performance Parameters
    config_ptr->performance_context.frame_count                  = 0;
    config_ptr->performance_context.average_speed                = 0;
    config_ptr->performance_context.starts_time                  = 0;
    config_ptr->performance_context.start_utime                  = 0;
    config_ptr->performance_context.max_latency                  = 0;
    config_ptr->performance_context.total_latency                = 0;
    config_ptr->performance_context.byte_count                   = 0;

    // ASM Type
    config_ptr->asm_type                                         = 1;

    config_ptr->stop_encoder                                     = EB_FALSE;
    config_ptr->target_socket                                     = -1;
    config_ptr->logical_processors                                = 0;

    config_ptr->processed_frame_count                            = 0;
    config_ptr->processed_byte_count                             = 0;

    return;
}

/**********************************
 * Destructor
 **********************************/
void eb_config_dtor(EbConfig *config_ptr)
{
    // Close any files that are open
    if (config_ptr->config_file) {
        fclose(config_ptr->config_file);
        config_ptr->config_file = (FILE *) NULL;
    }

    if (config_ptr->input_file) {
        if (config_ptr->input_file != stdin)
            fclose(config_ptr->input_file);
        config_ptr->input_file = (FILE *) NULL;
    }

    if (config_ptr->bitstream_file) {
        fclose(config_ptr->bitstream_file);
        config_ptr->bitstream_file = (FILE *) NULL;
    }

    if (config_ptr->recon_file) {
        fclose(config_ptr->recon_file);
        config_ptr->recon_file = (FILE *)NULL;
    }

    if (config_ptr->error_log_file) {
        fclose(config_ptr->error_log_file);
        config_ptr->error_log_file = (FILE *) NULL;
    }

    if (config_ptr->qp_file) {
        fclose(config_ptr->qp_file);
        config_ptr->qp_file = (FILE *)NULL;
    }

    return;
}

/**********************************
 * File Size
 **********************************/
static int find_file_size(
    FILE * const p_file)
{
    int file_size;

    fseek(p_file, 0, SEEK_END);
    file_size = ftell(p_file);
    rewind(p_file);

    return file_size;
}

/**********************************
 * Line Split
 **********************************/
static void line_split(
    uint32_t   *argc,
    char       *argv   [CONFIG_FILE_MAX_ARG_COUNT],
    uint32_t    arg_len[CONFIG_FILE_MAX_ARG_COUNT],
    char       *line_ptr)
{
    uint32_t i=0;
    *argc = 0;

    while((*line_ptr != CONFIG_FILE_NEWLINE_CHAR) &&
            (*line_ptr != CONFIG_FILE_RETURN_CHAR) &&
            (*line_ptr != CONFIG_FILE_COMMENT_CHAR) &&
            (*argc < CONFIG_FILE_MAX_ARG_COUNT)) {
        // Increment past whitespace
        while((*line_ptr == CONFIG_FILE_SPACE_CHAR || *line_ptr == CONFIG_FILE_TAB_CHAR) && (*line_ptr != CONFIG_FILE_NEWLINE_CHAR)) {
            ++line_ptr;
        }

        // Set arg
        if ((*line_ptr != CONFIG_FILE_NEWLINE_CHAR) &&
                (*line_ptr != CONFIG_FILE_RETURN_CHAR) &&
                (*line_ptr != CONFIG_FILE_COMMENT_CHAR) &&
                (*argc < CONFIG_FILE_MAX_ARG_COUNT)) {
            argv[*argc] = line_ptr;

            // Increment to next whitespace
            while(*line_ptr != CONFIG_FILE_SPACE_CHAR &&
                    *line_ptr != CONFIG_FILE_TAB_CHAR &&
                    *line_ptr != CONFIG_FILE_NEWLINE_CHAR &&
                    *line_ptr != CONFIG_FILE_RETURN_CHAR) {
                ++line_ptr;
                ++i;
            }

            // Set arg length
            arg_len[(*argc)++] = i;

            i=0;
        }
    }

    return;
}

/**********************************
* Set Config value
**********************************/
static void set_config_value(
    EbConfig *config,
    char     *name,
    char     *value)
{
    int i=0;

    while(config_entry[i].name != NULL) {
        if(EB_STRCMP(config_entry[i].name, name) == 0)  {
            (*config_entry[i].scf)((const char *) value, config);
        }
        ++i;
    }

    return;
}

/**********************************
* Parse Config File
**********************************/
static void parse_config_file(
    EbConfig *config,
    char     *buffer,
    int       size)
{
    uint32_t argc;
    char *argv[CONFIG_FILE_MAX_ARG_COUNT];
    uint32_t arg_len[CONFIG_FILE_MAX_ARG_COUNT];

    char var_name[CONFIG_FILE_MAX_VAR_LEN];
    char var_value[CONFIG_FILE_MAX_ARG_COUNT][CONFIG_FILE_MAX_VAR_LEN];

    uint32_t value_index;

    uint32_t comment_section_flag = 0;
    uint32_t new_line_flag = 0;

    // Keep looping until we process the entire file
    while(size--) {
        comment_section_flag = ((*buffer == CONFIG_FILE_COMMENT_CHAR) || (comment_section_flag != 0)) ? 1 : comment_section_flag;

        // At the beginning of each line
        if ((new_line_flag == 1) && (comment_section_flag == 0)) {
            // Do an argc/argv split for the line
            line_split(&argc, argv, arg_len, buffer);

            if ((argc > 2) && (*argv[1] == CONFIG_FILE_VALUE_SPLIT)) {
                // ***NOTE - We're assuming that the variable name is the first arg and
                // the variable value is the third arg.

                // Cap the length of the variable name
                arg_len[0] = (arg_len[0] > CONFIG_FILE_MAX_VAR_LEN - 1) ? CONFIG_FILE_MAX_VAR_LEN - 1 : arg_len[0];
                // Copy the variable name
                EB_STRNCPY(var_name, argv[0], arg_len[0]);
                // Null terminate the variable name
                var_name[arg_len[0]] = CONFIG_FILE_NULL_CHAR;

                for(value_index=0; (value_index < CONFIG_FILE_MAX_ARG_COUNT - 2) && (value_index < (argc - 2)); ++value_index) {

                    // Cap the length of the variable
                    arg_len[value_index+2] = (arg_len[value_index+2] > CONFIG_FILE_MAX_VAR_LEN - 1) ? CONFIG_FILE_MAX_VAR_LEN - 1 : arg_len[value_index+2];
                    // Copy the variable name
                    EB_STRNCPY(var_value[value_index], argv[value_index+2], arg_len[value_index+2]);
                    // Null terminate the variable name
                    var_value[value_index][arg_len[value_index+2]] = CONFIG_FILE_NULL_CHAR;

                    set_config_value(config, var_name, var_value[value_index]);
                }
            }
        }

        comment_section_flag = (*buffer == CONFIG_FILE_NEWLINE_CHAR) ? 0 : comment_section_flag;
        new_line_flag = (*buffer == CONFIG_FILE_NEWLINE_CHAR) ? 1 : 0;
        ++buffer;
    }

    return;
}

/******************************************
* Find Token
******************************************/
static int find_token(
    int           argc,
    char * const  argv[],
    char const   *token,
    char         *config_str)
{
    int return_error = -1;

    while((argc > 0) && (return_error != 0)) {
        return_error = EB_STRCMP(argv[--argc], token);
        if (return_error == 0) {
            EB_STRCPY(config_str, COMMAND_LINE_MAX_SIZE, argv[argc + 1]);
        }
    }

    return return_error;
}

/**********************************
* Read Config File
**********************************/
static int read_config_file(
    EbConfig  *config,
    char      *config_path,
    uint32_t   instance_idx)
{
    int return_error = 0;

    // Open the config file
    FOPEN(config->config_file, config_path, "rb");

    if (config->config_file != (FILE*) NULL) {
        int config_file_size = find_file_size(config->config_file);
        char *config_file_buffer = (char*) malloc(config_file_size);

        if (config_file_buffer != (char *) NULL) {
            int result_size = (int) fread(config_file_buffer, 1, config_file_size, config->config_file);

            if (result_size == config_file_size) {
                parse_config_file(config, config_file_buffer, config_file_size);
            } else {
                printf("Error channel %u: File Read Failed\n",instance_idx+1);
                return_error = -1;
            }
        } else {
            printf("Error channel %u: Memory Allocation Failed\n",instance_idx+1);
            return_error = -1;
        }

        free(config_file_buffer);
        fclose(config->config_file);
        config->config_file = (FILE*) NULL;
    } else {
        printf("Error channel %u: Couldn't open Config File: %s\n", instance_idx+1,config_path);
        return_error = -1;
    }

    return return_error;
}

/******************************************
* Verify Settings
******************************************/
#define PowerOfTwoCheck(x) (((x) != 0) && (((x) & (~(x) + 1)) == (x)))

static EbErrorType verify_settings(EbConfig *config, uint32_t channel_number)
{
    EbErrorType return_error = EB_ErrorNone;

    // Check Input File
    if(config->input_file == (FILE*) NULL) {
        fprintf(config->error_log_file, "Error instance %u: Invalid Input File\n",channel_number+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->frames_to_be_encoded <= -1) {
        fprintf(config->error_log_file, "Error instance %u: FrameToBeEncoded must be greater than 0\n",channel_number+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->frames_to_be_encoded >= 0x7FFFFFFFFFFFFFFF) {
        fprintf(config->error_log_file, "Error instance %u: FrameToBeEncoded must be less than 2^64 - 1\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->buffered_input > config->frames_to_be_encoded) {
        fprintf(config->error_log_file, "Error instance %u: Invalid buffered_input. buffered_input must be less or equal to the number of frames to be encoded\n",channel_number+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->use_qp_file == EB_TRUE && config->qp_file == NULL) {
        fprintf(config->error_log_file, "Error instance %u: Could not find QP file, use_qp_file is set to 1\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->injector > 1 ){
        fprintf(config->error_log_file, "Error Instance %u: Invalid injector [0 - 1]\n",channel_number+1);
        return_error = EB_ErrorBadParameter;
    }
    if(config->injector_frame_rate > (240<<16) && config->injector){
        fprintf(config->error_log_file, "Error Instance %u: The maximum allowed injector_frame_rate is 240 fps\n",channel_number+1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the injector frame_rate is non-zero
    if(config->injector_frame_rate <= 0 && config->injector) {
        fprintf(config->error_log_file, "Error Instance %u: The injector frame rate should be greater than 0 fps \n",channel_number+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->speed_control_flag > 1) {
        fprintf(config->error_log_file, "Error Instance %u: Invalid Speed Control flag [0 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    // target_socket
    if (config->target_socket != -1 && config->target_socket != 0  && config->target_socket != 1) {
        fprintf(config->error_log_file, "Error instance %u: Invalid target_socket [-1 - 1]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    return return_error;
}

/******************************************
 * Find Token for multiple inputs
 ******************************************/
int find_token_multiple_inputs(
    int          argc,
    char* const  argv[],
    const char  *token,
    char       **config_str)
{
    int return_error = -1;
    int done = 0;
    while((argc > 0) && (return_error != 0)) {
        return_error = EB_STRCMP(argv[--argc], token);
        if (return_error == 0) {
            int count;
            for (count=0; count < MAX_CHANNEL_NUMBER  ; ++count){
                if (done ==0){
                    if (argv[argc + count + 1] ){
                        if (strtoul(argv[argc + count + 1], NULL,0) != 0 || EB_STRCMP(argv[argc + count + 1], "0") == 0 ){
                            EB_STRCPY(config_str[count], COMMAND_LINE_MAX_SIZE, argv[argc + count + 1]);
                        }else if (argv[argc + count + 1][0] != '-'){
                            EB_STRCPY(config_str[count], COMMAND_LINE_MAX_SIZE, argv[argc + count + 1]);
                        }else {
                            EB_STRCPY(config_str[count], COMMAND_LINE_MAX_SIZE," ");
                            done = 1;
                        }
                    }else{
                        EB_STRCPY(config_str[count], COMMAND_LINE_MAX_SIZE, " ");
                        done =1;
                        //return return_error;
                    }
                }else
                    EB_STRCPY(config_str[count], COMMAND_LINE_MAX_SIZE, " ");
            }
        }
    }

    return return_error;
}
/******************************************************
* Count the number of configuration files inputted
******************************************************/
static int count_channels_input (
    int         argc,
    char *const argv[])
{
    int return_error = -1;
    uint32_t count = 0;
    uint32_t argc_max = (uint32_t) argc;
    while((argc > 0) && (return_error != 0)) {
        return_error = EB_STRCMP(argv[--argc], "-c");
        if (return_error == 0) {
            while (((argc + count + 1) < argc_max) && (count < MAX_CHANNEL_NUMBER) && (argv[argc + count + 1][0] != '-')){
                ++count;
            }
            return count;
        }
    }
    return -1;
}

uint32_t get_help(int argc, char *const argv[])
{
    char config_string[COMMAND_LINE_MAX_SIZE];
    if (find_token(argc, argv, HELP_TOKEN, config_string) == 0) {
        int token_index = -1;
        printf("\n%-25s\t%-25s\t%-25s\t\n\n" ,"TOKEN", "DESCRIPTION", "INPUT TYPE");
        printf("%-25s\t%-25s\t%-25s\t\n" ,"-nch", "NumberOfChannels", "Single input");
        while (config_entry[++token_index].token != NULL) {
            printf("%-25s\t%-25s\t%-25s\t\n", config_entry[token_index].token, config_entry[token_index].name, config_entry[token_index].type ? "Array input": "Single input");
        }
        return 1;

    }
    return 0;
}

uint32_t get_svt_version(int argc, char *const argv[])
{
    char config_string[COMMAND_LINE_MAX_SIZE];
    if (find_token(argc, argv, VERSION_TOKEN, config_string) == 0) {
        printf("SVT-VP9 version %d.%d.%d\n", SVT_VERSION_MAJOR, SVT_VERSION_MINOR, SVT_VERSION_PATCHLEVEL);
        printf("Copyright(c) 2018 Intel Corporation\n");
        printf("BSD-2-Clause Plus Patent License\n");
        printf("https://github.com/OpenVisualCloud/SVT-VP9\n");
        return 1;
    }
    return 0;
}

/******************************************************
* Get the number of channels and validate it with input
******************************************************/
uint32_t get_number_of_channels(int argc, char *const argv[])
{
    char config_string[COMMAND_LINE_MAX_SIZE];
    uint32_t channel_number;
    if (find_token(argc, argv, CHANNEL_NUMBER_TOKEN, config_string) == 0) {

        // Set the input file
        channel_number = strtol(config_string,  NULL, 0);
        if ((channel_number > (uint32_t) MAX_CHANNEL_NUMBER) || channel_number == 0){
            printf("Error: The number of channels has to be within the range [1,%u]\n",(uint32_t) MAX_CHANNEL_NUMBER);
            return 0;
        }else{
            if (channel_number != (uint32_t) count_channels_input(argc,argv)){
                printf("Error: The number of channels has to match the number of config files inputted\n");
                return 0;
            }
            return channel_number;
        }
    }

    return 1;
}

void mark_token_as_read(
    const char *token,
    char       *cmd_copy[],
    int        *cmd_token_cnt
    )
{
    int cmd_copy_index;
    for (cmd_copy_index = 0; cmd_copy_index < *(cmd_token_cnt); ++cmd_copy_index) {
        if (!EB_STRCMP(cmd_copy[cmd_copy_index], token)) {
            cmd_copy[cmd_copy_index] = cmd_copy[--(*cmd_token_cnt)];
        }
    }
}
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)

// Computes the number of frames in the input file
int32_t compute_frames_to_be_encoded(
    EbConfig *config)
{
    uint64_t file_size = 0;
    int32_t frame_count = 0;
    if (config->input_file) {
        fseeko64(config->input_file, 0L, SEEK_END);
        file_size = ftello64(config->input_file);
    }

    uint32_t frame_size = SIZE_OF_ONE_FRAME_IN_BYTES(config->source_width, config->source_height, (uint8_t)((config->encoder_bit_depth == 10) ? 1 : 0));

    if (frame_size == 0)
        return -1;

    frame_count = (int32_t)(file_size / frame_size);

    if (frame_count == 0)
        return -1;

    return frame_count;

}

/******************************************
* Read Command Line
******************************************/
EbErrorType read_command_line(
    int          argc,
    char *const  argv[],
    EbConfig   **configs,
    uint32_t     num_channels,
    EbErrorType *return_errors)
{

    EbErrorType return_error = EB_ErrorBadParameter;
    char            config_string[COMMAND_LINE_MAX_SIZE];        // for one input options
    char           *config_strings[MAX_CHANNEL_NUMBER]; // for multiple input options
    char           *cmd_copy[MAX_NUM_TOKENS];                 // keep track of extra tokens
    uint32_t    index           = 0;
    int             cmd_token_cnt   = 0;                        // total number of tokens
    int             token_index     = -1;

    for (index = 0; index < MAX_CHANNEL_NUMBER; ++index){
        config_strings[index] = (char*)malloc(sizeof(char)*COMMAND_LINE_MAX_SIZE);
    }

    // Copy tokens (except for CHANNEL_NUMBER_TOKEN ) into a temp token buffer hosting all tokens that are passed through the command line
    size_t len = EB_STRLEN(CHANNEL_NUMBER_TOKEN, COMMAND_LINE_MAX_SIZE);
    for (token_index = 0; token_index < argc; ++token_index) {
        if ((argv[token_index][0] == '-') && strncmp(argv[token_index], CHANNEL_NUMBER_TOKEN, len)) {
            cmd_copy[cmd_token_cnt++] = argv[token_index];
        }
    }

    /***************************************************************************************************/
    /****************  Find configuration files tokens and call respective functions  ******************/
    /***************************************************************************************************/

    // Find the Config File Path in the command line
    if (find_token_multiple_inputs(argc, argv, CONFIG_FILE_TOKEN, config_strings) == 0) {

        mark_token_as_read(CONFIG_FILE_TOKEN, cmd_copy, &cmd_token_cnt);
        // Parse the config file
        for (index = 0; index < num_channels; ++index){
            return_errors[index] = (EbErrorType )read_config_file(configs[index], config_strings[index], index);
            return_error = (EbErrorType )(return_error &  return_errors[index]);
        }
    }
    else {
        if (find_token(argc, argv, CONFIG_FILE_TOKEN, config_string) == 0) {
            printf("Error: Config File Token Not Found\n");
            return EB_ErrorBadParameter;
        }
        else {
            return_error = EB_ErrorNone;
        }
    }

    /***************************************************************************************************/
    /***********   Find SINGLE_INPUT configuration parameter tokens and call respective functions  **********/
    /***************************************************************************************************/
    token_index = -1;
    // Parse command line for tokens
    while (config_entry[++token_index].name != NULL){
        if (config_entry[token_index].type == SINGLE_INPUT){

            if (find_token_multiple_inputs(argc, argv, config_entry[token_index].token, config_strings) == 0) {

                // When a token is found mark it as found in the temp token buffer
                mark_token_as_read(config_entry[token_index].token, cmd_copy, &cmd_token_cnt);

                // Fill up the values corresponding to each channel
                for (index = 0; index < num_channels; ++index) {
                    if (EB_STRCMP(config_strings[index], " ")) {
                        (*config_entry[token_index].scf)(config_strings[index], configs[index]);
                    }
                    else {
                        break;
                    }
                }
            }
        }
    }

    /***************************************************************************************************/
    /**************************************   Verify configuration parameters   ************************/
    /***************************************************************************************************/
    // Verify the config values
    if (return_error == 0) {
        return_error = EB_ErrorBadParameter;
        for (index = 0; index < num_channels; ++index){
            if (return_errors[index] == EB_ErrorNone){
                return_errors[index] = verify_settings(configs[index], index);

                // Assuming no errors, set the frames to be encoded to the number of frames in the input yuv
                if (return_errors[index] == EB_ErrorNone && configs[index]->frames_to_be_encoded == 0)
                    configs[index]->frames_to_be_encoded = compute_frames_to_be_encoded(configs[index]);

                if (configs[index]->frames_to_be_encoded == -1) {
                    fprintf(configs[index]->error_log_file, "Error instance %u: Input yuv does not contain enough frames \n", index + 1);
                    return_errors[index] = EB_ErrorBadParameter;
                }

                // Force the injector latency mode, and injector frame rate when speed control is on
                if (return_errors[index] == EB_ErrorNone && configs[index]->speed_control_flag == 1) {
                    configs[index]->injector    = 1;
                }

            }
            return_error = (EbErrorType )(return_error & return_errors[index]);
        }
    }

    // Print message for unprocessed tokens
    if (cmd_token_cnt > 0) {
        int32_t cmd_copy_index;
        printf("Unprocessed tokens: ");
        for (cmd_copy_index = 0; cmd_copy_index < cmd_token_cnt; ++cmd_copy_index) {
            printf(" %s ", cmd_copy[cmd_copy_index]);
        }
        printf("\n\n");
    }

    for (index = 0; index < MAX_CHANNEL_NUMBER; ++index){
        free(config_strings[index]);
    }

    return return_error;
}
