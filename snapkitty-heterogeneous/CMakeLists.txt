# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# snapkitty-heterogeneous/CMakeLists.txt

cmake_minimum_required(VERSION 3.24)

project(SnapKittyHeterogeneous
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# The AArch64 gate is a warning, not fatal: no shipped runtime source is
# AArch64-specific (no __aarch__/__x86_64__ guards, no NEON/SVE intrinsics,
# no inline asm, no AArch64-only headers). The intended host per
# config/targets.toml ([host] architecture = "aarch64") is still recorded
# in the warning below; the build proceeds on the native host.
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    message(WARNING "SnapKitty host target per config/targets.toml is AArch64, but building on ${CMAKE_SYSTEM_PROCESSOR}; shipped runtime sources are portable (no AArch64-specific code), so proceeding with a native host build")
endif()

find_package(Threads REQUIRED)

# Runtime sources: 14 listed, 5 shipped. The 9 below were never shipped
# and are not stubbed; they stay in the list so the intended target stays
# visible, and are added to the library only when the file actually exists
# on disk. Each absent file produces a configure-time WARNING. If the files
# are ever dropped, they are picked up automatically with no edit here.
#   MISSING (not shipped, skipped): runtime/device/device_context.c,
#   runtime/device/device_capabilities.c, runtime/memory/buffer.c,
#   runtime/memory/allocation.c, runtime/memory/transfer.c,
#   runtime/events/event.c, runtime/events/synchronization.c,
#   runtime/scheduler/dispatch.c, runtime/scheduler/workload.c
#   PRESENT (built): runtime/discovery/linux_discovery.c,
#   runtime/discovery/nvidia_discovery.c, runtime/discovery/amd_discovery.c,
#   runtime/discovery/target_registry.c, runtime/device/device.c
# No shipped source calls any symbol from the missing files: the public
# API in runtime/include/sk_device.h (sk_discover_devices,
# sk_initialize_device, sk_shutdown_device) is fully implemented by
# runtime/device/device.c, and both executables link only against it.
set(SK_RUNTIME_SOURCES
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

set(SK_RUNTIME_EXISTING_SOURCES "")
foreach(sk_runtime_source IN LISTS SK_RUNTIME_SOURCES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${sk_runtime_source}")
        list(APPEND SK_RUNTIME_EXISTING_SOURCES "${sk_runtime_source}")
    else()
        message(WARNING "SnapKitty runtime source listed but not shipped, skipped: ${sk_runtime_source}")
    endif()
endforeach()

add_library(sk_runtime STATIC ${SK_RUNTIME_EXISTING_SOURCES})

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
