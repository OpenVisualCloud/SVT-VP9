# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_AVX2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/VPX/)

link_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
    if(CMAKE_C_COMPILER_VERSION VERSION_EQUAL 5.4)
        string(REPLACE "-flto" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    endif()
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /arch:AVX2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /arch:AVX2")   
endif(MSVC)

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -static-intel /W0 /Qwd10148 /Qwd10010 /Qwd10157")
endif()

file(GLOB all_files
    "*.h"
    "*.c"
    "*.asm"
)

add_library(${SVT_COMMON_LIBRARY_PREFIX}ASM_AVX2 STATIC
       ${all_files}
)



