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
 * Uses QTextBrowser to render HTML4/CSS 2.1 content with embedded QML applications.
 * Loads content from Content_Pages table in the cartridge database.
 * Applies settings (author defaults and user overrides) to content rendering.
 * Detects and processes QML app markers using ContentParser.
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
    
    QTextBrowser* m_textBrowser;
    ContentParser* m_contentParser;
    common::settings::SettingsManager* m_settingsManager;
    QList<QmlEmbeddedAppWidget*> m_qmlAppWidgets;
    QList<FormEmbeddedWidget*> m_formWidgets;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    int m_currentPageId = -1;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_READERVIEW_H
