cmake_minimum_required(VERSION 3.20)
project(SmartBook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable all warnings and treat warnings as errors
# Warnings are code smells that indicate potential defects
if(MSVC)
    # MSVC: Enable level 4 warnings and treat as errors
    add_compile_options(/W4 /WX)
else()
    # GCC/Clang: Enable all warnings and treat as errors
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add subdirectories
add_subdirectory(common)
add_subdirectory(reader)
add_subdirectory(creator)
add_subdirectory(test)

# Tools (optional utilities)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
    add_subdirectory(tools)
endif()

# PoC applications (optional, for development/testing)
option(BUILD_POC "Build Proof of Concept applications" OFF)
if(BUILD_POC)
    add_subdirectory(poc)
endif()

# Phase 2: Server (commented out for now)
# add_subdirectory(server)
