cmake_minimum_required(VERSION 3.20)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# create_test_cartridge utility
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

