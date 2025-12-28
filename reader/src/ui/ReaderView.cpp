/**
 * @file ReaderView.cpp
 * @brief Implementation of ReaderView widget for displaying cartridge content
 * 
 * This file implements the ReaderView widget, which is the primary content rendering
 * component for the SmartBook Reader. It uses QTextBrowser for HTML content rendering
 * and integrates QML embedded applications and Qt Widgets forms within content pages.
 * 
 * @section Architecture
 * 
 * **Content Rendering:**
 * - QTextBrowser renders HTML4 + CSS 2.1 content synchronously
 * - Theme changes use QPalette for flash-free updates
 * - Settings (font, spacing, alignment) are injected as CSS
 * 
 * **Embedded Components:**
 * - ContentParser detects QML app and form markers in HTML
 * - QmlEmbeddedAppWidget instances are created for each QML app marker
 * - FormEmbeddedWidget instances are created for each form marker
 * - Components are added to vertical layout below HTML content
 * 
 * **Theme Management:**
 * - Uses ThemeManager singleton for global theme preference
 * - Supports light, dark, sepia, and auto (system) themes
 * - Theme is applied via QPalette to prevent rendering flash
 * - All embedded components receive theme updates
 * 
 * @section Migration Notes
 * 
 * This implementation was migrated from Qt WebEngine (QWebEngineView) to QTextDocument/QTextBrowser
 * architecture in Phase 1 of the content rendering migration (2025-12-27).
 * 
 * Key changes:
 * - Replaced QWebEngineView with QTextBrowser
 * - Removed WebChannelBridge (replaced with QmlAppBridge)
 * - Removed async loading (QTextBrowser loads synchronously
 * - Theme changes no longer cause rendering flash (synchronous palette updates)
 * 
 * @see ReaderView.h
 * @see ContentParser
 * @see QmlEmbeddedAppWidget
 * @see FormEmbeddedWidget
 * @see content-rendering-decision-analysis.adoc
 */

#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/ContentParser.h"
#include "smartbook/reader/ui/QmlEmbeddedAppWidget.h"
#include "smartbook/reader/ui/FormEmbeddedWidget.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/settings/SettingsManager.h"
#include "smartbook/common/utils/ThemeManager.h"
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QPalette>
#include <QColor>
#include <QDebug>
#include <QApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QDateTime>

