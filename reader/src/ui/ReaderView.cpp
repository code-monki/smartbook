#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/settings/SettingsManager.h"
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QVBoxLayout>
#include <QUrl>
#include <QSqlQuery>
#include <QSqlError>
#include <QApplication>
#include <QDebug>

namespace smartbook {
namespace reader {

ReaderView::ReaderView(QWidget* parent)
    : QWidget(parent)
    , m_webView(nullptr)
    , m_webChannelBridge(nullptr)
    , m_settingsManager(nullptr)
    , m_currentPageId(-1)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    setupWebEngine();
    
    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);
    
    m_settingsManager = new common::settings::SettingsManager(this);
    
    connect(m_webView, &QWebEngineView::loadFinished,
            this, &ReaderView::onLoadFinished);
}

ReaderView::~ReaderView() {
    // Ensure WebEngine view is properly destroyed before parent widget
    if (m_webView) {
        // Disconnect signals to prevent callbacks during destruction
        disconnect(m_webView, nullptr, this, nullptr);
        
        // Clear WebChannel first
        if (m_webView->page()) {
            m_webView->page()->setWebChannel(nullptr);
        }
        
        // Delete WebChannel bridge first
        if (m_webChannelBridge) {
            delete m_webChannelBridge;
            m_webChannelBridge = nullptr;
        }
        
        // Delete settings manager
        if (m_settingsManager) {
            delete m_settingsManager;
            m_settingsManager = nullptr;
        }
        
        // Load blank page to trigger cleanup
        m_webView->setHtml("<!DOCTYPE html><html><body></body></html>");
        QApplication::processEvents();
        
        // Delete web view (this will trigger WebEngine cleanup)
        delete m_webView;
        m_webView = nullptr;
    }
}

void ReaderView::setupWebEngine() {
    // TODO: Configure WebEngine profile with security settings
    // For now, use default profile
    // DDD Section 5: WebEngine Profile Configuration
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
    
    // Build complete HTML document with CSS
    QString fullHtml = buildHtmlDocument(htmlContent, css);
    
    // Apply settings (font size, font family, theme, etc.) to HTML
    fullHtml = applySettingsToHtml(fullHtml);
    
    // Load into QWebEngineView
    m_webView->setHtml(fullHtml);
    
    // Setup WebChannel bridge if not already set up
    if (!m_webChannelBridge) {
        m_webChannelBridge = new WebChannelBridge(this);
        QWebChannel* channel = new QWebChannel(this);
        m_webChannelBridge->setupWebChannel(channel);
        m_webView->page()->setWebChannel(channel);
    }
    
    connector.closeCartridge();
}

QString ReaderView::buildHtmlDocument(const QString& htmlContent, const QString& css) {
    QString html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
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
    QString theme = m_settingsManager->getSetting("default_theme", "light");
    QString lineSpacing = m_settingsManager->getSetting("line_spacing", "1.5");
    QString textAlignment = m_settingsManager->getSetting("text_alignment", "left");
    QString pageMargins = m_settingsManager->getSetting("page_margins", R"({"top": 20, "bottom": 20, "left": 30, "right": 30})");
    
    // Inject settings as CSS variables and styles
    QString settingsCss = QString(R"(
        :root {
            --font-size: %1pt;
            --font-family: %2;
            --line-spacing: %3;
            --text-align: %4;
        }
        body {
            font-size: var(--font-size);
            font-family: var(--font-family);
            line-height: var(--line-spacing);
            text-align: var(--text-align);
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
            result.insert(headEndPos, "<style>" + settingsCss + "</style>");
        }
    }
    
    return result;
}

void ReaderView::onLoadFinished(bool success) {
    if (success) {
        emit contentLoaded();
    } else {
        emit errorOccurred("Failed to load content page");
    }
}

} // namespace reader
} // namespace smartbook
