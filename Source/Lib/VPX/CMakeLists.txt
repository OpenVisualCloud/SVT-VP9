# ~~~
# Copyright(c) 2019 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# ~~~

# VPX Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API
                    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3
                    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT
                    ${PROJECT_SOURCE_DIR}/Source/Lib/VPX)

file(GLOB
     all_vpx_files
     "*.h"
     "*.c")

# VPX Source Files
add_library(libVPX OBJECT ${all_vpx_files})