namespace smartbook {
namespace reader {

// ============================================================================
// Constructor and Destructor
// ============================================================================

ReaderView::ReaderView(QWidget* parent)
    : QWidget(parent)
    , m_textBrowser(nullptr)
    , m_contentParser(nullptr)
    , m_settingsManager(nullptr)
    , m_currentPageId(-1)
    , m_tempTheme()
{
    // Setup layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Create QTextBrowser for content rendering
    // QTextBrowser provides HTML4 + CSS 2.1 rendering with synchronous loading
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setMinimumSize(400, 300); // Ensure minimum size
    layout->addWidget(m_textBrowser);
    
    // Initialize components
    m_contentParser = new ContentParser();
    m_settingsManager = new common::settings::SettingsManager(this);
    
    // Set initial theme (light) explicitly - don't rely on settings manager yet
    // This ensures the widget has a valid appearance before cartridge loads
    QPalette palette = m_textBrowser->palette();
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::Text, QColor(0, 0, 0));
    palette.setColor(QPalette::Window, QColor(255, 255, 255));
    m_textBrowser->setPalette(palette);
}

ReaderView::~ReaderView() {
    // Clean up embedded components
    cleanupQmlAppWidgets();
    cleanupFormWidgets();
    
    // QTextBrowser and other child widgets are cleaned up automatically
    // by Qt's parent-child relationship
}

// ============================================================================
// Public Methods
// ============================================================================

void ReaderView::loadCartridge(const QString& cartridgePath, const QString& cartridgeGuid) {
    qDebug() << "ReaderView::loadCartridge: Called with path=" << cartridgePath << ", guid=" << cartridgeGuid;
    
    m_cartridgePath = cartridgePath;
    m_cartridgeGuid = cartridgeGuid;
    m_currentPageId = -1;
    
    // Load settings if cartridge GUID is provided
    if (!m_cartridgeGuid.isEmpty() && m_settingsManager) {
        qDebug() << "ReaderView::loadCartridge: Loading settings for cartridge";
        m_settingsManager->loadSettings(m_cartridgeGuid, cartridgePath);
        
        // If we had a temporary theme set before cartridge loaded, save it now
        if (!m_tempTheme.isEmpty()) {
            qDebug() << "ReaderView::loadCartridge: Saving temporary theme" << m_tempTheme;
            m_settingsManager->setUserOverride("default_theme", m_tempTheme);
            m_tempTheme.clear();
        }
        
        // Always use global theme preference (ignore cartridge-specific overrides)
        // This ensures theme selection in menu matches what's displayed
        QString globalTheme = common::utils::ThemeManager::getInstance().getTheme();
        qDebug() << "ReaderView::loadCartridge: Using global theme preference:" << globalTheme;
        
        // Apply theme immediately after loading settings
        qDebug() << "ReaderView::loadCartridge: Applying theme after settings load";
        applyTheme();
    }
    
    // Load first page (lowest page_order)
    qDebug() << "ReaderView::loadCartridge: Calling loadPage(-1)";
    loadPage(-1); // -1 means load first page
}

void ReaderView::loadPage(int pageId) {
    // Auto-save all form widgets before navigating away to prevent data loss
    qDebug() << "ReaderView::loadPage: Called with pageId=" << pageId;
    
    // Auto-save all form widgets before navigating away
    for (FormEmbeddedWidget* formWidget : m_formWidgets) {
        if (formWidget && formWidget->isLoaded()) {
            qDebug() << "ReaderView::loadPage: Auto-saving form" << formWidget->formId();
            formWidget->saveFormData();
        }
    }
    
    m_currentPageId = pageId;
    loadContentFromDatabase();
}

// ============================================================================
// Private Methods - Content Loading
// ============================================================================

void ReaderView::loadContentFromDatabase() {
    qDebug() << "ReaderView::loadContentFromDatabase: Starting, cartridgePath=" << m_cartridgePath << ", pageId=" << m_currentPageId;
    
    if (m_cartridgePath.isEmpty()) {
        QString error = "No cartridge path specified";
        qWarning() << "ReaderView::loadContentFromDatabase:" << error;
        emit errorOccurred(error);
        return;
    }
    
    if (!m_textBrowser) {
        QString error = "QTextBrowser widget is null";
        qWarning() << "ReaderView::loadContentFromDatabase:" << error;
        emit errorOccurred(error);
        return;
    }
    
    // Open cartridge database
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        QString error = "Failed to open cartridge: " + m_cartridgePath;
        qWarning() << "ReaderView::loadContentFromDatabase:" << error;
        emit errorOccurred(error);
        return;
    }
    
    qDebug() << "ReaderView::loadContentFromDatabase: Cartridge opened successfully";
    
