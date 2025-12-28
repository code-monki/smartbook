# ============================================================================
# SmartBook Tools CMakeLists.txt
# ============================================================================
#
# Build configuration for optional build utilities and tools.
#
# Tools:
#   - create_test_cartridge: Utility to create test cartridges for development
#     and testing purposes
#
# Dependencies:
#   - Qt6: Core, Sql (minimal dependencies for tools)
#
# Usage:
#   These tools are built automatically when tools/ directory is included.
#   They are optional and not required for the main application builds.
#
# See Documentation/build-process.adoc for more information.
# ============================================================================

cmake_minimum_required(VERSION 3.20)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# create_test_cartridge utility
# Creates test cartridges with sample content for development and testing
add_executable(create_test_cartridge
    create_test_cartridge.cpp
)

target_include_directories(create_test_cartridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

target_link_libraries(create_test_cartridge PRIVATE
    Qt6::Core
    Qt6::Sql
)

