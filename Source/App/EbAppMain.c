/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// main.cpp
//  -Contructs the following resources needed during the encoding process
//      -memory
//      -threads
//      -semaphores
//      -semaphores
//  -Configures the encoder
//  -Calls the encoder via the API
//  -Destructs the resources

/***************************************
 * Includes
 ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include "EbAppConfig.h"
#include "EbAppContext.h"
#include "EbAppTime.h"
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h> /* _O_BINARY */
#include <io.h>    /* _setmode() */
#else
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#endif

/***************************************
 * External Functions
 ***************************************/
extern AppExitConditionType process_input_buffer(
    EbConfig             *config,
    EbAppContext         *app_call_back);

extern AppExitConditionType process_output_recon_buffer(
    EbConfig             *config,
    EbAppContext         *app_call_back);

extern AppExitConditionType process_output_stream_buffer(
    EbConfig             *config,
    EbAppContext         *app_call_back,
    uint8_t               pic_send_done);

volatile int32_t keep_running = 1;

void EventHandler(int32_t dummy) {
    (void)dummy;
    keep_running = 0;

    // restore default signal handler
    signal(SIGINT, SIG_DFL);
}

void AssignAppThreadGroup(uint8_t targetSocket) {
#ifdef _WIN32
    if (GetActiveProcessorGroupCount() == 2) {
        GROUP_AFFINITY           groupAffinity;
        GetThreadGroupAffinity(GetCurrentThread(), &groupAffinity);
        groupAffinity.Group = targetSocket;
        SetThreadGroupAffinity(GetCurrentThread(), &groupAffinity, NULL);
    }
#else
    (void)targetSocket;
    return;
#endif
}

/***************************************
 * Encoder App Main
 ***************************************/
