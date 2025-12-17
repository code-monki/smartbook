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
set(CREATOR_SOURCES
    src/main.cpp
    src/CreatorMainWindow.cpp
    src/ContentEditor.cpp
    src/FormBuilder.cpp
    src/CartridgeExporter.cpp
    src/ui/MainWindow.cpp
    src/ui/ContentEditorView.cpp
    src/ui/FormBuilderDialog.cpp
)

# Header files
set(CREATOR_HEADERS
    include/smartbook/creator/CreatorMainWindow.h
    include/smartbook/creator/ContentEditor.h
    include/smartbook/creator/FormBuilder.h
    include/smartbook/creator/CartridgeExporter.h
    include/smartbook/creator/ui/MainWindow.h
    include/smartbook/creator/ui/ContentEditorView.h
    include/smartbook/creator/ui/FormBuilderDialog.h
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
    ${SMARTBOOK_COMMON_LIB}
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
