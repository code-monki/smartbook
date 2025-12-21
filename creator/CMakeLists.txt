cmake_minimum_required(VERSION 3.20)
project(SmartBookCreator VERSION 1.0.0 LANGUAGES CXX)

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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebChannel Sql)
# Qt6WebEngine is a meta-package; find individual components if meta-package not available
find_package(Qt6 COMPONENTS WebEngine QUIET)
if(NOT Qt6WebEngine_FOUND)
    # Fallback to individual WebEngine components
    find_package(Qt6 REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)
    # Create alias for compatibility
    if(TARGET Qt6::WebEngineCore AND TARGET Qt6::WebEngineWidgets)
        add_library(Qt6::WebEngine INTERFACE IMPORTED)
        target_link_libraries(Qt6::WebEngine INTERFACE Qt6::WebEngineCore Qt6::WebEngineWidgets)
    endif()
endif()

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Common library is built by top-level CMakeLists.txt
# Just link to the target directly

# Source files
set(CREATOR_SOURCES
    src/main.cpp
    src/CreatorMainWindow.cpp
    src/ContentEditor.cpp
    src/PageManager.cpp
    src/FormBuilder.cpp
    src/FormManager.cpp
    src/CartridgeExporter.cpp
    # src/ui/MainWindow.cpp  # TODO: Implement MainWindow UI
    # src/ui/ContentEditorView.cpp  # TODO: Implement ContentEditorView
    src/ui/FormBuilderDialog.cpp
    src/ui/PageListView.cpp
)

# Header files
set(CREATOR_HEADERS
    include/smartbook/creator/CreatorMainWindow.h
    include/smartbook/creator/ContentEditor.h
    include/smartbook/creator/PageManager.h
    include/smartbook/creator/FormBuilder.h
    include/smartbook/creator/FormManager.h
    include/smartbook/creator/CartridgeExporter.h
    include/smartbook/creator/ui/MainWindow.h
    include/smartbook/creator/ui/ContentEditorView.h
    include/smartbook/creator/ui/FormBuilderDialog.h
    include/smartbook/creator/ui/PageListView.h
)

# Create executable
add_executable(smartbook-creator
    ${CREATOR_SOURCES}
    ${CREATOR_HEADERS}
)

# Include directories
target_include_directories(smartbook-creator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

# Link libraries
target_link_libraries(smartbook-creator PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngine
    Qt6::WebChannel
    Qt6::Sql
    smartbook_common
)

# Platform-specific settings
if(APPLE)
    set_target_properties(smartbook-creator PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER com.smartbook.creator
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

# Installation
install(TARGETS smartbook-creator
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
