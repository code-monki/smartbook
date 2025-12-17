#ifndef SMARTBOOK_CREATOR_CONTENTEDITOR_H
#define SMARTBOOK_CREATOR_CONTENTEDITOR_H

#include <QWidget>
#include <QWebEngineView>

namespace smartbook {
namespace creator {

/**
 * @brief Content editor widget
 * 
 * WYSIWYG HTML editor using Qt WebEngineView for content authoring.
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
     */
    QString getContent() const;

private:
    QWebEngineView* m_webView;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_CONTENTEDITOR_H
