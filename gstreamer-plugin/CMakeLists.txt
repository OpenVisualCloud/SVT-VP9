cmake_minimum_required(VERSION 2.8)
project(gst-svt-vp9 C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

FIND_PACKAGE(PkgConfig REQUIRED)
pkg_check_modules (GSTREAMER REQUIRED gstreamer-1.0>=1.8)
include_directories (${GSTREAMER_INCLUDE_DIRS})

pkg_check_modules (GSTREAMER_BASE REQUIRED gstreamer-base-1.0>=1.8)
include_directories (${GSTREAMER_BASE_INCLUDE_DIRS})

pkg_check_modules (GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.8)
include_directories (${GSTREAMER_VIDEO_INCLUDE_DIRS})

pkg_check_modules (SVT_VP9 REQUIRED SvtVp9Enc)
include_directories (${SVT_VP9_INCLUDE_DIRS})

set(CMAKE_C_FLAGS "-fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -O2 -flto -Wall -Wextra -Wformat -Wformat-security")
set(CMAKE_SHARED_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")

add_library(gstsvtvp9enc SHARED gstsvtvp9enc.c)
target_link_libraries(gstsvtvp9enc
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${SVT_VP9_LIBRARIES})

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${GSTREAMER_LIBDIR}")
    message("Install to: " ${GSTREAMER_LIBDIR})
endif()

install (TARGETS gstsvtvp9enc
    LIBRARY DESTINATION gstreamer-1.0)

message("Build: " ${CMAKE_BUILD_TYPE})

