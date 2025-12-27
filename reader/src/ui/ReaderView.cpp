#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/ContentParser.h"
#include "smartbook/reader/ui/QmlEmbeddedAppWidget.h"
#include "smartbook/reader/ui/FormEmbeddedWidget.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/settings/SettingsManager.h"
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QPalette>
#include <QColor>
#include <QDebug>

namespace smartbook {
namespace reader {

ReaderView::ReaderView(QWidget* parent)
    : QWidget(parent)
    , m_textBrowser(nullptr)
    , m_contentParser(nullptr)
    , m_settingsManager(nullptr)
    , m_currentPageId(-1)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setOpenExternalLinks(true);
    layout->addWidget(m_textBrowser);
    
    m_contentParser = new ContentParser();
    m_settingsManager = new common::settings::SettingsManager(this);
}

ReaderView::~ReaderView() {
    cleanupQmlAppWidgets();
    // QTextBrowser cleanup is handled by Qt's parent-child relationship
}

void ReaderView::loadCartridge(const QString& cartridgePath, const QString& cartridgeGuid) {
    m_cartridgePath = cartridgePath;
    m_cartridgeGuid = cartridgeGuid;
    m_currentPageId = -1;
    
    // Load settings if cartridge GUID is provided
    if (!m_cartridgeGuid.isEmpty() && m_settingsManager) {
        m_settingsManager->loadSettings(m_cartridgeGuid, cartridgePath);
    }
    
    // Load first page (lowest page_order)
    loadPage(-1); // -1 means load first page
}

void ReaderView::loadPage(int pageId) {
    m_currentPageId = pageId;
    loadContentFromDatabase();
}

void ReaderView::loadContentFromDatabase() {
    if (m_cartridgePath.isEmpty()) {
        emit errorOccurred("No cartridge path specified");
        return;
    }
    
    // Open cartridge database
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        emit errorOccurred("Failed to open cartridge: " + m_cartridgePath);
        return;
    }
    
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
    
    QSqlQuery query = connector.executeQuery(queryString);
    
    if (!query.next()) {
        connector.closeCartridge();
        emit errorOccurred("No content pages found in cartridge");
        return;
    }
    
    int pageId = query.value(0).toInt();
    QString htmlContent = query.value(1).toString();
    QString css = query.value(2).toString();
    
    m_currentPageId = pageId;
    
    // Process QML app markers before building HTML document
    processQmlAppMarkers(htmlContent);
    processFormMarkers(htmlContent);
    
    // Clean HTML content (remove QML and form markers)
    QString cleanedHtml = m_contentParser->cleanHtml(htmlContent);
    
    // Build complete HTML document with CSS
    QString fullHtml = buildHtmlDocument(cleanedHtml, css);
    
    // Apply settings (font size, font family, theme, etc.) to HTML
    fullHtml = applySettingsToHtml(fullHtml);
    
    // Load into QTextBrowser (synchronous)
    m_textBrowser->setHtml(fullHtml);
    
    // Apply theme to QTextBrowser widget
    applyTheme();
    
    // Emit contentLoaded signal immediately (QTextBrowser loads synchronously)
    emit contentLoaded();
    
    connector.closeCartridge();
}

QString ReaderView::buildHtmlDocument(const QString& htmlContent, const QString& css) {
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

void ReaderView::applyTheme() {
    if (!m_textBrowser || !m_settingsManager) {
        return;
    }
    
    QString theme = m_settingsManager->getSetting("default_theme", "light");
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
    
    // Block repaints during palette change to prevent flash
    m_textBrowser->setAttribute(Qt::WA_UpdatesDisabled, true);
    m_textBrowser->setPalette(palette);
    m_textBrowser->setAttribute(Qt::WA_UpdatesDisabled, false);
    
    // Force single atomic repaint
    m_textBrowser->update();
}

void ReaderView::setTheme(const QString& theme) {
    if (!m_settingsManager || m_cartridgeGuid.isEmpty()) {
        return;
    }
    
    // Save theme as user override
    m_settingsManager->setUserOverride("default_theme", theme);
    
    // Apply theme immediately
    applyTheme();
}

void ReaderView::processQmlAppMarkers(const QString& htmlContent)
{
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
        
        QmlEmbeddedAppWidget* appWidget = new QmlEmbeddedAppWidget(this);
        appWidget->setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
        
        // Load the QML app
        bool loaded = appWidget->loadApp(m_cartridgePath, marker.appId);
        if (loaded) {
            m_qmlAppWidgets.append(appWidget);
            // Note: Widget positioning will be handled in a future update
            // For now, widgets are created but not positioned in layout
            // This is a placeholder for the actual embedding mechanism
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
            // Note: Widget positioning will be handled in a future update
            // For now, widgets are created but not positioned in layout
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

} // namespace reader
} // namespace smartbook
