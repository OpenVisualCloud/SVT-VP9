# ~~~
# Copyright(c) 2019 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# ~~~

# APP Directory CMakeLists.txt

# Include Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API
                    ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE})

file(GLOB
     all_files
     "*.h"
     "../API/*.h"
     "../../Config/*.cfg"
     "*.c"
     "*.asm")

# App Source Files
add_executable(SvtVp9EncApp ${all_files})

# ********** SET COMPILE FLAGS************

# Link the Encoder App
target_link_libraries(SvtVp9EncApp SvtVp9Enc)

if(UNIX)
    target_link_libraries(SvtVp9EncApp pthread)
    if(NOT APPLE)
        target_link_libraries(SvtVp9EncApp rt m) # math library
    endif()
endif()

install(TARGETS SvtVp9EncApp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
