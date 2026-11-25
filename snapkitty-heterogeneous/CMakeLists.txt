# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
/* snapkitty-heterogeneous/CMakeLists.txt */

cmake_minimum_required(VERSION 3.24)

project(SnapKittyHeterogeneous
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    message(FATAL_ERROR "SnapKitty ARM64 host build requires AArch64")
endif()

find_package(Threads REQUIRED)

add_library(sk_runtime STATIC
    runtime/discovery/linux_discovery.c
    runtime/discovery/nvidia_discovery.c
    runtime/discovery/amd_discovery.c
    runtime/discovery/target_registry.c
    runtime/device/device.c
    runtime/device/device_context.c
    runtime/device/device_capabilities.c
    runtime/memory/buffer.c
    runtime/memory/allocation.c
    runtime/memory/transfer.c
    runtime/events/event.c
    runtime/events/synchronization.c
    runtime/scheduler/dispatch.c
    runtime/scheduler/workload.c
)

target_include_directories(sk_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
)

target_compile_options(sk_runtime PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    -Wstrict-prototypes
)

target_link_libraries(sk_runtime PRIVATE
    ${CMAKE_DL_LIBS}
    Threads::Threads
)

add_executable(snapkitty-host runtime/main.c)
target_link_libraries(snapkitty-host PRIVATE sk_runtime)

enable_testing()

add_executable(sk-runtime-tests tests/unit/runtime/test_runtime.c)
target_link_libraries(sk-runtime-tests PRIVATE sk_runtime)

add_test(NAME runtime_tests COMMAND sk-runtime-tests)
