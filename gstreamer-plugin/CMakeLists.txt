cmake_minimum_required(VERSION 2.8)

if(NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(gst-svt-vp9 C)

set(POSITION_INDEPENDENT_CODE ON)

include(CheckCCompilerFlag)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.8)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0>=1.8)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.8)
pkg_check_modules(SVT_VP9 REQUIRED SvtVp9Enc)
include_directories(${GSTREAMER_INCLUDE_DIRS}
                    ${GSTREAMER_BASE_INCLUDE_DIRS}
                    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
                    ${SVT_VP9_INCLUDE_DIRS})

set(flags_to_test
    -Wextra
    -Wformat
    -Wformat-security
    -fstack-protector-strong
    -fPIE
    -fPIC
    -flto
    -D_FORTIFY_SOURCE=2)
if(MSVC)
    list(APPEND flags_to_test /MP)
else()
    list(INSERT flags_to_test 0 -Wall)
endif()

foreach(flag ${flags_to_test})
    string(REGEX
           REPLACE "[^A-Za-z0-9]"
                   "_"
                   flag_var
                   "${flag}")
    set(test_c_flag "C_FLAG${flag_var}")
    check_c_compiler_flag(${flag} "${test_c_flag}")
    if(${test_c_flag})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
    endif()
endforeach()

if(UNIX AND NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -z noexecstack -z relro -z now -pie")
endif()

add_library(gstsvtvp9enc SHARED gstsvtvp9enc.c)
target_link_libraries(gstsvtvp9enc
                      ${GSTREAMER_LIBRARIES}
                      ${GSTREAMER_BASE_LIBRARIES}
                      ${GSTREAMER_VIDEO_LIBRARIES}
                      ${SVT_VP9_LIBRARIES})

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${GSTREAMER_LIBDIR}")
    message("Install to: " ${GSTREAMER_LIBDIR})
endif()

install(TARGETS gstsvtvp9enc LIBRARY DESTINATION gstreamer-1.0)

message(STATUS "Build: " ${CMAKE_BUILD_TYPE})
