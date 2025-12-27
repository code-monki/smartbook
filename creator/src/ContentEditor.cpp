#include "smartbook/creator/ContentEditor.h"
#include "smartbook/creator/PageManager.h"
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QTextListFormat>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextList>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QToolBar>
#include <QAction>
#include <QKeySequence>
#include <QColorDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QApplication>
#include <QDebug>

namespace smartbook {
namespace creator {

ContentEditor::ContentEditor(QWidget* parent)
    : QWidget(parent)
    , m_textEdit(nullptr)
    , m_htmlEdit(nullptr)
    , m_toolbar(nullptr)
    , m_boldAction(nullptr)
    , m_italicAction(nullptr)
    , m_underlineAction(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_htmlModeAction(nullptr)
    , m_previewAction(nullptr)
    , m_htmlMode(false)
    , m_previewMode(false)
{
    setupUI();
    setupToolbar();
}

ContentEditor::~ContentEditor() {
    // Widgets are cleaned up by Qt's parent-child relationship
}

void ContentEditor::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar(this);
    layout->addWidget(m_toolbar);

    // Stacked widget to hold WYSIWYG and HTML editors
    QStackedWidget* stacked = new QStackedWidget(this);
    
    // WYSIWYG editor (QTextEdit)
    m_textEdit = new QTextEdit(this);
    m_textEdit->setAcceptRichText(true);
    connect(m_textEdit, &QTextEdit::textChanged, this, &ContentEditor::onContentChanged);
    connect(m_textEdit, &QTextEdit::undoAvailable, this, &ContentEditor::onUndoAvailable);
    connect(m_textEdit, &QTextEdit::redoAvailable, this, &ContentEditor::onRedoAvailable);
    stacked->addWidget(m_textEdit);
    
    // HTML source editor (QPlainTextEdit)
    m_htmlEdit = new QPlainTextEdit(this);
    m_htmlEdit->setFont(QFont("Courier New", 10));
    connect(m_htmlEdit, &QPlainTextEdit::textChanged, this, &ContentEditor::onContentChanged);
    connect(m_htmlEdit, &QPlainTextEdit::undoAvailable, this, &ContentEditor::onUndoAvailable);
    connect(m_htmlEdit, &QPlainTextEdit::redoAvailable, this, &ContentEditor::onRedoAvailable);
    stacked->addWidget(m_htmlEdit);
    
    layout->addWidget(stacked);
    
    // Start with WYSIWYG editor visible
    stacked->setCurrentWidget(m_textEdit);
}

void ContentEditor::setupToolbar() {
    // Undo/Redo
    m_undoAction = m_toolbar->addAction("Undo", this, &ContentEditor::undo);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    
    m_redoAction = m_toolbar->addAction("Redo", this, &ContentEditor::redo);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false);

    m_toolbar->addSeparator();

    // Formatting
    m_boldAction = m_toolbar->addAction("Bold", this, &ContentEditor::bold);
    m_boldAction->setShortcut(QKeySequence("Ctrl+B"));
    m_boldAction->setCheckable(true);

    m_italicAction = m_toolbar->addAction("Italic", this, &ContentEditor::italic);
    m_italicAction->setShortcut(QKeySequence("Ctrl+I"));
    m_italicAction->setCheckable(true);

    m_underlineAction = m_toolbar->addAction("Underline", this, &ContentEditor::underline);
    m_underlineAction->setShortcut(QKeySequence("Ctrl+U"));
    m_underlineAction->setCheckable(true);

    m_toolbar->addSeparator();

    // Lists
    m_toolbar->addAction("Unordered List", this, &ContentEditor::insertUnorderedList);
    m_toolbar->addAction("Ordered List", this, &ContentEditor::insertOrderedList);

    m_toolbar->addSeparator();

    // Links and Images
    m_toolbar->addAction("Insert Link", this, [this]() { insertLink(QString()); });
    m_toolbar->addAction("Insert Image", this, [this]() { insertImage(QString()); });

    m_toolbar->addSeparator();

    // Mode toggles
    m_htmlModeAction = m_toolbar->addAction("HTML Mode", this, [this]() {
        setHtmlMode(!m_htmlMode);
    });
    m_htmlModeAction->setCheckable(true);

