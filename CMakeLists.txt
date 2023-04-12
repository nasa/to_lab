cmake_minimum_required(VERSION 3.5)
project(CFS_TO_LAB C)

# These references are specifically needed for the table build
# it is easiest to add them as directory properties so they won't
# be considered include directories for TO_LAB itself.
include_directories(
    $<TARGET_PROPERTY:ci_lab,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:sample_app,INTERFACE_INCLUDE_DIRECTORIES>
)

set(APP_SRC_FILES
    fsw/src/to_lab_app.c
)

# Create the app module
add_cfe_app(to_lab ${APP_SRC_FILES})
add_cfe_tables(to_lab fsw/tables/to_lab_sub.c)

target_include_directories(to_lab PUBLIC
    fsw/mission_inc
    fsw/platform_inc
)
