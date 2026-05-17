cmake_minimum_required(VERSION 3.20)

project(
    XRayDoctorAssist
    VERSION 0.1.0
    DESCRIPTION "Offline AI-Assisted Chest X-Ray Analysis System"
    LANGUAGES C
)

# -------------------------------------------------------------------
# Global Configuration
# -------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------

find_package(PkgConfig REQUIRED)
pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

set(ONNXRUNTIME_ROOT ${PROJECT_SOURCE_DIR}/third_party/onnxruntime/onnxruntime-linux-x64-1.18.1)
set(ONNXRUNTIME_INCLUDE_DIR ${ONNXRUNTIME_ROOT}/include)
set(ONNXRUNTIME_LIBRARY ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so)


# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# -------------------------------------------------------------------
# Compiler Warnings
# -------------------------------------------------------------------

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wdouble-promotion
        -Wformat=2
        -Wundef
        -Werror
    )
endif()

# -------------------------------------------------------------------
# Include Directories
# -------------------------------------------------------------------

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# -------------------------------------------------------------------
# CLI Executable
# -------------------------------------------------------------------

add_executable(
    xray-cli
    src/cli/main.c
    src/engine/xray_engine.c
    src/engine/image_loader.c
    src/engine/preprocessor.c
    src/engine/preprocessor_validation.c
    src/engine/model_runner.c
    src/engine/model_output_validation.c
    src/engine/rule_engine.c
    src/export/result_formatter.c
    src/db/database.c
    src/utils/xray_status.c
    src/utils/xray_logger.c
)

# -------------------------------------------------------------------
# Output Directories
# -------------------------------------------------------------------

target_include_directories(
    xray-cli
    PRIVATE
    ${GDK_PIXBUF_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${ONNXRUNTIME_INCLUDE_DIR}
)

target_link_libraries(
    xray-cli
    PRIVATE
    ${GDK_PIXBUF_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${ONNXRUNTIME_LIBRARY}
    m
)

target_compile_options(
    xray-cli
    PRIVATE
    ${GDK_PIXBUF_CFLAGS_OTHER}
)

set_target_properties(
    xray-cli
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build
)

# -------------------------------------------------------------------
# Build Summary
# -------------------------------------------------------------------

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")

# -------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------

add_executable(
    test-rule-engine-invalid-probability
    tests/test_rule_engine_invalid_probability.c
    src/engine/rule_engine.c
    src/utils/xray_status.c
    src/utils/xray_logger.c
)

target_include_directories(
    test-rule-engine-invalid-probability
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    test-rule-engine-invalid-probability
    PRIVATE
    m
)

set_target_properties(
    test-rule-engine-invalid-probability
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build
)

# -------------------------------------------------------------------
# GTK3 Desktop GUI
# -------------------------------------------------------------------

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(
    xray-gui
    src/gui/main_gui.c
    src/ui/main_window.c
    src/engine/xray_engine.c
    src/engine/image_loader.c
    src/engine/preprocessor.c
    src/engine/preprocessor_validation.c
    src/engine/model_runner.c
    src/engine/model_output_validation.c
    src/engine/rule_engine.c
    src/export/result_formatter.c
    src/db/database.c
    src/utils/xray_status.c
    src/utils/xray_logger.c
)

target_include_directories(
    xray-gui
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${GTK3_INCLUDE_DIRS}
    ${GDK_PIXBUF_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${ONNXRUNTIME_INCLUDE_DIR}
)

target_link_libraries(
    xray-gui
    PRIVATE
    ${GTK3_LIBRARIES}
    ${GDK_PIXBUF_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${ONNXRUNTIME_LIBRARY}
    m
)

target_compile_options(
    xray-gui
    PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${GDK_PIXBUF_CFLAGS_OTHER}
)

set_target_properties(
    xray-gui
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build
)