    m_previewAction = m_toolbar->addAction("Preview", this, [this]() {
        setPreviewMode(!m_previewMode);
    });
    m_previewAction->setCheckable(true);
}

void ContentEditor::loadContent(const QString& htmlContent) {
    if (m_htmlMode) {
        // HTML mode: load into plain text editor
        m_htmlEdit->setPlainText(htmlContent);
    } else {
        // WYSIWYG mode: load into rich text editor
        m_textEdit->setHtml(htmlContent);
    }
}

QString ContentEditor::getContent() const {
    if (m_htmlMode) {
        return m_htmlEdit->toPlainText();
    } else {
        return m_textEdit->toHtml();
    }
}

QString ContentEditor::getContentForSave() {
    return getContent();
}

void ContentEditor::setHtmlMode(bool enabled) {
    if (m_htmlMode == enabled) {
        return;
    }
    
    // Sync content between modes before switching
    syncContentBetweenModes();
    
    m_htmlMode = enabled;
    m_htmlModeAction->setChecked(enabled);
    
    // Switch visible editor
    QStackedWidget* stacked = qobject_cast<QStackedWidget*>(m_textEdit->parent());
    if (stacked) {
        if (enabled) {
            stacked->setCurrentWidget(m_htmlEdit);
        } else {
            stacked->setCurrentWidget(m_textEdit);
        }
    }
}

void ContentEditor::setPreviewMode(bool enabled) {
    m_previewMode = enabled;
    m_previewAction->setChecked(enabled);
    
    // Set read-only state
    m_textEdit->setReadOnly(enabled);
    m_htmlEdit->setReadOnly(enabled);
}

void ContentEditor::syncContentBetweenModes() {
    if (m_htmlMode) {
        // Switching from HTML to WYSIWYG: convert HTML to rich text
        QString html = m_htmlEdit->toPlainText();
        m_textEdit->setHtml(html);
    } else {
        // Switching from WYSIWYG to HTML: convert rich text to HTML
        QString html = m_textEdit->toHtml();
        m_htmlEdit->setPlainText(html);
    }
}

QTextEdit* ContentEditor::getCurrentEditor() const {
    if (m_htmlMode) {
        return nullptr; // HTML mode uses QPlainTextEdit
    }
    return m_textEdit;
}

void ContentEditor::onContentChanged() {
    emit contentChanged();
}

void ContentEditor::onUndoAvailable(bool available) {
    if (m_undoAction) {
        m_undoAction->setEnabled(available);
    }
    emit undoAvailable(available);
}

void ContentEditor::onRedoAvailable(bool available) {
    if (m_redoAction) {
        m_redoAction->setEnabled(available);
    }
    emit redoAvailable(available);
}

void ContentEditor::cut() {
    QTextEdit* editor = getCurrentEditor();
    if (editor) {
        editor->cut();
    } else if (m_htmlMode) {
        m_htmlEdit->cut();
    }
}

void ContentEditor::copy() {
    QTextEdit* editor = getCurrentEditor();
    if (editor) {
        editor->copy();
    } else if (m_htmlMode) {
        m_htmlEdit->copy();
    }
}

void ContentEditor::paste() {
    QTextEdit* editor = getCurrentEditor();
    if (editor) {
        editor->paste();
    } else if (m_htmlMode) {
        m_htmlEdit->paste();
    }
}

void ContentEditor::undo() {
    QTextEdit* editor = getCurrentEditor();
    if (editor) {
        editor->undo();
    } else if (m_htmlMode) {
        m_htmlEdit->undo();
    }
    emit contentChanged();
}

void ContentEditor::redo() {
    QTextEdit* editor = getCurrentEditor();
    if (editor) {
        editor->redo();
    } else if (m_htmlMode) {
        m_htmlEdit->redo();
    }
    emit contentChanged();
}

void ContentEditor::selectAll() {
    QTextEdit* editor = getCurrentEditor();
    if (editor) {
        editor->selectAll();
    } else if (m_htmlMode) {
        m_htmlEdit->selectAll();
    }
}

void ContentEditor::bold() {
    setBold(!m_boldAction->isChecked());
}

void ContentEditor::italic() {
    setItalic(!m_italicAction->isChecked());
}

void ContentEditor::underline() {
    setUnderline(!m_underlineAction->isChecked());
}

void ContentEditor::setBold(bool enabled) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontWeight(enabled ? QFont::Bold : QFont::Normal);
    cursor.setCharFormat(format);
    editor->setTextCursor(cursor);
    
