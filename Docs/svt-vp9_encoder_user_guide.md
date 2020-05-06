
# Scalable Video Technology for VP9 Encoder (SVT-VP9 Encoder) User Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Sample Application Guide](#sample-application-guide)
    - 2.1 [Input Video Format](#input-video-format)
    - 2.2 [Running the encoder](#running-the-encoder)
        - 2.2.1 [List of all configuration parameters](#list-of-all-configuration-parameters)
3. [Best Known Configuration (BKC)](#best-known-configuration-bkc)
    - 3.1 [Hardware BKC](#hardware-bkc)
    - 3.2 [Software BKC](#software-bkc)
5. [Legal Disclaimer](#legal-disclaimer)

## Introduction

This document describes how to use the Scalable Video Technology for VP9 Encoder (SVT-VP9).  In particular, this user guide describes how to run the sample application with the respective dynamically linked library.

## Sample Application Guide

This section describes how to run the sample encoder application that uses the SVT-VP9 Encoder library.  It describes the input video format, the command line input parameters and the resulting outputs.

### Input Video Format

The SVT-VP9 Encoder supports the following input format:

8-bit yuv420p:

 ![alt](8bit_yuv420p.png)

### Running the encoder

This section describes how to run the sample encoder application <u>SvtVp9EncApp.exe</u> (on Windows\*) or <u>SvtVp9EncApp</u> (on Linux\*) from the command line, including descriptions of the most commonly used input parameters and outputs.

The sample application typically takes the following command line parameters:

>-c filename [**Optional**]

A text file that contains encoder parameters such as input file name, quantization parameter etc. Refer to the comments in the Config/Sample.cfg for specific details. The list of encoder parameters are also listed below. Note that command line parameters take precedence over the parameters included in the configuration file when there is a conflict.

>-i filename **[Required]**

A YUV file (e.g. 8 bit 4:2:0 planar) containing the video sequence that will be encoded.  The dimensions of each image are specified by -w and -h as indicated below.

>-b filename **[Optional]**

The resulting encoded bit stream file in binary format. If none specified, no output bit stream will be produced by the encoder.

>-w integer **[Required]**

The width of each input image in units of picture luma pixels,  e.g. 1920

>-h integer **[Required]**]

The height of each input image in units of picture luma pixels,  e.g. 1080

>-n integer **[Optional]**

The number of frames of the sequence to encode.  e.g. 100. If the input frame count is larger than the number of frames in the input video, the encoder will loopback to the first frame when it is done.

>-intra-period integer **[Optional]**

The intra period defines the interval of frames after which you insert an Intra refresh. It is strongly recommended to use (multiple of 8) -1 the closest to 1 second (e.g. 55, 47, 31, 23 should be used for 60, 50, 30, (24 or 25) respectively). Because all intra refreshes are closed gop add 1 to the value above (e.g. 56 instead of 55).

>-rc integer **[Optional]**

This token sets the bitrate control encoding mode [1: Variable Bitrate, 0: Constant QP]. When rc is set to 1, it is best to match the -lad (lookahead distance described in the next section) parameter to the -intra-period. When -rc is set to 0, a qp value is expected with the use of the -q command line option otherwise a default value is assigned (45).

For example, the following command encodes 100 frames of the YUV video sequence into the bin bit stream file.  The picture is 1920 luma pixels wide and 1080 pixels high using the <u>Sample.cfg</u> configuration. The QP equals 30 and the md5 checksum is not included in the bit stream.

>SvtVp9EncApp.exe -i CrowdRun\_1920x1080.yuv -w 1920 -h 1080 -n 100 -fps 50 -b output.ivf

It should be noted that not all the encoder parameters present in the <u>Sample.cfg</u> can be changed using the command line.

#### List of all configuration parameters

The encoder parameters present in the Sample.cfg file are listed in this table below along with their status of support, command line parameter and the range of values that the parameters can take.

| **Configuration file parameter** | **Command line** |   **Range**   | **Default** | **Description** |
| --- | --- | --- | --- | --- |
| **ChannelNumber** | -nch | [1 - 6] | 1 | Number of encode instances |
| **ConfigFile** | -c | any string | null | Configuration file path |
| **InputFile** | -i | any string | None | Input file path |
| **StreamFile** | -b | any string | null | output bitstream file path |
| **ErrorFile** | -errlog | any string | stderr | error log displaying configuration or encode errors |
| **ReconFile** | -o | any string | null | Recon file path. Optional output of recon. |
| **UseQpFile** | -use-q-file | [0 - 1] | 0 | When set to 1, overwrite the picture qp assignment using qp values in QpFile |
| **QpFile** | -qp-file | any string | null | Path to qp file |
| **EncoderMode** | -enc-mode | [0 - 9] | 9 | A preset defining the quality vs density tradeoff point that the encoding is to be performed at. (e.g. 0 is the highest quality mode, 9 is the highest density mode).|
| **Tune** | -tune | [0 - 2] | 1 | 0 = SQ - visually optimized mode, <br>1 = OQ - PSNR / SSIM optimized mode,<br>2 = VMAF - VMAF optimized mode |
| **EncoderBitDepth** | -bit-depth | [8] | 8 | specifies the bit depth of the input video |
| **SourceWidth** | -w | [64 - 8192] | 0 | Input source width |
| **SourceHeight** | -h | [64 - 4320] | 0 | Input source height |
| **FrameToBeEncoded** | -n | [0 - 2^31-1] | 0 | Number of frames to be encoded, if number of frames is > number of frames in file, the encoder will loop to the beginning and continue the encode. 0 encodes the full clip. |
| **BufferedInput** | -nb | [-1, 1 to 2^31-1] | -1 | number of frames to preload to the RAM before the start of the encode. If -nb = 100 and -n 1000 --> the encoder will encode the first 100 frames of the video 10 timesUse -1 to not preload any frames. |
| **FrameRate** | -fps | [0 - 2^64-1] | 25 | If the number is less than 1000, the input frame rate is an integer number between 1 and 60, else the input number is in Q16 format (shifted by 16 bits) [Max allowed is 240 fps]. If FrameRateNumerator and FrameRateDenominator are both !=0 the encoder will ignore this parameter |
| **FrameRateNumerator** | -fps-num | [0 - 2^64-1] | 0 | Frame rate numerator e.g. 6000. When zero, the encoder will use -fps if FrameRateDenominator is also zero, otherwise an error is returned |
| **FrameRateDenominator** | -fps-denom | [0 - 2^64-1] | 0 | Frame rate denominator e.g. 100. When zero, the encoder will use -fps if FrameRateNumerator is also zero, otherwise an error is returned |
| **Injector** | -inj | [0 - 1] | 0 | Enable injection of input frames at the specified framerate (0 = OFF, 1 = ON) |
| **InjectorFrameRate** | -inj-frm-rt | [1 - 240] | 60 | Frame Rate used for the injector. Recommended to match the encoder speed. |
| **SpeedControlFlag** | -speed-ctrl | [0 - 1] | 0 | Enables the Speed Control functionality to achieve the real-time encoding speed defined by -fps. When this parameter is set to 1 it forces -inj to be 1 -inj-frm-rt to be set to the -fps. |
| **BaseLayerSwitchMode** | -base-layer-switch-mode | [0 - 1] | 0 | 0 = Use B-frames in the base layer pointing to the same past picture. <br>1 = Use P-frames in the base layer|
| **PredStructure** | -pred-struct | [2] | 2 | 2 = Random Access.|
| **IntraPeriod** | -intra-period | [-2 - 255] | -2 | Distance Between Intra Frame inserted. -1 denotes no intra update. -2 denotes auto. |
| **QP** | -q | [0 - 63] | 50 | Initial quantization parameter for the Intra pictures used when RateControlMode 0 (CQP) |
| **LoopFilter** | -loop-filter | [0 - 1] | 1 | Enables or disables the loop filter, <br>0 = OFF, 1 = ON |
| **UseDefaultMeHme** | -use-default-me-hme | [0 - 1] | 1 | 0 = Overwrite Default ME HME parameters. <br>1 = Use default ME HME parameters, dependent on width and height |
| **HME** | -hme | [0 - 1] | 1 | Enable HME, 0 = OFF, 1 = ON |
| **SearchAreaWidth** | -search-w | [1 - 256] | Depends on input resolution | Search Area in Width |
| **SearchAreaHeight** | -search-h | [1 - 256] | Depends on input resolution | Search Area in Height |
| **RateControlMode** | -rc | [0 - 2] | 0 | 0 = CQP , 1 = VBR , 2 = CBR|
| **TargetBitRate** | -tbr | Any Number | 7000000 | Target bitrate in bits / second. Only used when RateControlMode is set to 1 |
| **vbvMaxrate** | -vbv-maxrate | Any Number | 0 | VBVMaxrate in bits / second. Only used when RateControlMode is set to 1 |
| **vbvBufsize** | -vbv-bufsize | Any Number | 0 | VBV BufferSize in bits / second. Only used when RateControlMode is set to 1 |
| **MaxQpAllowed** | -max-qp | [0 - 63] | 63 | Maximum QP value allowed for rate control use. Only used when RateControlMode is set to 1. Has to be > MinQpAllowed |
| **MinQpAllowed** | -min-qp | [0 - 63] | 10 | Minimum QP value allowed for rate control use. Only used when RateControlMode is set to 1. Has to be < MaxQpAllowed |
| **AsmType** | -asm | [0 - 1] | 1 | Assembly instruction set (0 = C Only, 1 = Automatically select highest assembly instruction set supported) |
| **LogicalProcessorNumber** | -lp | [0, total number of logical processor] | 0 | The number of logical processor which encoder threads run on.Refer to Appendix A.1 |
| **TargetSocket** | -ss | [-1,1] | -1 | For dual socket systems, this can specify which socket the encoder runs on.Refer to Appendix A.1 |
| **SwitchThreadsToRtPriority** | -rt | [0 - 1] | 1 | Enables or disables threads to real time priority, 0 = OFF, 1 = ON (only works on Linux) |
| **Profile** | -profile | [0] | 0 | 0 = 8-bit 4:2:0 |
| **Level** | -level | [1, 2, 2.1,3, 3.1, 4, 4.1, 5, 5.1, 5.2, 6, 6.1, 6.2] | 0 | 0 to 6.2 [0 for auto determine Level] |

## Best Known Configuration (BKC)

This section outlines the best known hardware and software configurations that would allow the SVT-VP9 Encoder to run with the highest computational performance. For the CQP mode, the output bit stream will not change if these BKCs have not been applied.

### Hardware BKC

The SVT-VP9 Encoder is optimized for use on Xeon® Scalable Processors products. For best multichannel encode, servers should be set up with at least one 2666 Mhz DDR4 RAM DIMM per RAM channel per socket. For example, a dual Xeon Platinum 8180 server is best set up with 12 x 2666 Mhz DDR4 RAM DIMM.

### Software BKC

#### Windows* OS (Tested on Windows* Server 2016)

Visual Studio* 2017 offers Profile Guided Optimization (PGO) to improve compiler optimization for the application. The tool uses an instrumented build to generate a set of profile information of the most frequently used code and optimal paths. The profile is then used to provide extra information for the compiler to optimize the application. To take advantage of PGO, build using the following:

1. Open the solution file with Visual Studio* 2017 and build code in Release mode
2. Right click SvtVp9EncApp project from the Solution Explorer -> Profile Guided Optimization -> Instrument (Repeat for SvtVp9Enc)
3. Right click SvtVp9EncApp project from the Solution Explorer -> Properties -> Debugging
4. Add configuration parameters and run encoder (e.g. 1280x720 video encode of 300 frames)
5. Right click SvtVp9EncApp project from the Solution Explorer -> Profile Guided Optimization -> Run Instrumented/Optimized Application
6. Right click SvtVp9EncApp project from the Solution Explorer -> Profile Guided Optimization -> Optimize (Repeat for SvtVp9Enc)

#### Linux* OS (Tested on Ubuntu* Server 18.04 and 16.04)

Some Linux\* Operating systems and kernels assign CPU utilization limits to applications running on servers. Therefore, to allow the application to utilize up to ~100% of the CPUs assigned to it, it is best to run the following commands before and when running the encoder:
  > sudo  sysctl  -w  kernel.sched\_rt\_runtime\_us=1000000
  - this command should be executed every time the server is rebooted

The above section is not needed for Windows\* as it does not perform the CPU utilization limitation on the application.

#### Command Line BKC
The SVT-VP9 encoder achieves the best performance when restricting each channel to only one socket on either Windows\* or Linux\* operating systems. For example, when running four channels on a dual socket system, it&#39;s best to pin two channels to each socket and not split every channel on both sockets.

LogicalProcessorNumber (-lp) and TargetSocket (-ss) parameters can be used to management the threads. Or you can use OS commands like below.

For example, in order to run a 2-stream 4kp60 simultaneous encode on a dual socket Xeon Gold 6140 system the following command lines should be used:

##### *Running Windows** Server 2016:

>start /node 0 SvtVp9EncApp.exe -enc-mode 8 -tune 0  -w 3840  -h 2160 -bit-depth 8 -i in.yuv  -rc 1 -tbr 10000000 -fps 60  -b out1.bin   -n 5000

>start /node 1 SvtVp9EncApp.exe -enc-mode 8 -tune 0  -w 3840  -h 2160 -bit-depth 8 -i in.yuv  -rc 1 -tbr 10000000 -fps 60 -b out3.bin   -n 5000

##### *Running Ubuntu** 18.04:

>taskset 0x00003FFFF00003FFFF ./SvtVp9EncApp -enc-mode 8 -tune 0  -w 3840  -h 2160 -bit-depth 8 -i in.yuv  -rc 1 -tbr 10000000 -fps 60 -b out1.bin   -n 5000 &amp;

>taskset 0xFFFFC0000FFFFC0000 ./SvtVp9EncApp -enc-mode 8 -tune 0  -w 3840  -h 2160 -bit-depth 8 -i in.yuv  -rc 1 -tbr 10000000 -fps 60 -b out3.bin   -n 5000  &amp;

<br>
Where 0xFFFFC0000FFFFC0000 and 0x00003FFFF00003FFFF are masks for sockets 0 and 1 respectively on a dual 6140 system.

## Appendix A Encoder Parameters
### 1. Thread management parameters

LogicalProcessorNumber (-lp) and TargetSocket (-ss) parameters are used to management thread affinity on Windows and Ubuntu OS. These are some examples how you use them together.

If LogicalProcessorNumber and TargetSocket are not set, threads are managed by OS thread scheduler.

>SvtVp9EncApp.exe -i in.yuv -w 3840 -h 2160 –lp 40
If only LogicalProcessorNumber is set, threads run on 40 logical processors. Threads may run on dual sockets if 40 is larger than logical processor number of a socket.

NOTE: On Windows, thread affinity can be set only by group on system with more than 64 logical processors. So, if 40 is larger than logical processor number of a single socket, threads run on all logical processors of both sockets.

>SvtVp9EncApp.exe -i in.yuv -w 3840 -h 2160 –ss 1
If only TargetSocket is set, threads run on all the logical processors of socket 1.

>SvtVp9EncApp.exe -i in.yuv -w 3840 -h 2160 –lp 20 –ss 0
If both LogicalProcessorNumber and TargetSocket are set, threads run on 20 logical processors of socket 0. Threads guaranteed to run only on socket 0 if 20 is larger than logical processor number of socket 0.

## Legal Disclaimer

Optimization Notice: Intel compilers may or may not optimize to the same degree for non-Intel microprocessors for optimizations that are not unique to Intel microprocessors. These optimizations include SSE2, SSE3, and SSSE3 instruction sets and other optimizations. Intel does not guarantee the availability, functionality, or effectiveness of any optimization on microprocessors not manufactured by Intel. Microprocessor-dependent optimizations in this product are intended for use with Intel microprocessors. Certain optimizations not specific to Intel microarchitecture are reserved for Intel microprocessors. Please refer to the applicable product User and Reference Guides for more information regarding the specific instruction sets covered by this notice.

Notice Revision #20110804

Intel technologies features and benefits depend on system configuration and may require enabled hardware, software or service activation. Performance varies depending on system configuration. No computer system can be absolutely secure. Check with your system manufacturer or retailer.

No license (express or implied, by estoppel or otherwise) to any intellectual property rights is granted by this document.

Intel disclaims all express and implied warranties, including without limitation, the implied warranties of merchantability, fitness for a particular purpose, and non-infringement, as well as any warranty arising from course of performance, course of dealing, or usage in trade.

The products and services described may contain defects or errors known as errata which may cause deviations from published specifications. Current characterized errata are available on request. No product or component can be absolutely secure.

This document contains information on products, services and/or processes in development.  All information provided here is subject to change without notice. Contact your Intel representative to obtain the latest forecast, schedule, specifications and roadmaps.

Intel, Intel Xeon, Intel Core, the Intel logo and others are trademarks of Intel Corporation and its subsidiaries in the U.S. and/or other countries.

\*Other names and brands may be claimed as the property of others.

© Intel Corporation
