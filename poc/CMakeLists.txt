cmake_minimum_required(VERSION 3.20)
project(SmartBookPoC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick QuickWidgets)

# PoC 1: QTextBrowser Rendering
add_executable(poc_qtextbrowser_rendering
    poc_qtextbrowser_rendering.cpp
)
set_target_properties(poc_qtextbrowser_rendering PROPERTIES AUTOMOC ON)
target_link_libraries(poc_qtextbrowser_rendering PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# PoC 2: QML Embedding
add_executable(poc_qml_embedding
    poc_qml_embedding.cpp
)
set_target_properties(poc_qml_embedding PROPERTIES AUTOMOC ON)
target_link_libraries(poc_qml_embedding PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickWidgets
)

