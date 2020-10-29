# ~~~
# Copyright(c) 2019 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# ~~~

cmake_minimum_required(VERSION 2.8)
set(project_name "svt-vp9")

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    message(WARNING "Building in-source is highly not recommended\n"
                    "Please use the Build folder or create your own.")
endif()

option(YASM "Use yasm (if present in PATH)" ON)
if(YASM)
    find_program(YASM_EXE yasm)
    if(YASM_EXE AND NOT CMAKE_ASM_NASM_COMPILER MATCHES "yasm")
        set(CMAKE_ASM_NASM_COMPILER ${YASM_EXE})
        message(STATUS "Found YASM: ${YASM_EXE}")
    endif()
endif()

project(${project_name} C ASM_NASM)

if(NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(GNUInstallDirs)
include(CheckCCompilerFlag)

if(CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_ASM_NASM_COMPILER = ${CMAKE_ASM_NASM_COMPILER}")
if(CMAKE_AR)
    message(STATUS "CMAKE_AR = ${CMAKE_AR}")
endif()
if(CMAKE_RANLIB)
    message(STATUS "CMAKE_RANLIB = ${CMAKE_RANLIB}")
endif()

if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "")
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "..." FORCE)
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

set(SVT_VP9_VERSION_MAJOR "0")
set(SVT_VP9_VERSION_MINOR "3")
set(SVT_VP9_VERSION_PATCHLEVEL "0")

configure_file(${PROJECT_SOURCE_DIR}/Source/API/EbApiVersion.h.in ${PROJECT_SOURCE_DIR}/Source/API/EbApiVersion.h @ONLY)

if(NOT DEFINED SVT_VP9_VERSION OR SVT_VP9_VERSION STREQUAL "")
    set(
        SVT_VP9_VERSION
        "${SVT_VP9_VERSION_MAJOR}.${SVT_VP9_VERSION_MINOR}.${SVT_VP9_VERSION_PATCHLEVEL}"
        )
endif()
set(SVT_VP9_SOVERSION 1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})

if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin")
endif()
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()
if(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
    set(CMAKE_INSTALL_INCLUDEDIR "include")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(CMAKE_VERSION VERSION_LESS "3.1")
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS "--std=gnu99 ${CMAKE_C_FLAGS}")
    endif()
else()
    set(CMAKE_C_STANDARD 99)
endif()
set(CAN_USE_ASSEMBLER TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DWIN64")
else()
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DUNIX64")
endif()

if(UNIX)
    if(APPLE)
        set(CMAKE_MACOSX_RPATH 1)
        set(CMAKE_C_ARCHIVE_CREATE
            "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_CXX_ARCHIVE_CREATE
            "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_C_ARCHIVE_FINISH
            "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
        set(CMAKE_CXX_ARCHIVE_FINISH
            "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    else()
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -z noexecstack -z relro -z now -pie ")
    endif()
endif()

set(flags_to_test
    -Wextra
    -Wformat
    -Wformat-security
    -fPIE
    -fPIC
    -D_FORTIFY_SOURCE=2
    #-flto
    -fvisibility=hidden
    /GS
    /sdl)
if(MSVC)
    list(APPEND flags_to_test /MP)
else()
    option(NATIVE "Build for native performance (march=native)")
    list(INSERT flags_to_test 0 -Wall)
    # for Mingw64 support, see
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65782
    if(MINGW)
        list(APPEND flags_to_test -fno-asynchronous-unwind-tables -mxsave)
    else()
        list(APPEND flags_to_test -fstack-protector-strong)
    endif()
    if(NATIVE)
        list(APPEND flags_to_test -march=native)
    endif()
endif()
set(release_flags_to_test)
set(debug_flags_to_test)

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} /NXCompat /DynamicBase")
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} /NXCompat /DynamicBase")
endif()

macro(test_apply_compiler_flags)
    foreach(cflag ${ARGV})
        string(REGEX
               REPLACE "[^A-Za-z0-9]"
                       "_"
                       cflag_var
                       "${cflag}")
        set(test_c_flag "C_FLAG${cflag_var}")
        check_c_compiler_flag(${cflag} "${test_c_flag}")
        if(${test_c_flag})
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
        endif()
    endforeach()
    if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
        message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: ${CMAKE_C_FLAGS}")
    endif()
endmacro()

test_apply_compiler_flags(${flags_to_test})

foreach(cflag ${release_flags_to_test})
    string(REGEX
           REPLACE "[^A-Za-z0-9]"
                   "_"
                   cflag_var
                   "${cflag}")
    set(test_c_flag "C_FLAG${cflag_var}")
    check_c_compiler_flag(${cflag} "${test_c_flag}")
    if(${test_c_flag})
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${cflag}")
    endif()
endforeach()
foreach(cflag ${debug_flags_to_test})
    string(REGEX
           REPLACE "[^A-Za-z0-9]"
                   "_"
                   cflag_var
                   "${cflag}")
    set(test_c_flag "C_FLAG${cflag_var}")
    check_c_compiler_flag(${cflag} "${test_c_flag}")
    if(${test_c_flag})
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${cflag}")
    endif()
endforeach()

if(CMAKE_ASM_NASM_OBJECT_FORMAT MATCHES "win")
    set(CMAKE_ASM_NASM_FLAGS_DEBUG "${CMAKE_ASM_NASM_FLAGS_DEBUG} -gcv8")
elseif(CMAKE_ASM_NASM_OBJECT_FORMAT MATCHES "macho")
    if(CMAKE_ASM_NASM_COMPILER MATCHES "nasm")
        set(CMAKE_ASM_NASM_FLAGS_DEBUG "${CMAKE_ASM_NASM_FLAGS_DEBUG} -gdwarf")
    elseif(CMAKE_ASM_NASM_COMPILER MATCHES "yasm")
        set(CMAKE_ASM_NASM_FLAGS_DEBUG "${CMAKE_ASM_NASM_FLAGS_DEBUG} -gnull")
    endif()
else()
    if(CMAKE_ASM_NASM_COMPILER MATCHES "nasm")
        set(CMAKE_ASM_NASM_FLAGS_DEBUG "${CMAKE_ASM_NASM_FLAGS_DEBUG} -gdwarf")
    elseif(CMAKE_ASM_NASM_COMPILER MATCHES "yasm")
        set(CMAKE_ASM_NASM_FLAGS_DEBUG "${CMAKE_ASM_NASM_FLAGS_DEBUG} -gdwarf2")
    endif()
endif()

# BUILD_SHARED_LIBS is a standard CMake variable, but we declare it here to make
# it prominent in the GUI.
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." ON)
option(COVERAGE "Generate coverage report")
option(BUILD_APP "Build Enc App" ON)

# Prepare for Coveralls
if(COVERAGE AND NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Add Subdirectories
add_subdirectory(Source/Lib/VPX)
add_subdirectory(Source/Lib/Codec)
add_subdirectory(Source/Lib/C_DEFAULT)
add_subdirectory(Source/Lib/ASM_SSE2)
add_subdirectory(Source/Lib/ASM_SSSE3)
add_subdirectory(Source/Lib/ASM_SSE4_1)
add_subdirectory(Source/Lib/ASM_AVX2)
if(BUILD_APP)
    add_subdirectory(Source/App)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/Source/API/
        DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/svt-vp9"
        FILES_MATCHING
        PATTERN "*.h")
