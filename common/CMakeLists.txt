cmake_minimum_required(VERSION 3.20)
project(SmartBookCommon VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(APPLE)
    set(PLATFORM_OS "macos")
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        set(PLATFORM_ARCH "arm64")
    else()
        set(PLATFORM_ARCH "x86_64")
    endif()
elseif(UNIX)
    set(PLATFORM_OS "linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(PLATFORM_ARCH "arm64")
    else()
        set(PLATFORM_ARCH "x86_64")
    endif()
elseif(WIN32)
    set(PLATFORM_OS "windows")
    set(PLATFORM_ARCH "x86_64")
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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
    # Try both %USERPROFILE%\Qt and C:\Qt
    set(QT_SEARCH_PATHS
        "$ENV{USERPROFILE}/Qt"
        "C:/Qt"
    )
    foreach(QT_ROOT ${QT_SEARCH_PATHS})
        if(EXISTS "${QT_ROOT}")
            # Look for MSVC first (most common on Windows)
            file(GLOB MSVC_PATHS "${QT_ROOT}/6.*/msvc*_64")
            if(MSVC_PATHS)
                list(SORT MSVC_PATHS)
                list(REVERSE MSVC_PATHS)
                list(GET MSVC_PATHS 0 QT_PREFIX)
                set(CMAKE_PREFIX_PATH ${QT_PREFIX} ${CMAKE_PREFIX_PATH})
                break()
            endif()
            # Fallback to MinGW
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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Sql)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(COMMON_SOURCES
    src/database/LocalDBManager.cpp
    src/database/CartridgeDBConnector.cpp
    src/security/SignatureVerifier.cpp
    src/security/TrustRegistry.cpp
    src/utils/PlatformUtils.cpp
    src/utils/PathUtils.cpp
    src/metadata/MetadataExtractor.cpp
    src/manifest/ManifestManager.cpp
)

# Header files
set(COMMON_HEADERS
    include/smartbook/common/database/LocalDBManager.h
    include/smartbook/common/database/CartridgeDBConnector.h
    include/smartbook/common/security/SignatureVerifier.h
    include/smartbook/common/security/TrustRegistry.h
    include/smartbook/common/utils/PlatformUtils.h
    include/smartbook/common/utils/PathUtils.h
    include/smartbook/common/metadata/MetadataExtractor.h
    include/smartbook/common/manifest/ManifestManager.h
)

# Create shared library
add_library(smartbook_common SHARED
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

# Include directories
target_include_directories(smartbook_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link Qt libraries
target_link_libraries(smartbook_common PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
)

# Platform-specific settings
if(APPLE)
    set_target_properties(smartbook_common PROPERTIES
        MACOSX_RPATH ON
        INSTALL_NAME_DIR "@rpath"
    )
elseif(UNIX)
    set_target_properties(smartbook_common PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Installation
install(TARGETS smartbook_common
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/smartbook
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