    // Query Content_Pages table
    // If pageId is -1, load first page (lowest page_order)
    QString queryString;
    if (m_currentPageId == -1) {
        queryString = R"(
            SELECT page_id, html_content, associated_css
            FROM Content_Pages
            ORDER BY page_order ASC
            LIMIT 1
        )";
    } else {
        queryString = QString(R"(
            SELECT page_id, html_content, associated_css
            FROM Content_Pages
            WHERE page_id = %1
        )").arg(m_currentPageId);
    }
    
    QSqlQuery query(connector.getDatabase());
    if (!query.exec(queryString)) {
        QString error = "Failed to query content: " + query.lastError().text();
        qWarning() << "ReaderView::loadContentFromDatabase:" << error;
        connector.closeCartridge();
        emit errorOccurred(error);
        return;
    }
    
    if (!query.next()) {
        QString error = "No content pages found in cartridge";
        qWarning() << "ReaderView::loadContentFromDatabase:" << error;
        connector.closeCartridge();
        emit errorOccurred(error);
        return;
    }
    
    int pageId = query.value(0).toInt();
    QString htmlContent = query.value(1).toString();
    QString css = query.value(2).toString();
    
    qDebug() << "ReaderView::loadContentFromDatabase: Loaded page" << pageId << "with" << htmlContent.length() << "chars of HTML";
    
    m_currentPageId = pageId;
    
    // Process QML app markers before building HTML document
    processQmlAppMarkers(htmlContent);
    processFormMarkers(htmlContent);
    
    // Clean HTML content (remove QML and form markers)
    QString cleanedHtml = m_contentParser->cleanHtml(htmlContent);
    qDebug() << "ReaderView::loadContentFromDatabase: Cleaned HTML length:" << cleanedHtml.length();
    
    // Build complete HTML document with CSS
    QString fullHtml = buildHtmlDocument(cleanedHtml, css);
    
    // Apply settings (font size, font family, theme, etc.) to HTML
    fullHtml = applySettingsToHtml(fullHtml);
    
    qDebug() << "ReaderView::loadContentFromDatabase: Setting HTML content, length:" << fullHtml.length();
    qDebug() << "ReaderView::loadContentFromDatabase: HTML preview (first 500 chars):" << fullHtml.left(500);
    
    // Ensure QTextBrowser is visible before setting content
    m_textBrowser->show();
    m_textBrowser->setVisible(true);
    m_textBrowser->raise();
    
    qDebug() << "ReaderView::loadContentFromDatabase: Before setHtml - QTextBrowser visible:" << m_textBrowser->isVisible();
    qDebug() << "ReaderView::loadContentFromDatabase: Before setHtml - QTextBrowser size:" << m_textBrowser->size();
    qDebug() << "ReaderView::loadContentFromDatabase: Before setHtml - QTextBrowser geometry:" << m_textBrowser->geometry();
    
    // Load into QTextBrowser (synchronous)
    m_textBrowser->setHtml(fullHtml);
    
    qDebug() << "ReaderView::loadContentFromDatabase: After setHtml - QTextBrowser document size:" << m_textBrowser->document()->size();
    qDebug() << "ReaderView::loadContentFromDatabase: After setHtml - QTextBrowser toPlainText length:" << m_textBrowser->toPlainText().length();
    qDebug() << "ReaderView::loadContentFromDatabase: After setHtml - QTextBrowser toPlainText preview:" << m_textBrowser->toPlainText().left(200);
    
    // Verify content was actually set
    QString retrievedHtml = m_textBrowser->toHtml();
    qDebug() << "ReaderView::loadContentFromDatabase: Retrieved HTML length:" << retrievedHtml.length();
    
    // Apply theme to QTextBrowser widget
    applyTheme();
    
    // Force update and repaint
    m_textBrowser->update();
    m_textBrowser->repaint();
    m_textBrowser->viewport()->update();
    QApplication::processEvents();
    
    qDebug() << "ReaderView::loadContentFromDatabase: Content loaded successfully";
    
    // Emit contentLoaded signal immediately (QTextBrowser loads synchronously)
    emit contentLoaded();
    
    connector.closeCartridge();
}

QString ReaderView::buildHtmlDocument(const QString& htmlContent, const QString& css) {
    /**
     * @brief Build complete HTML document from content and CSS
     * 
     * Creates a valid HTML4 document structure with DOCTYPE, head, and body.
     * Uses HTML4 DOCTYPE for QTextBrowser compatibility (HTML5 not supported).
     * 
     * @param htmlContent Raw HTML content from Content_Pages table
     * @param css CSS styles to inject into document
     * @return Complete HTML document string
     */
    
    // Use HTML4 DOCTYPE for QTextBrowser compatibility
    QString html = R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <style type="text/css">
)";
    
    if (!css.isEmpty()) {
        html += css;
    }
    
    html += R"(
    </style>
</head>
<body>
)";
    
    html += htmlContent;
    
    html += R"(
