# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
cmake_minimum_required(VERSION 3.16)
project(native_execution C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(sk_native STATIC
    src/abi.c src/value.c src/memory.c src/kernel.c
    src/dispatch.c src/context.c src/contract.c)
target_include_directories(sk_native PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(sk_native PRIVATE /W4)
else()
  target_compile_options(sk_native PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

enable_testing()
foreach(test_name abi memory kernel dispatch)
  add_executable(test_${test_name} tests/test_${test_name}.c)
  target_link_libraries(test_${test_name} PRIVATE sk_native)
  add_test(NAME ${test_name} COMMAND test_${test_name})
endforeach()

add_executable(native_probe examples/native_probe.c)
target_link_libraries(native_probe PRIVATE sk_native)
