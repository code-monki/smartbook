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

# Find Qt6 Test and Sql
find_package(Qt6 COMPONENTS Test Sql Gui Widgets Quick QuickWidgets Qml Network)
if(Qt6Test_FOUND AND Qt6Sql_FOUND)
    message(STATUS "Qt6 Test and Sql found - unit tests enabled")
    set(QT_TEST_AVAILABLE TRUE)
else()
    message(WARNING "Qt6 Test or Sql not found - unit tests disabled")
    set(QT_TEST_AVAILABLE FALSE)
endif()

# Find common library - use target from common subdirectory
# The common library is built as a target, so we reference it directly
# We need to ensure common is built first

# Unit tests
if(QT_TEST_AVAILABLE)
    enable_testing()
    
    # Common library unit tests - separate executables for each test file
    # test_localdbmanager
    add_executable(test_localdbmanager
        unit/test_localdbmanager.cpp
    )
    set_target_properties(test_localdbmanager PROPERTIES AUTOMOC ON)
    target_include_directories(test_localdbmanager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_localdbmanager PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestLocalDBManager COMMAND test_localdbmanager)
    
    # test_manifestmanager
    add_executable(test_manifestmanager
        unit/test_manifestmanager.cpp
    )
    set_target_properties(test_manifestmanager PROPERTIES AUTOMOC ON)
    target_include_directories(test_manifestmanager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_manifestmanager PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestManifestManager COMMAND test_manifestmanager)
    
    # test_cartridgedbconnector
    add_executable(test_cartridgedbconnector
        unit/test_cartridgedbconnector.cpp
    )
    set_target_properties(test_cartridgedbconnector PROPERTIES AUTOMOC ON)
    target_include_directories(test_cartridgedbconnector PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_cartridgedbconnector PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestCartridgeDBConnector COMMAND test_cartridgedbconnector)
    
    # test_cartridgedbconnector_errors
    add_executable(test_cartridgedbconnector_errors
        unit/test_cartridgedbconnector_errors.cpp
    )
    set_target_properties(test_cartridgedbconnector_errors PROPERTIES AUTOMOC ON)
    target_include_directories(test_cartridgedbconnector_errors PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_cartridgedbconnector_errors PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestCartridgeDBConnectorErrors COMMAND test_cartridgedbconnector_errors)
    
    # test_manifestmanager_deletion
    add_executable(test_manifestmanager_deletion
        unit/test_manifestmanager_deletion.cpp
    )
    set_target_properties(test_manifestmanager_deletion PROPERTIES AUTOMOC ON)
    target_include_directories(test_manifestmanager_deletion PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_manifestmanager_deletion PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestManifestDeletion COMMAND test_manifestmanager_deletion)
    
    # test_signatureverifier
    add_executable(test_signatureverifier
        unit/test_signatureverifier.cpp
    )
    set_target_properties(test_signatureverifier PROPERTIES AUTOMOC ON)
    target_include_directories(test_signatureverifier PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_signatureverifier PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestSignatureVerifier COMMAND test_signatureverifier)
    
    # test_signatureverifier_l2l3
    add_executable(test_signatureverifier_l2l3
        unit/test_signatureverifier_l2l3.cpp
    )
    set_target_properties(test_signatureverifier_l2l3 PROPERTIES AUTOMOC ON)
    target_include_directories(test_signatureverifier_l2l3 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_signatureverifier_l2l3 PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestSignatureVerifierL2L3 COMMAND test_signatureverifier_l2l3)
    
    # test_trustregistry
    add_executable(test_trustregistry
        unit/test_trustregistry.cpp
    )
    set_target_properties(test_trustregistry PROPERTIES AUTOMOC ON)
    target_include_directories(test_trustregistry PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_trustregistry PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestTrustRegistry COMMAND test_trustregistry)
    
    # test_signatureverifier_tampering
    add_executable(test_signatureverifier_tampering
        unit/test_signatureverifier_tampering.cpp
    )
    set_target_properties(test_signatureverifier_tampering PROPERTIES AUTOMOC ON)
    target_include_directories(test_signatureverifier_tampering PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_signatureverifier_tampering PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestSignatureVerifierTampering COMMAND test_signatureverifier_tampering)
    
    # test_manifestmanager_update
    add_executable(test_manifestmanager_update
        unit/test_manifestmanager_update.cpp
    )
    set_target_properties(test_manifestmanager_update PROPERTIES AUTOMOC ON)
    target_include_directories(test_manifestmanager_update PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_manifestmanager_update PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestManifestUpdate COMMAND test_manifestmanager_update)
    
    # test_manifestmanager_performance
    add_executable(test_manifestmanager_performance
        unit/test_manifestmanager_performance.cpp
    )
    set_target_properties(test_manifestmanager_performance PROPERTIES AUTOMOC ON)
    target_include_directories(test_manifestmanager_performance PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_manifestmanager_performance PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestManifestPerformance COMMAND test_manifestmanager_performance)
    
    # test_database_errors
    add_executable(test_database_errors
        unit/test_database_errors.cpp
    )
    set_target_properties(test_database_errors PROPERTIES AUTOMOC ON)
    target_include_directories(test_database_errors PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_database_errors PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestDatabaseErrors COMMAND test_database_errors)
    
    # test_librarymanager_performance
    add_executable(test_librarymanager_performance
        unit/test_librarymanager_performance.cpp
    )
    set_target_properties(test_librarymanager_performance PROPERTIES AUTOMOC ON)
    target_include_directories(test_librarymanager_performance PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_librarymanager_performance PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestLibraryManagerPerformance COMMAND test_librarymanager_performance)
    
    # test_libraryview_dualview
    add_executable(test_libraryview_dualview
        unit/test_libraryview_dualview.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/LibraryView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/LibraryView.h
    )
    set_target_properties(test_libraryview_dualview PROPERTIES AUTOMOC ON)
    target_include_directories(test_libraryview_dualview PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    )
    target_link_libraries(test_libraryview_dualview PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestLibraryViewDualView COMMAND test_libraryview_dualview)
    
    # test_readerview_content
    add_executable(test_readerview_content
        unit/test_readerview_content.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ReaderView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ReaderView.h
    )
    set_target_properties(test_readerview_content PROPERTIES AUTOMOC ON)
    target_include_directories(test_readerview_content PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    )
    target_link_libraries(test_readerview_content PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestReaderViewContent COMMAND test_readerview_content)
    
    # test_settings_manager
    add_executable(test_settings_manager
        unit/test_settings_manager.cpp
    )
    set_target_properties(test_settings_manager PROPERTIES AUTOMOC ON)
    target_include_directories(test_settings_manager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_settings_manager PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestSettingsManager COMMAND test_settings_manager)

    # test_searchsnippetgenerator
    add_executable(test_searchsnippetgenerator
        unit/test_searchsnippetgenerator.cpp
    )
    set_target_properties(test_searchsnippetgenerator PROPERTIES AUTOMOC ON)
    target_include_directories(test_searchsnippetgenerator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_searchsnippetgenerator PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestSearchSnippetGenerator COMMAND test_searchsnippetgenerator)

    # test_contenteditor
    add_executable(test_contenteditor
        unit/test_contenteditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/ContentEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/PageManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/ContentEditor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/PageManager.h
    )
    set_target_properties(test_contenteditor PROPERTIES AUTOMOC ON)
    target_include_directories(test_contenteditor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
    )
    target_link_libraries(test_contenteditor PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestContentEditor COMMAND test_contenteditor)
    
    # test_pagemanager
    add_executable(test_pagemanager
        unit/test_pagemanager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/PageManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/PageManager.h
    )
    set_target_properties(test_pagemanager PROPERTIES AUTOMOC ON)
    target_include_directories(test_pagemanager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
    )
    target_link_libraries(test_pagemanager PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestPageManager COMMAND test_pagemanager)
    
    # test_contenteditor_pagemanager_integration
    add_executable(test_contenteditor_pagemanager_integration
        unit/test_contenteditor_pagemanager_integration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/ContentEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/PageManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/ContentEditor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/PageManager.h
    )
    set_target_properties(test_contenteditor_pagemanager_integration PROPERTIES AUTOMOC ON)
    target_include_directories(test_contenteditor_pagemanager_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
    )
    target_link_libraries(test_contenteditor_pagemanager_integration PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestContentEditorPageManagerIntegration COMMAND test_contenteditor_pagemanager_integration)
    
    # test_formbuilder
    add_executable(test_formbuilder
        unit/test_formbuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/FormBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/FormManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/FormBuilder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/FormManager.h
    )
    set_target_properties(test_formbuilder PROPERTIES AUTOMOC ON)
    target_include_directories(test_formbuilder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
    )
    target_link_libraries(test_formbuilder PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestFormBuilder COMMAND test_formbuilder)
    
    # test_qmlembeddedappwidget
    add_executable(test_qmlembeddedappwidget
        unit/test_qmlembeddedappwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/QmlAppBridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/QmlEmbeddedAppWidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/QmlAppBridge.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/QmlEmbeddedAppWidget.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
    )
    set_target_properties(test_qmlembeddedappwidget PROPERTIES AUTOMOC ON)
    target_include_directories(test_qmlembeddedappwidget PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    )
    target_link_libraries(test_qmlembeddedappwidget PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickWidgets
        Qt6::Qml
        smartbook_common
    )
    add_test(NAME TestQmlEmbeddedAppWidget COMMAND test_qmlembeddedappwidget)
    
    # test_qmlappbridge
    add_executable(test_qmlappbridge
        unit/test_qmlappbridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/QmlAppBridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/QmlAppBridge.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
    )
    set_target_properties(test_qmlappbridge PROPERTIES AUTOMOC ON)
    target_include_directories(test_qmlappbridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_qmlappbridge PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestQmlAppBridge COMMAND test_qmlappbridge)
    
    # test_contentparser
    add_executable(test_contentparser
        unit/test_contentparser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ContentParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ContentParser.h
    )
    set_target_properties(test_contentparser PROPERTIES AUTOMOC ON)
    target_include_directories(test_contentparser PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    )
    target_link_libraries(test_contentparser PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        smartbook_common
    )
    add_test(NAME TestContentParser COMMAND test_contentparser)
    
    # test_formschemaparser
    add_executable(test_formschemaparser
        unit/test_formschemaparser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/forms/FormSchemaParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include/smartbook/common/forms/FormSchemaParser.h
    )
    set_target_properties(test_formschemaparser PROPERTIES AUTOMOC ON)
    target_include_directories(test_formschemaparser PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_formschemaparser PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestFormSchemaParser COMMAND test_formschemaparser)
    
    # test_formwidgetgenerator
    add_executable(test_formwidgetgenerator
        unit/test_formwidgetgenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/forms/FormWidgetGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/forms/FormSchemaParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include/smartbook/common/forms/FormWidgetGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include/smartbook/common/forms/FormSchemaParser.h
    )
    set_target_properties(test_formwidgetgenerator PROPERTIES AUTOMOC ON)
    target_include_directories(test_formwidgetgenerator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_formwidgetgenerator PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestFormWidgetGenerator COMMAND test_formwidgetgenerator)
    
    # test_formvalidator
    add_executable(test_formvalidator
        unit/test_formvalidator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/forms/FormValidator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include/smartbook/common/forms/FormValidator.h
    )
    set_target_properties(test_formvalidator PROPERTIES AUTOMOC ON)
    target_include_directories(test_formvalidator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_formvalidator PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        smartbook_common
    )
    add_test(NAME TestFormValidator COMMAND test_formvalidator)
    
    # test_readerview_qml_integration
    add_executable(test_readerview_qml_integration
        unit/test_readerview_qml_integration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ReaderView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/QmlEmbeddedAppWidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/QmlAppBridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ContentParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ReaderView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/QmlEmbeddedAppWidget.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/QmlAppBridge.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ContentParser.h
    )
    set_target_properties(test_readerview_qml_integration PROPERTIES AUTOMOC ON)
    target_include_directories(test_readerview_qml_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    )
    target_link_libraries(test_readerview_qml_integration PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        Qt6::QuickWidgets
        Qt6::Qml
        smartbook_common
    )
    add_test(NAME TestReaderViewQmlIntegration COMMAND test_readerview_qml_integration)
    
    # test_contenteditor_form_integration
    add_executable(test_contenteditor_form_integration
        unit/test_contenteditor_form_integration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/ContentEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/FormBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/FormManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/PageManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/ContentEditor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/FormBuilder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/FormManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/PageManager.h
    )
    set_target_properties(test_contenteditor_form_integration PROPERTIES AUTOMOC ON)
    target_include_directories(test_contenteditor_form_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
    )
    target_link_libraries(test_contenteditor_form_integration PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestContentEditorFormIntegration COMMAND test_contenteditor_form_integration)
    
    # test_manifestmanager_errors
    add_executable(test_manifestmanager_errors
        unit/test_manifestmanager_errors.cpp
    )
    set_target_properties(test_manifestmanager_errors PROPERTIES AUTOMOC ON)
    target_include_directories(test_manifestmanager_errors PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_manifestmanager_errors PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestManifestErrors COMMAND test_manifestmanager_errors)
    
    # test_metadataextractor
    add_executable(test_metadataextractor
        unit/test_metadataextractor.cpp
    )
    set_target_properties(test_metadataextractor PROPERTIES AUTOMOC ON)
    target_include_directories(test_metadataextractor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    target_link_libraries(test_metadataextractor PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        smartbook_common
    )
    add_test(NAME TestMetadataExtractor COMMAND test_metadataextractor)
    
    # test_importmanager
    add_executable(test_importmanager
        unit/test_importmanager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ImportManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ImportDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ImportManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ImportDialog.h
    )
    set_target_properties(test_importmanager PROPERTIES AUTOMOC ON)
    target_include_directories(test_importmanager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_importmanager PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestImportManager COMMAND test_importmanager)
    
    # test_webchannelbridge - DEPRECATED: WebChannelBridge removed in Phase 5
    # This test will be replaced in Phase 4 with QmlAppBridge tests
    # add_executable(test_webchannelbridge
    #     unit/test_webchannelbridge.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/WebChannelBridge.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/WebChannelBridge.h
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
    # )
    # set_target_properties(test_webchannelbridge PROPERTIES AUTOMOC ON)
    # target_include_directories(test_webchannelbridge PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/unit
    # )
    # target_link_libraries(test_webchannelbridge PRIVATE
    #     Qt6::Test
    #     Qt6::Core
    #     Qt6::Sql
    #     Qt6::Gui
    #     Qt6::Widgets
    #     Qt6::WebChannel
    #     smartbook_common
    #     test_helpers
    # )
    # add_test(NAME TestWebChannelBridge COMMAND test_webchannelbridge)
    
    # test_readerviewwindow_windowstate
    add_executable(test_readerviewwindow_windowstate
        unit/test_readerviewwindow_windowstate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ReaderViewWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ReaderView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/SecurityErrorDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ReaderViewWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ReaderView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/SecurityErrorDialog.h
    )
    set_target_properties(test_readerviewwindow_windowstate PROPERTIES AUTOMOC ON)
    target_include_directories(test_readerviewwindow_windowstate PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_readerviewwindow_windowstate PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestReaderViewWindowWindowState COMMAND test_readerviewwindow_windowstate)
    
    # test_consentdialog
    add_executable(test_consentdialog
        unit/test_consentdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
    )
    set_target_properties(test_consentdialog PROPERTIES AUTOMOC ON)
    target_include_directories(test_consentdialog PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_consentdialog PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestConsentDialog COMMAND test_consentdialog)
    
    # test_securityerrordialog
    add_executable(test_securityerrordialog
        unit/test_securityerrordialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/SecurityErrorDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/SecurityErrorDialog.h
    )
    set_target_properties(test_securityerrordialog PROPERTIES AUTOMOC ON)
    target_include_directories(test_securityerrordialog PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_securityerrordialog PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
    )
    add_test(NAME TestSecurityErrorDialog COMMAND test_securityerrordialog)
    
    # test_cartridgeexporter
    # Find OpenSSL for CartridgeExporter signing
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        add_executable(test_cartridgeexporter
            unit/test_cartridgeexporter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/CartridgeExporter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/CertificateManager.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/CartridgeExporter.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/CertificateManager.h
        )
        set_target_properties(test_cartridgeexporter PROPERTIES AUTOMOC ON)
        target_include_directories(test_cartridgeexporter PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
            ${CMAKE_CURRENT_SOURCE_DIR}/unit
        )
        target_link_libraries(test_cartridgeexporter PRIVATE
            Qt6::Test
            Qt6::Core
            Qt6::Sql
            Qt6::Network
            Qt6::Gui
            Qt6::Widgets
            smartbook_common
            test_helpers
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        add_test(NAME TestCartridgeExporter COMMAND test_cartridgeexporter)
    else()
        message(WARNING "OpenSSL not found - skipping test_cartridgeexporter")
    endif()
    
    # test_metadataeditor
    # Note: MetadataEditor and ResourceManager are part of creator executable, not a library
    # We compile the source files directly, but need to ensure Qt Widgets is properly linked
    add_executable(test_metadataeditor
        unit/test_metadataeditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/MetadataEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/ResourceManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/MetadataEditor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/ResourceManager.h
    )
    set_target_properties(test_metadataeditor PROPERTIES AUTOMOC ON)
    target_include_directories(test_metadataeditor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_metadataeditor PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestMetadataEditor COMMAND test_metadataeditor)
    
    # test_resourcemanager
    add_executable(test_resourcemanager
        unit/test_resourcemanager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/ResourceManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/ResourceManager.h
    )
    set_target_properties(test_resourcemanager PROPERTIES AUTOMOC ON)
    target_include_directories(test_resourcemanager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_resourcemanager PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestResourceManager COMMAND test_resourcemanager)
    
    # test_certificatemanager
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        add_executable(test_certificatemanager
            unit/test_certificatemanager.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/src/CertificateManager.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include/smartbook/creator/CertificateManager.h
        )
        set_target_properties(test_certificatemanager PROPERTIES AUTOMOC ON)
        target_include_directories(test_certificatemanager PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../creator/include
            ${CMAKE_CURRENT_SOURCE_DIR}/unit
        )
        target_link_libraries(test_certificatemanager PRIVATE
            Qt6::Test
            Qt6::Core
            Qt6::Network
            Qt6::Gui
            Qt6::Widgets
            smartbook_common
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        add_test(NAME TestCertificateManager COMMAND test_certificatemanager)
    else()
        message(WARNING "OpenSSL not found - skipping test_certificatemanager")
    endif()
    
    # Test helpers library (shared by multiple test executables)
    add_library(test_helpers STATIC
        unit/test_helpers.cpp
    )
    target_include_directories(test_helpers PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_helpers PRIVATE
        Qt6::Core
        Qt6::Sql
    )
endif()

# Integration tests
if(QT_TEST_AVAILABLE)
    # test_import_workflow
    add_executable(test_import_workflow
        integration/test_import_workflow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ImportManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ImportDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ImportManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ImportDialog.h
    )
    set_target_properties(test_import_workflow PROPERTIES AUTOMOC ON)
    target_include_directories(test_import_workflow PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_import_workflow PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestImportWorkflow COMMAND test_import_workflow)
    
    # test_security_workflow
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        add_executable(test_security_workflow
            integration/test_security_workflow.cpp
        )
        set_target_properties(test_security_workflow PROPERTIES AUTOMOC ON)
        target_include_directories(test_security_workflow PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
            ${CMAKE_CURRENT_SOURCE_DIR}/unit
        )
        target_link_libraries(test_security_workflow PRIVATE
            Qt6::Test
            Qt6::Core
            Qt6::Sql
            Qt6::Gui
            Qt6::Widgets
            smartbook_common
            test_helpers
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        add_test(NAME TestSecurityWorkflow COMMAND test_security_workflow)
    else()
        message(WARNING "OpenSSL not found - skipping test_security_workflow")
    endif()
    
    # test_multiwindow_isolation
    add_executable(test_multiwindow_isolation
        integration/test_multiwindow_isolation.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ReaderViewWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/LibraryManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ReaderView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/LibraryView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/SecurityErrorDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ImportManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ImportDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ReaderViewWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/LibraryManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ReaderView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/LibraryView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/SecurityErrorDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ImportManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ImportDialog.h
    )
    set_target_properties(test_multiwindow_isolation PROPERTIES AUTOMOC ON)
    target_include_directories(test_multiwindow_isolation PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_multiwindow_isolation PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestMultiWindowIsolation COMMAND test_multiwindow_isolation)
    
    # test_form_data_persistence - DEPRECATED: Uses WebChannelBridge
    # This test needs to be updated in Phase 4 to use QmlAppBridge or direct API
    # add_executable(test_form_data_persistence
    #     integration/test_form_data_persistence.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ReaderViewWindow.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ReaderView.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/SecurityErrorDialog.cpp
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ReaderViewWindow.h
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ReaderView.h
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/SecurityErrorDialog.h
    # )
    # set_target_properties(test_form_data_persistence PROPERTIES AUTOMOC ON)
    # target_include_directories(test_form_data_persistence PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/unit
    # )
    # target_link_libraries(test_form_data_persistence PRIVATE
    #     Qt6::Test
    #     Qt6::Core
    #     Qt6::Sql
    #     Qt6::Gui
    #     Qt6::Widgets
    #     smartbook_common
    #     test_helpers
    # )
    # NOTE: test_form_data_persistence uses WebChannelBridge which is deprecated
    # This test needs to be updated in Phase 4 to use QmlAppBridge or direct API
    # add_test(NAME TestFormDataPersistence COMMAND test_form_data_persistence)
    
    # test_settings_application
    add_executable(test_settings_application
        integration/test_settings_application.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ReaderViewWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ReaderView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ConsentDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/SecurityErrorDialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ReaderViewWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ReaderView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ConsentDialog.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/SecurityErrorDialog.h
    )
    set_target_properties(test_settings_application PROPERTIES AUTOMOC ON)
    target_include_directories(test_settings_application PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/unit
    )
    target_link_libraries(test_settings_application PRIVATE
        Qt6::Test
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Widgets
        smartbook_common
        test_helpers
    )
    add_test(NAME TestSettingsApplication COMMAND test_settings_application)
    
    # test_error_handling
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        add_executable(test_error_handling
            integration/test_error_handling.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ImportManager.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../reader/src/ui/ImportDialog.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ImportManager.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include/smartbook/reader/ui/ImportDialog.h
        )
        set_target_properties(test_error_handling PROPERTIES AUTOMOC ON)
        target_include_directories(test_error_handling PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../reader/include
            ${CMAKE_CURRENT_SOURCE_DIR}/unit
        )
        target_link_libraries(test_error_handling PRIVATE
            Qt6::Test
            Qt6::Core
            Qt6::Sql
            Qt6::Gui
            Qt6::Widgets
            smartbook_common
            test_helpers
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        add_test(NAME TestErrorHandling COMMAND test_error_handling)
    else()
        message(WARNING "OpenSSL not found - skipping test_error_handling")
    endif()
endif()
