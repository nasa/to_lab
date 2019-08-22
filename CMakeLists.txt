cmake_minimum_required(VERSION 2.6.4)
project(CFE_TO_LAB C)

add_definitions(-D_LINUX_OS_)
include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(../../cfe/fsw/cfe-core/src/inc)
include_directories(../../osal/src/os/inc)
include_directories(../../osal/build/inc)
include_directories(../../build/editor_inc)
include_directories(../../psp/fsw/inc)
include_directories(../../build/tools/elf2cfetbl/inc_stubs)
include_directories(../ci_lab/fsw/platform_inc)
include_directories(../to_lab/fsw/platform_inc)
include_directories(../sample_app/fsw/platform_inc)


aux_source_directory(fsw/src APP_SRC_FILES)

include(../../cfe/cmake/arch_build.cmake)

# Create the app module
add_cfe_app(to_lab ${APP_SRC_FILES})

