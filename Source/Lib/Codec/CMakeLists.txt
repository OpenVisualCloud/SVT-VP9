# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/VPX/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/VPX/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")   
endif(MSVC)

file(GLOB all_files
    "*.h"
    "*.c"
)
 
# Encoder Lib Source Files
add_library(SvtVp9Enc SHARED
    ${all_files}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Set version number for SONAME.
    set_target_properties(SvtVp9Enc PROPERTIES SOVERSION ${SVT_VP9_SOVERSION})

    # Pass version script to ensure that only global-namespace symbols
    # are exported.
    set(VERSION_SCRIPT ${PROJECT_SOURCE_DIR}/Source/API/EbApi.version)
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script=${VERSION_SCRIPT}")
    set_target_properties(SvtVp9Enc PROPERTIES LINK_DEPENDS ${VERSION_SCRIPT})
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_link_libraries(SvtVp9Enc
    VPX
    C_DEFAULT
    ASM_SSE2
    ASM_SSSE3
    ASM_SSE4_1
    ASM_AVX2
    m)
else()
target_link_libraries(SvtVp9Enc
    VPX
    C_DEFAULT
    ASM_SSE2
    ASM_SSSE3
    ASM_SSE4_1
    ASM_AVX2)
    
endif()

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()

if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
    set(CMAKE_INSTALL_INCLUDEDIR "include/svt-vp9")
endif()

configure_file(../pkg-config.pc.in ${CMAKE_BINARY_DIR}/SvtVp9Enc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/SvtVp9Enc.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(TARGETS SvtVp9Enc DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY ../../API/ DESTINATION "${CMAKE_INSTALL_PREFIX}/include/svt-vp9" FILES_MATCHING PATTERN "*.h")
