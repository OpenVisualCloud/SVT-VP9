# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# APP Directory CMakeLists.txt
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

# Include Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

file(GLOB all_files
    "*.h"
    "../API/*.h"
    "../../Config/*.cfg"
    "*.c"
    "*.asm"
)


# App Source Files
add_executable (SvtVp9EncApp
    ${all_files}
)

#********** SET COMPILE FLAGS************

if (UNIX)

    # Link the Encoder App
    target_link_libraries (SvtVp9EncApp
                SvtVp9Enc
                pthread
                rt 
                m) # math library
endif()

if (MSVC OR MSYS OR MINGW OR WIN32)    
    # Link the Encoder App
    target_link_libraries (SvtVp9EncApp
                           SvtVp9Enc)
endif()

install(TARGETS SvtVp9EncApp RUNTIME DESTINATION bin)