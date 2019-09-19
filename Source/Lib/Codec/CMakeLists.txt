# ~~~
# Copyright(c) 2019 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# ~~~

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API
                    ${PROJECT_SOURCE_DIR}/Source/Lib/VPX
                    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec
                    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1
                    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})

file(GLOB
     all_files
     "*.h"
     "*.c")

# Encoder Lib Source Files
add_library(SvtVp9Enc
            $<TARGET_OBJECTS:VP9_ASM_AVX2>
            $<TARGET_OBJECTS:VP9_ASM_SSE2>
            $<TARGET_OBJECTS:VP9_ASM_SSE4_1>
            $<TARGET_OBJECTS:VP9_ASM_SSSE3>
            $<TARGET_OBJECTS:VP9_C_DEFAULT>
            $<TARGET_OBJECTS:libVPX>
            ${all_files})

if(UNIX)
    set(LIBS "-lpthread ${LIBS}")
    if(NOT APPLE)
        list(APPEND PLATFORM_LIBS m)
        set(LIBS "-lm ${LIBS}")
    endif()

    # Pass version script to ensure that only global-namespace symbols are
    # exported.
    if(APPLE)
        set(VERSION_SCRIPT ${PROJECT_SOURCE_DIR}/Source/API/EbExport_list)
        set(
            CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} -exported_symbols_list=${VERSION_SCRIPT}"
            )
    else()
        set(VERSION_SCRIPT ${PROJECT_SOURCE_DIR}/Source/API/EbApi.version)
        set(
            CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script=${VERSION_SCRIPT}"
            )
    endif()
    set_target_properties(SvtVp9Enc PROPERTIES LINK_DEPENDS ${VERSION_SCRIPT})
endif()

target_link_libraries(SvtVp9Enc ${PLATFORM_LIBS})

if(COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
        set(LIBS "-lgcov ${LIBS}")
    else(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(LIBS "--coverage ${LIBS}")
    endif()
endif()

add_dependencies(SvtVp9Enc
                 libVPX
                 VP9_C_DEFAULT
                 VP9_ASM_SSE2
                 VP9_ASM_SSSE3
                 VP9_ASM_SSE4_1
                 VP9_ASM_AVX2)

# Set version number for SONAME.
set_target_properties(SvtVp9Enc PROPERTIES SOVERSION ${SVT_VP9_SOVERSION})
configure_file(../pkg-config.pc.in ${CMAKE_BINARY_DIR}/SvtVp9Enc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/SvtVp9Enc.pc
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(TARGETS SvtVp9Enc DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS SvtVp9Enc
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
