#ifndef SMARTBOOK_CREATOR_CONTENTEDITOR_H
#define SMARTBOOK_CREATOR_CONTENTEDITOR_H

#include <QWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QAction>
#include <QString>
#include <QColor>

namespace smartbook {
namespace creator {

class PageManager;

/**
 * @brief Content editor widget for HTML content authoring
 * 
 * ContentEditor provides a WYSIWYG (What-You-See-Is-What-You-Get) HTML editor
 * for the Creator Tool. It uses QTextEdit for rich text editing and QPlainTextEdit
 * for HTML source editing, allowing authors to create and edit cartridge content.
 * 
 * @section Architecture
 * 
 * **WYSIWYG Mode (Default):**
 * - Uses QTextEdit for rich text editing
 * - Provides formatting toolbar (bold, italic, underline, colors, fonts, lists, links, images)
 * - Content is edited visually with immediate feedback
 * - HTML is generated automatically from formatted content
 * 
 * **HTML Source Mode:**
 * - Uses QPlainTextEdit for direct HTML editing
 * - Allows authors to edit raw HTML source
 * - Content is synchronized between WYSIWYG and HTML modes
 * 
 * **Preview Mode:**
 * - Shows read-only preview of content
 * - Uses QTextEdit in read-only mode
 * - Useful for reviewing content before saving
 * 
 * @section Features
 * 
 * **Rich Text Formatting:**
 * - Text formatting: bold, italic, underline
 * - Font selection: family, size, color
 * - Text alignment: left, center, right, justify
 * - Lists: ordered and unordered
 * - Links: insert hyperlinks
 * - Images: insert images from file system
 * 
 * **Content Management:**
 * - Load content from cartridge database
 * - Save content to cartridge database via PageManager
 * - Undo/redo support
 * - Content change tracking
 * 
 * **Form Integration:**
 * - Insert form markers: `<div data-smartbook-form="form_id"></div>`
 * - Form markers are inserted at cursor position
 * - Markers are preserved in HTML output
 * 
 * @section Usage
 * 
 * @code
 * ContentEditor* editor = new ContentEditor(parent);
 * editor->loadContent(htmlContent);
 * 
 * // Apply formatting
 * editor->setBold(true);
 * editor->setTextColor(QColor(255, 0, 0));
 * 
 * // Get edited content
 * QString html = editor->getContent();
 * @endcode
 * 
 * @section Migration
 * 
 * This class was migrated from Qt WebEngine (QWebEngineView) to QTextEdit/QPlainTextEdit
 * architecture in Phase 2 of the content rendering migration (2025-12-27).
 * 
 * **Key Changes:**
 * - Replaced QWebEngineView with QTextEdit (WYSIWYG) and QPlainTextEdit (HTML source)
 * - Removed async JavaScript execution (now synchronous)
 * - Removed content caching (QTextEdit is synchronous)
 * - Implemented rich text formatting using QTextCursor and QTextCharFormat
 * 
 * @section Limitations
 * 
 * **HTML Support:**
 * - Limited to HTML4 + CSS 2.1 subset (QTextEdit limitations)
 * - No HTML5 features (semantic elements, canvas, video, audio)
 * - No CSS3 features (flexbox, grid, transforms, animations)
 * 
 * **WYSIWYG Capabilities:**
 * - Basic rich text formatting supported
 * - Advanced layout features not available
 * - Complex CSS styling may not render correctly in editor
 * 
 * @see PageManager
 * @see qtextedit-wysiwyg-capabilities.adoc
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
     * @return HTML content (synchronous, no caching needed)
     */
    QString getContent() const;
    
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
     * @param enabled true for HTML mode (QPlainTextEdit), false for WYSIWYG mode (QTextEdit)
     */
    void setHtmlMode(bool enabled);

    /**
     * @brief Check if in HTML editing mode
     * @return true if in HTML mode, false if in WYSIWYG mode
     */
    bool isHtmlMode() const { return m_htmlMode; }

    /**
     * @brief Toggle preview mode
     * @param enabled true to show preview (read-only), false to show editor
     */
    void setPreviewMode(bool enabled);

    /**
     * @brief Check if in preview mode
     * @return true if in preview mode
     */
    bool isPreviewMode() const { return m_previewMode; }
    
    // Rich text formatting methods
    void setBold(bool enabled);
    void setItalic(bool enabled);
    void setUnderline(bool enabled);
    void setFontFamily(const QString& family);
    void setFontSize(int size);
    void setTextColor(const QColor& color);
    void setAlignment(Qt::Alignment alignment);
    void insertList(bool ordered);
    void insertLink(const QString& url = QString());
    void insertImage(const QString& path = QString());

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

private slots:
    void onContentChanged();
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);

private:
    void setupUI();
    void setupToolbar();
    void syncContentBetweenModes();
    QTextEdit* getCurrentEditor() const;
    
    QTextEdit* m_textEdit;          // WYSIWYG editor
    QPlainTextEdit* m_htmlEdit;     // HTML source editor
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
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_CONTENTEDITOR_H
