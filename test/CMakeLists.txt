cmake_minimum_required(VERSION 3.20)
project(SmartBookTests VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find Qt6
# Try custom Qt path first (~/Qt on Unix, C:/Qt on Windows), then system default
if(APPLE)
    # macOS: ~/Qt/6.x.x/macos
    if(EXISTS "$ENV{HOME}/Qt")
        file(GLOB QT_PATHS "$ENV{HOME}/Qt/6.*/macos")
        list(SORT QT_PATHS)
        list(REVERSE QT_PATHS)
        list(GET QT_PATHS 0 QT_PREFIX)
        if(QT_PREFIX)
            set(CMAKE_PREFIX_PATH ${QT_PREFIX} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
elseif(WIN32)
    # Windows: C:/Qt/6.x.x/msvc2019_64 or C:/Qt/6.x.x/mingw*_64
    set(QT_SEARCH_PATHS
        "$ENV{USERPROFILE}/Qt"
        "C:/Qt"
    )
    foreach(QT_ROOT ${QT_SEARCH_PATHS})
        if(EXISTS "${QT_ROOT}")
            file(GLOB MSVC_PATHS "${QT_ROOT}/6.*/msvc*_64")
            if(MSVC_PATHS)
                list(SORT MSVC_PATHS)
                list(REVERSE MSVC_PATHS)
                list(GET MSVC_PATHS 0 QT_PREFIX)
                set(CMAKE_PREFIX_PATH ${QT_PREFIX} ${CMAKE_PREFIX_PATH})
                break()
            endif()
            file(GLOB MINGW_PATHS "${QT_ROOT}/6.*/mingw*_64")
            if(MINGW_PATHS)
                list(SORT MINGW_PATHS)
                list(REVERSE MINGW_PATHS)
                list(GET MINGW_PATHS 0 QT_PREFIX)
                set(CMAKE_PREFIX_PATH ${QT_PREFIX} ${CMAKE_PREFIX_PATH})
                break()
            endif()
        endif()
    endforeach()
else()
    # Linux: ~/Qt/6.x.x/gcc_64 or ~/Qt/6.x.x/clang_64
    if(EXISTS "$ENV{HOME}/Qt")
        file(GLOB QT_PATHS "$ENV{HOME}/Qt/6.*/gcc_64" "$ENV{HOME}/Qt/6.*/clang_64")
        if(QT_PATHS)
            list(SORT QT_PATHS)
            list(REVERSE QT_PATHS)
            list(GET QT_PATHS 0 QT_PREFIX)
            set(CMAKE_PREFIX_PATH ${QT_PREFIX} ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()

# Find Qt6 Test
find_package(Qt6 COMPONENTS Test)
if(Qt6Test_FOUND)
    message(STATUS "Qt6 Test found - unit tests enabled")
    set(QT_TEST_AVAILABLE TRUE)
else()
    message(WARNING "Qt6 Test not found - unit tests disabled")
    set(QT_TEST_AVAILABLE FALSE)
endif()

# Find common library
find_library(SMARTBOOK_COMMON_LIB
    NAMES smartbook_common
    PATHS ${CMAKE_BINARY_DIR}/../common/lib
          ${CMAKE_BINARY_DIR}/../../common/lib
          /usr/local/lib
)

# Unit tests
if(QT_TEST_AVAILABLE)
    enable_testing()
    
    # Common library unit tests
    add_executable(test_common_unit
        unit/test_localdbmanager.cpp
    )
    target_include_directories(test_common_unit PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_common_unit PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        ${SMARTBOOK_COMMON_LIB}
    )
    add_test(NAME CommonUnitTests COMMAND test_common_unit)
endif()

# Integration tests
# add_executable(test_integration
#     integration/test_cartridge_loading.cpp
# )
# target_link_libraries(test_integration PRIVATE
#     Qt6::Core
#     ${SMARTBOOK_COMMON_LIB}
# )
