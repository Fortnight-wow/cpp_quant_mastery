# =============================================================================
# cpp-quant-mastery — Root CMake Configuration
# =============================================================================
# Modern C++20 quantitative finance learning repository.
#
# Design:
#   - Each module directory (01_Basics … 08_Trading_Systems_Simulation) owns
#     its own CMakeLists.txt and registers one executable per lesson .cpp file.
#   - Subdirectories are added only when their CMakeLists.txt exists, so the
#     project configures cleanly while modules are generated incrementally.
#
# Usage:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ./build/07_Derivative_Pricing_Models/black_scholes   # example target path
# =============================================================================

cmake_minimum_required(VERSION 3.20)

project(
    cpp_quant_mastery
    VERSION 1.0.0
    DESCRIPTION "C++20 learning path: basics to quantitative finance and low-latency systems"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Global C++20 standard (required for concepts, designated init, etc.)
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release when no build type is specified (better for numerics demos)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo)" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set — defaulting to Release")
endif()

# -----------------------------------------------------------------------------
# Compiler warnings (treat as errors optional — off for learning repo)
# -----------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Wno-unused-parameter)
    endif()
endif()

# Link math library on Unix (some lesson targets may use <cmath> heavily)
if(UNIX AND NOT APPLE)
    link_libraries(m)
endif()

# -----------------------------------------------------------------------------
# Module registry — order reflects curriculum progression
# -----------------------------------------------------------------------------
set(CPP_QUANT_MODULES
    01_Basics
    02_Memory_and_Pointers
    03_Object_Oriented_Programming
    04_Standard_Template_Library
    05_Advanced_CPP
    06_Numerical_Methods_and_Linear_Algebra
    07_Derivative_Pricing_Models
    08_Trading_Systems_Simulation
)

set(CPP_QUANT_ACTIVE_MODULES "")

foreach(MODULE IN LISTS CPP_QUANT_MODULES)
    set(MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}")
    set(MODULE_CMAKE "${MODULE_DIR}/CMakeLists.txt")

    if(EXISTS "${MODULE_CMAKE}")
        add_subdirectory("${MODULE}")
        list(APPEND CPP_QUANT_ACTIVE_MODULES "${MODULE}")
        message(STATUS "Module enabled: ${MODULE}")
    else()
        message(STATUS "Module pending (no CMakeLists.txt yet): ${MODULE}")
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Summary target — lists all registered lesson executables
# -----------------------------------------------------------------------------
if(CPP_QUANT_ACTIVE_MODULES)
    message(STATUS "Active modules: ${CPP_QUANT_ACTIVE_MODULES}")
else()
    message(STATUS "No modules built yet — generate Module 01 to begin.")
endif()

# Optional: aggregate target once modules define their executables
# Each module CMakeLists should append to CPP_QUANT_LESSON_TARGETS.
set(CPP_QUANT_LESSON_TARGETS "" CACHE INTERNAL "All lesson executable targets")

if(CPP_QUANT_LESSON_TARGETS)
    add_custom_target(lessons ALL
        DEPENDS ${CPP_QUANT_LESSON_TARGETS}
        COMMENT "Building all cpp-quant-mastery lesson executables"
    )
endif()
