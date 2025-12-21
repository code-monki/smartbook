#ifndef SMARTBOOK_CREATOR_CONTENTEDITOR_H
#define SMARTBOOK_CREATOR_CONTENTEDITOR_H

#include <QWidget>
#include <QWebEngineView>
#include <QToolBar>
#include <QAction>
#include <QString>

namespace smartbook {
namespace creator {

class PageManager;

/**
 * @brief Content editor widget
 * 
 * WYSIWYG HTML editor using Qt WebEngineView for content authoring.
 * Implements FR-CT-3.1 through FR-CT-3.5
 */
class ContentEditor : public QWidget {
    Q_OBJECT

public:
    explicit ContentEditor(QWidget* parent = nullptr);
    ~ContentEditor();

    /**
     * @brief Load content for editing
     * @param htmlContent HTML content to edit
     */
    void loadContent(const QString& htmlContent);

    /**
     * @brief Get edited content
     * @return HTML content
     * @note This method returns cached content. Call updateContentCache() first
     *       to get the latest content from the editor.
     */
    QString getContent() const;
    
    /**
     * @brief Update content cache from editor
     * This is called asynchronously and updates m_currentContent
     */
    void updateContentCache();
    
    /**
     * @brief Get current content for saving
     * @return HTML content ready to save
     */
    QString getContentForSave();
    
    /**
     * @brief Save current content to a page via PageManager
     * @param pageManager PageManager instance
     * @param pageId Page ID to save to
     * @return true if saved successfully
     */
    bool saveToPage(PageManager* pageManager, int pageId);
    
    /**
     * @brief Insert form marker into content at cursor position
     * @param formId Form identifier
     * @return true if inserted successfully
     */
    bool insertFormMarker(const QString& formId);

    /**
     * @brief Toggle HTML editing mode
     * @param enabled true for HTML mode, false for WYSIWYG mode
     */
    void setHtmlMode(bool enabled);

    /**
     * @brief Check if in HTML editing mode
     * @return true if in HTML mode, false if in WYSIWYG mode
     */
    bool isHtmlMode() const { return m_htmlMode; }

    /**
     * @brief Toggle preview mode
     * @param enabled true to show preview, false to show editor
     */
    void setPreviewMode(bool enabled);

    /**
     * @brief Check if in preview mode
     * @return true if in preview mode
     */
    bool isPreviewMode() const { return m_previewMode; }

signals:
    void contentChanged();
    void undoAvailable(bool available);
    void redoAvailable(bool available);

public slots:
    void cut();
    void copy();
    void paste();
    void undo();
    void redo();
    void selectAll();
    
    // Formatting actions
    void bold();
    void italic();
    void underline();
    void insertUnorderedList();
    void insertOrderedList();
    void insertLink();
    void insertImage();

private slots:
    void onLoadFinished(bool success);
    void onContentChanged();
    void executeJavaScript(const QString& script);

private:
    void setupUI();
    void setupToolbar();
    void setupWebEngine();
    void setupEditorHTML();
    QString getEditorHTML() const;
    void injectEditorScripts();
    void updateContentFromJavaScript(const QString& content);
    
    QWebEngineView* m_webView;
    QToolBar* m_toolbar;
    QAction* m_boldAction;
    QAction* m_italicAction;
    QAction* m_underlineAction;
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_htmlModeAction;
    QAction* m_previewAction;
    
    bool m_htmlMode;
    bool m_previewMode;
    QString m_currentContent;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_CONTENTEDITOR_H