int32_t main(int32_t argc, char* argv[])
{
    if (get_svt_version(argc, argv) || get_help(argc, argv))
        return EB_ErrorNone;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    // GLOBAL VARIABLES
    EbErrorType             return_error = EB_ErrorNone;            // Error Handling
    AppExitConditionType    exit_condition = APP_ExitConditionNone;    // Processing loop exit condition

    EbErrorType             return_errors[MAX_CHANNEL_NUMBER];          // Error Handling
    AppExitConditionType    exit_conditions[MAX_CHANNEL_NUMBER];          // Processing loop exit condition
    AppExitConditionType    exit_conditions_output[MAX_CHANNEL_NUMBER];         // Processing loop exit condition
    AppExitConditionType    exit_conditions_recon[MAX_CHANNEL_NUMBER];         // Processing loop exit condition
    AppExitConditionType    exit_conditions_input[MAX_CHANNEL_NUMBER];          // Processing loop exit condition

    EbBool                 channel_active[MAX_CHANNEL_NUMBER];

    EbConfig             *configs[MAX_CHANNEL_NUMBER];        // Encoder Configuration

    uint32_t                num_channels = 0;
    uint32_t                instance_count=0;
    EbAppContext         *app_callbacks[MAX_CHANNEL_NUMBER];   // Instances App callback data
    signal(SIGINT, EventHandler);

    // Get num_channels
    num_channels = get_number_of_channels(argc, argv);
    if (num_channels == 0) {
        return EB_ErrorBadParameter;
    }

    // Initialize config
    for (instance_count = 0; instance_count < num_channels; ++instance_count) {
        configs[instance_count] = (EbConfig*)malloc(sizeof(EbConfig));
        if (!configs[instance_count])
            return EB_ErrorInsufficientResources;
        eb_config_ctor(configs[instance_count]);
        return_errors[instance_count] = EB_ErrorNone;
    }

    // Initialize appCallback
    for (instance_count = 0; instance_count < num_channels; ++instance_count) {
        app_callbacks[instance_count] = (EbAppContext*)malloc(sizeof(EbAppContext));
        if (!app_callbacks[instance_count])
            return EB_ErrorInsufficientResources;
    }

    for (instance_count = 0; instance_count < MAX_CHANNEL_NUMBER; ++instance_count) {
        exit_conditions[instance_count] = APP_ExitConditionError;         // Processing loop exit condition
        exit_conditions_output[instance_count] = APP_ExitConditionError;         // Processing loop exit condition
        exit_conditions_recon[instance_count] = APP_ExitConditionError;         // Processing loop exit condition
        exit_conditions_input[instance_count] = APP_ExitConditionError;         // Processing loop exit condition
        channel_active[instance_count] = EB_FALSE;
    }

    // Read all configuration files.
    return_error = read_command_line(argc, argv, configs, num_channels, return_errors);

    // Process any command line options, including the configuration file
    if (return_error == EB_ErrorNone) {

        // Set main thread affinity
        if (configs[0]->target_socket != -1)
            AssignAppThreadGroup(configs[0]->target_socket);

        // Init the Encoder
        for (instance_count = 0; instance_count < num_channels; ++instance_count) {
            if (return_errors[instance_count] == EB_ErrorNone) {

                configs[instance_count]->active_channel_count = num_channels;
                configs[instance_count]->channel_id = instance_count;
                app_svt_vp9_get_time(
                    &configs[instance_count]
                         ->performance_context.lib_start_time[0],
                    &configs[instance_count]
                         ->performance_context.lib_start_time[1]);
                return_errors[instance_count] = init_encoder(configs[instance_count], app_callbacks[instance_count], instance_count);
                return_error = (EbErrorType)(return_error | return_errors[instance_count]);
            }
            else {
                channel_active[instance_count] = EB_FALSE;
            }
        }

        {
            // Start the Encoder
            for (instance_count = 0; instance_count < num_channels; ++instance_count) {
                if (return_errors[instance_count] == EB_ErrorNone) {
                    return_error = (EbErrorType)(return_error & return_errors[instance_count]);
                    exit_conditions[instance_count]       = APP_ExitConditionNone;
                    exit_conditions_output[instance_count] = APP_ExitConditionNone;
                    exit_conditions_recon[instance_count]  = configs[instance_count]->recon_file ? APP_ExitConditionNone : APP_ExitConditionError;
                    exit_conditions_input[instance_count]  = APP_ExitConditionNone;
                    channel_active[instance_count]        = EB_TRUE;
                    app_svt_vp9_get_time(
                        &configs[instance_count]
                             ->performance_context.encode_start_time[0],
                        &configs[instance_count]
                             ->performance_context.encode_start_time[1]);

                }
                else {
                    exit_conditions[instance_count]       = APP_ExitConditionError;
                    exit_conditions_output[instance_count] = APP_ExitConditionError;
                    exit_conditions_recon[instance_count]  = APP_ExitConditionError;
                    exit_conditions_input[instance_count]  = APP_ExitConditionError;
                }

#if DISPLAY_MEMORY
                EB_APP_MEMORY();
#endif
            }
            printf("Encoding          ");
            fflush(stdout);

            while (exit_condition == APP_ExitConditionNone) {
                exit_condition = APP_ExitConditionFinished;
                for (instance_count = 0; instance_count < num_channels; ++instance_count) {
                    if (channel_active[instance_count] == EB_TRUE) {
                        if (exit_conditions_input[instance_count] == APP_ExitConditionNone)
                            exit_conditions_input[instance_count] = process_input_buffer(
                                    configs[instance_count],
                                    app_callbacks[instance_count]);
                        if (exit_conditions_recon[instance_count] == APP_ExitConditionNone)
                            exit_conditions_recon[instance_count] = process_output_recon_buffer(
                                    configs[instance_count],
                                    app_callbacks[instance_count]);
                        if (exit_conditions_output[instance_count] == APP_ExitConditionNone)
                            exit_conditions_output[instance_count] = process_output_stream_buffer(
                                    configs[instance_count],
                                    app_callbacks[instance_count],
                                    (exit_conditions_input[instance_count] == APP_ExitConditionNone) || (exit_conditions_recon[instance_count] == APP_ExitConditionNone)? 0 : 1);
                        if (((exit_conditions_recon[instance_count] == APP_ExitConditionFinished || !configs[instance_count]->recon_file)  && exit_conditions_output[instance_count] == APP_ExitConditionFinished && exit_conditions_input[instance_count] == APP_ExitConditionFinished)||
                                ((exit_conditions_recon[instance_count] == APP_ExitConditionError && configs[instance_count]->recon_file) || exit_conditions_output[instance_count] == APP_ExitConditionError || exit_conditions_input[instance_count] == APP_ExitConditionError)){
                            channel_active[instance_count] = EB_FALSE;
                            if (configs[instance_count]->recon_file)
                                exit_conditions[instance_count] = (AppExitConditionType)(exit_conditions_recon[instance_count] | exit_conditions_output[instance_count] | exit_conditions_input[instance_count]);
                            else
                                exit_conditions[instance_count] = (AppExitConditionType)(exit_conditions_output[instance_count] | exit_conditions_input[instance_count]);
                        }
                    }
                }
                // check if all channels are inactive
                for (instance_count = 0; instance_count < num_channels; ++instance_count) {
                    if (channel_active[instance_count] == EB_TRUE)
                        exit_condition = APP_ExitConditionNone;
                }
            }

            for (instance_count = 0; instance_count < num_channels; ++instance_count) {
                if (exit_conditions[instance_count] == APP_ExitConditionFinished && return_errors[instance_count] == EB_ErrorNone) {
                    double frame_rate;

                    if ((configs[instance_count]->frame_rate_numerator != 0 && configs[instance_count]->frame_rate_denominator != 0) || configs[instance_count]->frame_rate != 0) {

                        if (configs[instance_count]->frame_rate_numerator && configs[instance_count]->frame_rate_denominator && (configs[instance_count]->frame_rate_numerator != 0 && configs[instance_count]->frame_rate_denominator != 0)) {
                            frame_rate = ((double)configs[instance_count]->frame_rate_numerator) / ((double)configs[instance_count]->frame_rate_denominator);
                        }
                        else if (configs[instance_count]->frame_rate > 1000) {
                            // Correct for 16-bit fixed-point fractional precision
                            frame_rate = ((double)configs[instance_count]->frame_rate) / (1 << 16);
                        }
                        else {
                            frame_rate = (double)configs[instance_count]->frame_rate;
                        }
                        printf("\nSUMMARY --------------------------------- Channel %u  --------------------------------\n", instance_count + 1);

                        printf("Total Frames\t\tFrame Rate\t\tByte Count\t\tBitrate\n");
                        printf("%12d\t\t%4.2f fps\t\t%10.0f\t\t%5.2f kbps\n",
                                (int32_t)configs[instance_count]->performance_context.frame_count,
                                (double)frame_rate,
                                (double)configs[instance_count]->performance_context.byte_count,
                                ((double)(configs[instance_count]->performance_context.byte_count << 3) * frame_rate / (configs[instance_count]->processed_frame_count * 1000)));
                        fflush(stdout);
                    }
                }
            }
            printf("\n");
            fflush(stdout);
        }
        for (instance_count = 0; instance_count < num_channels; ++instance_count) {
            if (exit_conditions[instance_count] == APP_ExitConditionFinished && return_errors[instance_count] == EB_ErrorNone) {

                if (configs[instance_count]->stop_encoder == EB_FALSE) {

                    printf("\nChannel %u\nAverage Speed:\t\t%.2f fps\nTotal Encoding Time:\t%.0f ms\nTotal Execution Time:\t%.0f ms\nAverage Latency:\t%.0f ms\nMax Latency:\t\t%u ms\n",
                            (uint32_t)(instance_count + 1),
                            configs[instance_count]->performance_context.average_speed,
                            configs[instance_count]->performance_context.total_encode_time * 1000,
                            configs[instance_count]->performance_context.total_execution_time * 1000,
                            configs[instance_count]->performance_context.average_latency,
                            (uint32_t)(configs[instance_count]->performance_context.max_latency));

                }
                else {
                    printf("\nChannel %u Encoding Interrupted\n", (uint32_t)(instance_count + 1));
                }
            }
            else if (return_errors[instance_count] == EB_ErrorInsufficientResources) {
                printf("Could not allocate enough memory for channel %u\n", instance_count + 1);
            }
            else {
                printf("Error encoding at channel %u! Check error log file for more details ... \n", instance_count + 1);
            }
        }

        // DeInit Encoder
        for (instance_count = num_channels; instance_count > 0; --instance_count) {
            if (return_errors[instance_count - 1] == EB_ErrorNone)
                return_errors[instance_count - 1] = de_init_encoder(app_callbacks[instance_count - 1], instance_count - 1);
        }
    }
    else {
        printf("Error in configuration, could not begin encoding! ... \n");
        printf("Run %s --help for a list of options\n", argv[0]);
    }
    // Destruct the App memory variables
    for (instance_count = 0; instance_count < num_channels; ++instance_count) {
        eb_config_dtor(configs[instance_count]);
        if (configs[instance_count])
            free(configs[instance_count]);
        if (app_callbacks[instance_count])
            free(app_callbacks[instance_count]);
    }

    printf("Encoder finished\n");

    return (return_error == 0) ? 0 : 1;
}
