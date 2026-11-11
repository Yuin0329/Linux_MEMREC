cmake_minimum_required(VERSION 3.20)

project(linxray VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "linxray targets Linux. On Windows, build inside WSL2 Ubuntu.")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core: reusable system information collectors and their public interfaces.
add_library(linxray_core src/memory.cpp)
target_include_directories(linxray_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(linxray_core PUBLIC cxx_std_20)

# App: command-line argument handling and presentation.
add_executable(linxray app/main.cpp)
target_link_libraries(linxray PRIVATE linxray_core)

# Tests: core behavior and complete CLI behavior share the same core library.
include(CTest)
if(BUILD_TESTING)
    add_executable(linxray_memory_tests tests/unit/memory_tests.cpp)
    target_link_libraries(linxray_memory_tests PRIVATE linxray_core)

    add_test(NAME memory.parser COMMAND linxray_memory_tests)
    add_test(
        NAME memory.cli
        COMMAND "${CMAKE_COMMAND}"
            "-DLINXRAY=$<TARGET_FILE:linxray>"
            "-DFIXTURE_DIR=${PROJECT_SOURCE_DIR}/tests/fixtures"
            -P "${PROJECT_SOURCE_DIR}/tests/cli/memory_cli.cmake"
    )
endif()

# Apply the same warnings to every C++ target.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target linxray_core linxray linxray_memory_tests)
        if(TARGET ${target})
            target_compile_options(${target} PRIVATE
                -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            )
        endif()
    endforeach()
endif()
