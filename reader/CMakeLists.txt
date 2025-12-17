cmake_minimum_required(VERSION 3.20)
project(SmartBookReader VERSION 1.0.0 LANGUAGES CXX)

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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngine WebChannel Sql)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find common library
find_library(SMARTBOOK_COMMON_LIB
    NAMES smartbook_common
    PATHS ${CMAKE_BINARY_DIR}/../common/lib
          ${CMAKE_BINARY_DIR}/../../common/lib
          /usr/local/lib
)

# Source files
set(READER_SOURCES
    src/main.cpp
    src/LibraryManager.cpp
    src/ReaderViewWindow.cpp
    src/WebChannelBridge.cpp
    src/ui/LibraryView.cpp
    src/ui/ReaderView.cpp
    src/ui/ConsentDialog.cpp
)

# Header files
set(READER_HEADERS
    include/smartbook/reader/LibraryManager.h
    include/smartbook/reader/ReaderViewWindow.h
    include/smartbook/reader/WebChannelBridge.h
    include/smartbook/reader/ui/LibraryView.h
    include/smartbook/reader/ui/ReaderView.h
    include/smartbook/reader/ui/ConsentDialog.h
)

# Create executable
add_executable(smartbook-reader
    ${READER_SOURCES}
    ${READER_HEADERS}
)

# Include directories
target_include_directories(smartbook-reader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

# Link libraries
target_link_libraries(smartbook-reader PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngine
    Qt6::WebChannel
    Qt6::Sql
    ${SMARTBOOK_COMMON_LIB}
)

# Platform-specific settings
if(APPLE)
    set_target_properties(smartbook-reader PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER com.smartbook.reader
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

# Installation
install(TARGETS smartbook-reader
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
