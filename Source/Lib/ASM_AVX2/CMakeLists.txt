# ~~~
# Copyright(c) 2019 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# ~~~

# ASM_AVX2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API
                    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec
                    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2
                    ${PROJECT_SOURCE_DIR}/Source/Lib/VPX)

set(flags_to_test -mavx2)

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    if(WIN32)
        # Intel Windows (*Note - The Warning level /W0 should be made to /W4 at
        # some point)
        list(APPEND flags_to_test /Qdiag-disable:10010,10148,10157 /W0)
    else()
        list(APPEND flags_to_test -static-intel -w)
    endif()
endif()

if(MSVC)
    list(APPEND flags_to_test /arch:AVX2)
endif()

test_apply_compiler_flags(${flags_to_test})

file(GLOB
     all_files
     "*.h"
     "*.c"
     "*.asm")

add_library(VP9_ASM_AVX2 OBJECT ${all_files})