</body>
</html>
)";
    
    return html;
}

QString ReaderView::applySettingsToHtml(const QString& html) {
    /**
     * @brief Apply rendering settings to HTML content
     * 
     * Injects CSS 2.1 compatible styles into HTML document based on settings
     * from cartridge Settings table (author defaults + user overrides).
     * 
     * Settings applied:
     * - Font size (default_font_size)
     * - Font family (default_font_family)
     * - Line spacing (line_spacing)
     * - Text alignment (text_alignment)
     * 
     * Note: Uses CSS 2.1 properties only (no CSS variables or CSS3 features).
     * 
     * @param html HTML document string
     * @return HTML with settings CSS injected
     */
    if (!m_settingsManager) {
        return html;
    }
    
    // Get settings with priority resolution
    QString fontSize = m_settingsManager->getSetting("default_font_size", "12");
    QString fontFamily = m_settingsManager->getSetting("default_font_family", "serif");
    QString lineSpacing = m_settingsManager->getSetting("line_spacing", "1.5");
    QString textAlignment = m_settingsManager->getSetting("text_alignment", "left");
    
    // Inject settings as CSS 2.1 compatible styles (no CSS variables)
    QString settingsCss = QString(R"(
        body {
            font-size: %1pt;
            font-family: %2;
            line-height: %3;
            text-align: %4;
        }
    )").arg(fontSize, fontFamily, lineSpacing, textAlignment);
    
    // Insert settings CSS before closing </style> tag
    QString result = html;
    int styleEndPos = result.lastIndexOf("</style>");
    if (styleEndPos != -1) {
        result.insert(styleEndPos, settingsCss);
    } else {
        // No style tag, add one in head
        int headEndPos = result.indexOf("</head>");
        if (headEndPos != -1) {
            result.insert(headEndPos, "<style type=\"text/css\">" + settingsCss + "</style>");
        }
    }
    
    return result;
}

// ============================================================================
// Private Methods - Theme Management
// ============================================================================

void ReaderView::applyTheme() {
    /**
     * @brief Apply theme to content rendering
     * 
     * Applies theme colors via QPalette to prevent rendering flash.
     * Uses Qt::WA_UpdatesDisabled attribute to block intermediate repaints
     * during palette change, ensuring atomic theme update.
     * 
     * Theme colors:
     * - Light: White background (#FFFFFF), black text (#000000)
     * - Dark: Dark background (#1E1E1E), light text (#D4D4D4)
     * - Sepia: Sepia background (#F4ECD8), dark brown text (#5C4B37)
     * 
     * Also applies theme to all embedded form widgets.
     */
    if (!m_textBrowser) {
        qWarning() << "ReaderView::applyTheme: m_textBrowser is null";
        return;
    }
    
    QString theme = "light"; // Default
    // Check for temporary theme first (set before cartridge loads)
    if (!m_tempTheme.isEmpty()) {
        theme = m_tempTheme;
        qDebug() << "ReaderView::applyTheme: Using temporary theme" << theme;
    } else if (m_settingsManager) {
        // Always use global theme preference (cartridge-specific overrides are deprecated)
        // This ensures theme selection in menu matches what's displayed
        theme = common::utils::ThemeManager::getInstance().getTheme();
        qDebug() << "ReaderView::applyTheme: Using global theme preference:" << theme;
    } else {
        // Use global theme preference
        theme = common::utils::ThemeManager::getInstance().getTheme();
        qDebug() << "ReaderView::applyTheme: Using global theme preference:" << theme;
    }
    
    // Resolve "auto" theme by detecting system appearance
    theme = common::utils::ThemeManager::getInstance().resolveTheme(theme);
    qDebug() << "ReaderView::applyTheme: Resolved theme:" << theme;
    
    qDebug() << "ReaderView::applyTheme: Applying theme" << theme;
    
    QColor bgColor, textColor;
    
    if (theme == "dark") {
        bgColor = QColor(30, 30, 30);
        textColor = QColor(212, 212, 212);
    } else if (theme == "sepia") {
        bgColor = QColor(244, 236, 216);
        textColor = QColor(92, 75, 55);
    } else { // light (default)
        bgColor = QColor(255, 255, 255);
        textColor = QColor(0, 0, 0);
    }
    
    // Update palette atomically to minimize flash
    QPalette palette = m_textBrowser->palette();
    palette.setColor(QPalette::Base, bgColor);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Window, bgColor);
    
    // Block repaints during palette change to prevent flash
    m_textBrowser->setAttribute(Qt::WA_UpdatesDisabled, true);
    m_textBrowser->setPalette(palette);
    m_textBrowser->setAttribute(Qt::WA_UpdatesDisabled, false);
    
    // Also update parent widget palette if available
    if (parentWidget()) {
        QPalette parentPalette = parentWidget()->palette();
        parentPalette.setColor(QPalette::Window, bgColor);
        parentWidget()->setPalette(parentPalette);
    }
    
    // Apply theme to all form widgets
    for (FormEmbeddedWidget* formWidget : m_formWidgets) {
        if (formWidget) {
            formWidget->applyTheme(bgColor, textColor);
        }
    }
    
    // Force single atomic repaint
    m_textBrowser->update();
    m_textBrowser->repaint();
    qDebug() << "ReaderView::applyTheme: Theme applied successfully, bgColor=" << bgColor.name() << ", textColor=" << textColor.name();
}

