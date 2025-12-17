#include "smartbook/creator/ContentEditor.h"
#include <QWebEngineView>
#include <QVBoxLayout>

namespace smartbook {
namespace creator {

ContentEditor::ContentEditor(QWidget* parent)
    : QWidget(parent)
    , m_webView(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);

    // TODO: Setup WYSIWYG editor (contentEditable, etc.)
}

ContentEditor::~ContentEditor() {
}

void ContentEditor::loadContent(const QString& htmlContent) {
    // TODO: Load HTML into editor
    m_webView->setHtml(htmlContent);
}

QString ContentEditor::getContent() const {
    // TODO: Extract HTML from editor
    return QString();
}

} // namespace creator
} // namespace smartbook
