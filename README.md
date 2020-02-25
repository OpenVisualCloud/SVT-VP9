# Scalable Video Technology for VP9 Encoder (SVT-VP9 Encoder)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/OpenVisualCloud/SVT-VP9?branch=master&svg=true)](https://ci.appveyor.com/project/OpenVisualCloud/SVT-VP9)
[![Travis Build Status](https://travis-ci.org/OpenVisualCloud/SVT-VP9.svg?branch=master)](https://travis-ci.org/OpenVisualCloud/SVT-VP9)
[![Coverage Status](https://coveralls.io/repos/github/OpenVisualCloud/SVT-VP9/badge.svg?branch=master)](https://coveralls.io/github/OpenVisualCloud/SVT-VP9?branch=master)

The Scalable Video Technology for VP9 Encoder (SVT-VP9 Encoder) is a VP9-compliant encoder library core. The SVT-VP9 Encoder development is a work-in-progress targeting performance levels applicable to both VOD and Live encoding/transcoding video applications.

The SVT-VP9 Encoder is being optimized to achieve excellent performance levels currently supporting 10 density-quality presets (please refer to the user guide for more details) on a system with a dual Intel® Xeon® Scalable processor targeting:

- Real-time encoding of up to two 4Kp60 streams on the Gold 6140 with M8.

SVT-VP9 Encoder also supports 3 modes:

- A visually optimized mode for visual quality (-tune 0)

- An PSNR/SSIM optimized mode for PSNR / SSIM benchmarking (-tune 1 (Default setting))

- An VMAF optimized mode for VMAF benchmarking (-tune 2)

## License

SVT-VP9 Encoder is licensed under the OSI-approved BSD+Patent license. See [LICENSE](LICENSE.md) for details.

## Documentation

More details about the SVT-VP9 Encoder usage can be found under:
-   [svt-vp9-encoder-user-guide](Docs/svt-vp9_encoder_user_guide.md)

## System Requirements

### Operating System

SVT-VP9 Encoder may run on any Windows* or Linux* 64 bit operating systems. The list below represents the operating systems that the encoder application and library were tested and validated on:

* __Windows* Operating Systems (64-bit):__

    -  Windows* Server 2016

* __Linux* Operating Systems (64-bit):__

    -  Ubuntu* 16.04 Server LTS

    -  Ubuntu* 18.04 Server LTS

### Hardware

The SVT-VP9 Encoder library supports the x86 architecture

* __CPU Requirements__

In order to achieve the performance targeted by the SVT-VP9 Encoder, the specific CPU model listed above would need to be used when running the encoder. Otherwise, the encoder runs on any 5th Generation Intel® Core™ processor, (Intel® Xeon® CPUs, E5-v4 or newer).

* __RAM Requirements__

In order to run the highest resolution supported by the SVT-VP9 Encoder, at least 10GB of RAM is required to run a 4k 8bit stream multi-threading on an 8180 system. The SVT-VP9 Encoder application will display an error if the system does not have enough RAM to support this. The following table shows the minimum amount of RAM required for some standard resolutions of 8bit video per stream:

|        Resolution         | Minimum Footprint (GB)|
|-----------------------|-----------------------|
|        4k                 |           10             |
|        1080p             |            4          |
|        720p             |            3          |
|        480p             |            2          |

## Build and Install

### Windows* Operating Systems (64-bit):

- __Build Requirements__
  - Visual Studio* 2017 (download [here](https://www.visualstudio.com/vs/older-downloads/)) or 2019 (download [here](https://visualstudio.microsoft.com/downloads/))
  - CMake 3.14 or later (download [here](https://github.com/Kitware/CMake/releases/download/v3.14.4/cmake-3.14.4-win64-x64.msi))
  - YASM Assembler version 1.2.0 or later
    - Download the yasm exe from the following [link](http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe)
    - Rename yasm-1.3.0-win64.exe to yasm.exe
    - Copy yasm.exe into a location that is in the PATH environment variable

* __Build Instructions__
    -    Generate the Visual Studio* 2017 project files by following the steps below cd Build\windows
        -    run <u>generate_vs17.bat</u> [such would generate the visual studio project files]
    -    Open the "<u>svt-vp9.sln</u>" using Visual Studio* 2017 and click on Build -- > Build Solution

* __Binaries Location__
    -   Binaries can be found under <repo dir>\Bin/Release or <repo dir>\Bin/Debug, depending on whether Debug or Release were selected in the build mode

* __Installation__
-    For the binaries to operate properly on your system, the following conditions have to be met:
    -    On any of the Windows* Operating Systems listed in the OS requirements section, install Visual Studio* 2017
    -    Once the installation is complete, copy the binaries to a location making sure that both the sample application "<u>SvtVp9EncApp.exe</u>” and library "<u>SvtVp9Enc.dll</u>” are in the same folder.
    -    Open the command prompt window at the chosen location and run the sample application to encode.
        > SvtVp9EncApp.exe -i [in.yuv] -w [width] -h [height] -b [out.ivf].
    -    Sample application supports reading from pipe. E.g:
        > ffmpeg -i [input.mp4] -nostdin -f rawvideo -pix_fmt yuv420p - | SvtVp9EncApp.exe -i stdin -n [number_of_frames_to_encode] -w [width] -h [height].

### Linux* Operating Systems (64-bit):

* __Build Requirements__
     -    GCC 5.4.0 or later
     -    CMake 3.5.1 or later
     -    YASM Assembler version 1.2.0 or later

* __Build Instructions__
     - `./Build/linux/build.sh <release | debug>` (if none specified, both release and debug will be built)
     - To build a static library and binary, append `static`
     - Additional options can be found by typing `./Build/linux/build.sh --help`

* __Sample Binaries location__
     -    Binaries can be found under Bin/Release and / or Bin/Debug

* __Installation__
For the binaries to operate properly on your system, the following conditions have to be met:
    -    On any of the Linux* Operating Systems listed above, copy the binaries under a location of your choice.
    -    Change the permissions on the sample application “<u>SvtVp9EncApp</u>” executable by running the         command:
        >chmod +x SvtVp9EncApp
    -    cd into your chosen location
    -    Run the sample application to encode.
        >    ./SvtVp9EncApp -i [in.yuv] -w [width] -h [height] -b [out.ivf].
    -    Sample application supports reading from pipe. E.g:
        >ffmpeg -i [input.mp4] -nostdin -f rawvideo -pix_fmt yuv420p - | ./SvtVp9EncApp -i stdin -n [number_of_frames_to_encode] -w [width] -h [height].

## How to evaluate by ready-to-run executables with docker

Refer to the guide [here](https://github.com/OpenVisualCloud/Dockerfiles/blob/master/doc/svt.md#Evaluate-SVT).

## Demo features and limitations

-  **Multi-instance support:** The multi-instance functionality is a demo feature implemented in the SVT-VP9 Encoder sample application as an example of one sample application using multiple encoding libraries. Encoding using the multi-instance support is limited to only 6 simultaneous streams. For example two channels encoding on Windows: SvtVp9EncApp.exe -nch 2 -c firstchannel.cfg secondchannel.cfg
-  **Features enabled:** The library will display an error message any feature combination that is not currently supported.

## How to Contribute

We welcome community contributions to the SVT-VP9 Encoder. Thank you for your time! By contributing to the project, you agree to the license and copyright terms in the OSI-approved BSD+Patent license and to the release of your contribution under these terms. See [LICENSE](LICENSE.md) for details.

### Contribution process

-  Follow the [coding guidelines](STYLE.md)

-  Validate that your changes do not break a build

-  Perform smoke tests and ensure they pass

-  Submit a pull request for review to the maintainer

### How to Report Bugs and Provide Feedback

Use the [Issues](https://github.com/OpenVisualCloud/SVT-VP9/issues) tab on Github. To avoid duplicate issues, please make sure you go through the existing issues before logging a new one.

## IRC

`#svt` on Freenode. Join via [Freenode Webchat](https://webchat.freenode.net/?channels=svt) or use your favorite IRC client.

## Notices and Disclaimers

The notices and disclaimers can be found [here](NOTICES.md)