void ReaderView::setTheme(const QString& theme) {
    /**
     * @brief Set theme preference
     * 
     * Saves theme as global preference (shared across all apps) and applies
     * it immediately. Clears any cartridge-specific overrides to ensure
     * global theme takes precedence.
     */
    qDebug() << "ReaderView::setTheme: Called with theme=" << theme << ", cartridgeGuid=" << m_cartridgeGuid;
    
    // Store theme temporarily for immediate application
    m_tempTheme = theme;
    
    // Save theme as global preference (shared across all apps)
    common::utils::ThemeManager::getInstance().setTheme(theme);
    
    // Clear any cartridge-specific override to ensure global theme is used
    // This ensures menu selection matches displayed theme
    if (!m_cartridgeGuid.isEmpty() && m_settingsManager) {
        // Remove cartridge-specific override so global theme takes precedence
        m_settingsManager->resetToAuthorDefaults(); // This clears user overrides
        qDebug() << "ReaderView::setTheme: Cleared cartridge-specific override to use global theme";
    }
    
    // Clear temp theme
    m_tempTheme.clear();
    
    // Apply theme immediately
    applyTheme();
}

// ============================================================================
// Private Methods - Embedded Component Processing
// ============================================================================

void ReaderView::processQmlAppMarkers(const QString& htmlContent)
{
    /**
     * @brief Process QML app markers in HTML content
     * 
     * Detects QML app markers (`<div data-smartbook-qml-app="app_id"></div>`)
     * and creates QmlEmbeddedAppWidget instances for each marker.
     * 
     * Process:
     * 1. Clean up existing QML app widgets
     * 2. Parse HTML for QML app markers using ContentParser
     * 3. For each marker, create QmlEmbeddedAppWidget and load QML app
     * 4. Add widgets to layout (currently below HTML content)
     * 
     * @note Future enhancement: Position widgets inline with content using
     * anchor positions from markers.
     */
    // Clean up existing QML app widgets
    cleanupQmlAppWidgets();
    
    if (!m_contentParser) {
        return;
    }
    
    // Parse HTML for QML app markers
    QList<ContentParser::QmlAppMarker> markers = m_contentParser->parseContent(htmlContent);
    
    if (markers.isEmpty()) {
        return;
    }
    
    // Create QmlEmbeddedAppWidget for each marker
    for (const ContentParser::QmlAppMarker& marker : markers) {
        if (marker.appId.isEmpty()) {
            continue;
        }
        
        qDebug() << "ReaderView::processQmlAppMarkers: Creating QML app widget for appId=" << marker.appId;
        
        QmlEmbeddedAppWidget* appWidget = new QmlEmbeddedAppWidget(this);
        appWidget->setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
        
        // Load the QML app
        bool loaded = appWidget->loadApp(m_cartridgePath, marker.appId);
        if (loaded) {
            m_qmlAppWidgets.append(appWidget);
            
            // Add widget to layout (below QTextBrowser for now)
            // TODO: In the future, position widgets inline with content using anchor positions
            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(this->layout());
            if (layout) {
                layout->addWidget(appWidget);
                appWidget->setMinimumHeight(200); // Give it a reasonable size
                appWidget->show();
                qDebug() << "ReaderView::processQmlAppMarkers: Added QML app widget to layout, visible=" << appWidget->isVisible();
            } else {
                qWarning() << "ReaderView::processQmlAppMarkers: Layout is not a QVBoxLayout!";
            }
        } else {
            qWarning() << "Failed to load QML app:" << marker.appId << appWidget->errorMessage();
            delete appWidget;
        }
    }
}

