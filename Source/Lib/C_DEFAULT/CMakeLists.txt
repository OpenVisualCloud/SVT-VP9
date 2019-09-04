# ~~~
# Copyright(c) 2019 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# ~~~

# C_DEFAULT Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API
                    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec
                    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT
                    ${PROJECT_SOURCE_DIR}/Source/Lib/VPX)

file(GLOB
     all_files
     "*.h"
     "*.c")

# Utility Source Files
add_library(VP9_C_DEFAULT OBJECT ${all_files})