    m_boldAction->setChecked(enabled);
    emit contentChanged();
}

void ContentEditor::setItalic(bool enabled) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontItalic(enabled);
    cursor.setCharFormat(format);
    editor->setTextCursor(cursor);
    
    m_italicAction->setChecked(enabled);
    emit contentChanged();
}

void ContentEditor::setUnderline(bool enabled) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setUnderlineStyle(enabled ? QTextCharFormat::SingleUnderline : QTextCharFormat::NoUnderline);
    cursor.setCharFormat(format);
    editor->setTextCursor(cursor);
    
    m_underlineAction->setChecked(enabled);
    emit contentChanged();
}

void ContentEditor::setFontFamily(const QString& family) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontFamilies({family});
    cursor.setCharFormat(format);
    editor->setTextCursor(cursor);
    
    emit contentChanged();
}

void ContentEditor::setFontSize(int size) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setFontPointSize(size);
    cursor.setCharFormat(format);
    editor->setTextCursor(cursor);
    
    emit contentChanged();
}

void ContentEditor::setTextColor(const QColor& color) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextCharFormat format = cursor.charFormat();
    format.setForeground(color);
    cursor.setCharFormat(format);
    editor->setTextCursor(cursor);
    
    emit contentChanged();
}

void ContentEditor::setAlignment(Qt::Alignment alignment) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextBlockFormat format = cursor.blockFormat();
    format.setAlignment(alignment);
    cursor.setBlockFormat(format);
    editor->setTextCursor(cursor);
    
    emit contentChanged();
}

void ContentEditor::insertList(bool ordered) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QTextCursor cursor = editor->textCursor();
    QTextListFormat listFormat;
    if (ordered) {
        listFormat.setStyle(QTextListFormat::ListDecimal);
    } else {
        listFormat.setStyle(QTextListFormat::ListDisc);
    }
    cursor.createList(listFormat);
    
    emit contentChanged();
}

void ContentEditor::insertUnorderedList() {
    insertList(false);
}

void ContentEditor::insertOrderedList() {
    insertList(true);
}

void ContentEditor::insertLink(const QString& url) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QString linkUrl = url;
    if (linkUrl.isEmpty()) {
        bool ok;
        linkUrl = QInputDialog::getText(this, "Insert Link", "Enter URL:", QLineEdit::Normal, "", &ok);
        if (!ok || linkUrl.isEmpty()) {
            return;
        }
    }
    
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection()) {
        QTextCharFormat format = cursor.charFormat();
        format.setAnchor(true);
        format.setAnchorHref(linkUrl);
        format.setForeground(QColor(0, 0, 255));
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        cursor.setCharFormat(format);
    } else {
        cursor.insertHtml(QString("<a href=\"%1\">%1</a>").arg(linkUrl));
    }
    
    emit contentChanged();
}

void ContentEditor::insertImage(const QString& path) {
    QTextEdit* editor = getCurrentEditor();
    if (!editor) return;
    
    QString imagePath = path;
    if (imagePath.isEmpty()) {
        imagePath = QFileDialog::getOpenFileName(this, "Insert Image", "", "Images (*.png *.jpg *.jpeg *.gif *.bmp)");
        if (imagePath.isEmpty()) {
            return;
        }
    }
    
    QTextCursor cursor = editor->textCursor();
    QTextImageFormat imageFormat;
    imageFormat.setName(imagePath);
    cursor.insertImage(imageFormat);
    
    emit contentChanged();
}

bool ContentEditor::saveToPage(PageManager* pageManager, int pageId) {
    if (!pageManager) {
        qWarning() << "PageManager is null for save operation";
        return false;
    }
    
    QString content = getContentForSave();
    return pageManager->updatePageContent(pageId, content);
}

bool ContentEditor::insertFormMarker(const QString& formId) {
    if (formId.isEmpty()) {
        qWarning() << "Cannot insert form marker: formId is empty";
        return false;
    }
    
    QTextEdit* editor = getCurrentEditor();
    if (!editor) {
        qWarning() << "Cannot insert form marker: not in WYSIWYG mode";
        return false;
    }
    
    QString marker = QString(R"(<div data-smartbook-form="%1"></div>)").arg(formId);
    QTextCursor cursor = editor->textCursor();
    cursor.insertHtml(marker);
    
    emit contentChanged();
    return true;
}

} // namespace creator
} // namespace smartbook