void ReaderView::cleanupQmlAppWidgets()
{
    for (QmlEmbeddedAppWidget* widget : m_qmlAppWidgets) {
        if (widget) {
            widget->deleteLater();
        }
    }
    m_qmlAppWidgets.clear();
}

void ReaderView::processFormMarkers(const QString& htmlContent)
{
    /**
     * @brief Process form markers in HTML content
     * 
     * Detects form markers (`<div data-smartbook-form="form_id"></div>`)
     * and creates FormEmbeddedWidget instances for each marker.
     * 
     * Process:
     * 1. Clean up existing form widgets
     * 2. Parse HTML for form markers using ContentParser
     * 3. For each marker, create FormEmbeddedWidget and load form schema
     * 4. Add widgets to layout (currently below HTML content)
     * 
     * @note Future enhancement: Position widgets inline with content using
     * anchor positions from markers.
     */
    // Clean up existing form widgets
    cleanupFormWidgets();
    
    if (!m_contentParser) {
        return;
    }
    
    // Parse HTML for form markers
    QList<ContentParser::FormMarker> markers = m_contentParser->parseFormMarkers(htmlContent);
    
    if (markers.isEmpty()) {
        return;
    }
    
    // Create FormEmbeddedWidget for each marker
    for (const ContentParser::FormMarker& marker : markers) {
        if (marker.formId.isEmpty()) {
            continue;
        }
        
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget(marker.formId, this);
        
        // Load the form
        bool loaded = formWidget->loadForm(m_cartridgePath);
        if (loaded) {
            m_formWidgets.append(formWidget);
            
            // Add widget to layout (below QTextBrowser for now)
            // TODO: In the future, position widgets inline with content using anchor positions
            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(this->layout());
            if (layout) {
                layout->addWidget(formWidget);
                formWidget->show();
                qDebug() << "ReaderView::processFormMarkers: Added form widget to layout, formId=" << marker.formId;
            } else {
                qWarning() << "ReaderView::processFormMarkers: Layout is not a QVBoxLayout!";
            }
        } else {
            qWarning() << "Failed to load form:" << marker.formId << formWidget->errorMessage();
            delete formWidget;
        }
    }
}

void ReaderView::cleanupFormWidgets()
{
    for (FormEmbeddedWidget* widget : m_formWidgets) {
        if (widget) {
            widget->deleteLater();
        }
    }
    m_formWidgets.clear();
}

QString ReaderView::loadGlobalThemePreference()
{
    // Use ThemeManager for global theme preference
    return common::utils::ThemeManager::getInstance().getTheme();
}

void ReaderView::saveGlobalThemePreference(const QString& theme)
{
    // Use ThemeManager for global theme preference
    common::utils::ThemeManager::getInstance().setTheme(theme);
}

QString ReaderView::detectSystemTheme()
{
    // Use ThemeManager for system theme detection
    return common::utils::ThemeManager::detectSystemTheme();
}

} // namespace reader
} // namespace smartbook
