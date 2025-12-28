#ifndef SMARTBOOK_READER_UI_READERVIEW_H
#define SMARTBOOK_READER_UI_READERVIEW_H

#include <QWidget>
#include <QTextBrowser>
#include <QString>

namespace smartbook {
namespace common {
namespace settings {
    class SettingsManager;
}
}

namespace reader {

class ContentParser;
class QmlEmbeddedAppWidget;
class FormEmbeddedWidget;

/**
 * @brief Reader view widget - displays cartridge content
 * 
 * The ReaderView widget is the primary content rendering component for the SmartBook Reader.
 * It uses QTextBrowser to render HTML4/CSS 2.1 content and integrates QML embedded applications
 * and Qt Widgets forms within the content layout.
 * 
 * @section Architecture
 * 
 * **Content Rendering:**
 * - Uses QTextBrowser for HTML content rendering (HTML4 + CSS 2.1 subset)
 * - Theme-aware palette-based styling (no flash on theme changes)
 * - Synchronous content loading (no async delays)
 * 
 * **Embedded Applications:**
 * - Detects QML app markers in HTML: `<div data-smartbook-qml-app="app_id"></div>`
 * - Creates QmlEmbeddedAppWidget instances for each marker
 * - QML apps run in isolated QQuickWidget instances
 * 
 * **Form Rendering:**
 * - Detects form markers in HTML: `<div data-smartbook-form="form_id"></div>`
 * - Creates FormEmbeddedWidget instances for each marker
 * - Forms are rendered as Qt Widgets with validation and persistence
 * 
 * **Settings Application:**
 * - Loads author-defined settings from cartridge Settings table
 * - Applies user overrides (stored in local database)
 * - Supports theme selection (light, dark, sepia, auto)
 * - Settings are applied via CSS injection and QPalette
 * 
 * @section Usage
 * 
 * @code
 * ReaderView* view = new ReaderView(parent);
 * view->loadCartridge("/path/to/cartridge.sqlite", "cartridge-guid");
 * connect(view, &ReaderView::contentLoaded, this, &MyClass::onContentLoaded);
 * @endcode
 * 
 * @section Migration
 * 
 * This class was migrated from Qt WebEngine (QWebEngineView) to QTextDocument/QTextBrowser
 * architecture in Phase 1 of the content rendering migration (2025-12-27).
 * 
 * @see ContentParser
 * @see QmlEmbeddedAppWidget
 * @see FormEmbeddedWidget
 * @see content-rendering-decision-analysis.adoc
 */
class ReaderView : public QWidget {
    Q_OBJECT

public:
    explicit ReaderView(QWidget* parent = nullptr);
    ~ReaderView();

    /**
     * @brief Load cartridge content
     * @param cartridgePath Path to cartridge file
     * @param cartridgeGuid Cartridge GUID for settings lookup
     */
    void loadCartridge(const QString& cartridgePath, const QString& cartridgeGuid = QString());
    
    /**
     * @brief Load a specific page by page_id
     * @param pageId Page ID to load
     */
    void loadPage(int pageId);
    
    /**
     * @brief Get current page ID
     * @return Current page ID, or -1 if no page loaded
     */
    int getCurrentPageId() const { return m_currentPageId; }
    
    /**
     * @brief Set theme (light/dark/sepia)
     * @param theme Theme name
     */
    void setTheme(const QString& theme);

signals:
    void contentLoaded();
    void errorOccurred(const QString& errorMessage);

private:
    void loadContentFromDatabase();
    QString buildHtmlDocument(const QString& htmlContent, const QString& css);
    QString applySettingsToHtml(const QString& html);
    void applyTheme();
    void processQmlAppMarkers(const QString& htmlContent);
    void cleanupQmlAppWidgets();
    void processFormMarkers(const QString& htmlContent);
    void cleanupFormWidgets();
    QString loadGlobalThemePreference();
    void saveGlobalThemePreference(const QString& theme);
    QString detectSystemTheme();
    
    QTextBrowser* m_textBrowser;
    ContentParser* m_contentParser;
    common::settings::SettingsManager* m_settingsManager;
    QList<QmlEmbeddedAppWidget*> m_qmlAppWidgets;
    QList<FormEmbeddedWidget*> m_formWidgets;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    int m_currentPageId = -1;
    QString m_tempTheme; // Temporary theme storage when cartridgeGuid isn't set yet
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_READERVIEW_H
