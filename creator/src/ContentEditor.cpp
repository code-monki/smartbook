#include "smartbook/creator/ContentEditor.h"
#include "smartbook/creator/PageManager.h"
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QKeySequence>
#include <QApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaType>
#include <QDebug>

namespace smartbook {
namespace creator {

ContentEditor::ContentEditor(QWidget* parent)
    : QWidget(parent)
    , m_webView(nullptr)
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
    setupWebEngine();
    setupEditorHTML();
}

ContentEditor::~ContentEditor() {
    if (m_webView) {
        disconnect(m_webView, nullptr, this, nullptr);
        delete m_webView;
        m_webView = nullptr;
    }
}

void ContentEditor::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar(this);
    layout->addWidget(m_toolbar);

    // WebEngine view
    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);

    connect(m_webView, &QWebEngineView::loadFinished,
            this, &ContentEditor::onLoadFinished);
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
    m_toolbar->addAction("Insert Link", this, &ContentEditor::insertLink);
    m_toolbar->addAction("Insert Image", this, &ContentEditor::insertImage);

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

void ContentEditor::setupWebEngine() {
    QWebEnginePage* page = m_webView->page();
    QWebEngineSettings* settings = page->settings();
    
    // Enable JavaScript
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    
    // Enable local content access
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
}

void ContentEditor::setupEditorHTML() {
    QString editorHTML = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            font-size: 14px;
            line-height: 1.6;
            padding: 20px;
            max-width: 800px;
            margin: 0 auto;
        }
        #editor {
            min-height: 400px;
            border: 1px solid #ddd;
            padding: 15px;
            outline: none;
        }
        #editor:focus {
            border-color: #0066cc;
        }
        #htmlEditor {
            width: 100%;
            min-height: 400px;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            border: 1px solid #ddd;
            padding: 15px;
            display: none;
        }
        #htmlEditor:focus {
            border-color: #0066cc;
            outline: none;
        }
    </style>
</head>
<body>
    <div id="editor" contenteditable="true"></div>
    <textarea id="htmlEditor"></textarea>
    <script>
        const editor = document.getElementById('editor');
        const htmlEditor = document.getElementById('htmlEditor');
        let isHtmlMode = false;
        
        // Content change detection
        function notifyContentChanged() {
            // Store content for retrieval
            const content = getContent();
            if (window.qt && window.qt.webChannelTransport) {
                window.qt.webChannelTransport.send({
                    type: 'contentChanged',
                    content: content
                });
            }
        }
        
        editor.addEventListener('input', notifyContentChanged);
        htmlEditor.addEventListener('input', notifyContentChanged);
        editor.addEventListener('paste', function(e) {
            e.preventDefault();
            const text = (e.clipboardData || window.clipboardData).getData('text/plain');
            document.execCommand('insertText', false, text);
            notifyContentChanged();
        });
        
        // Formatting commands
        window.formatBold = function() {
            document.execCommand('bold', false, null);
            notifyContentChanged();
        };
        
        window.formatItalic = function() {
            document.execCommand('italic', false, null);
            notifyContentChanged();
        };
        
        window.formatUnderline = function() {
            document.execCommand('underline', false, null);
            notifyContentChanged();
        };
        
        window.insertUnorderedList = function() {
            document.execCommand('insertUnorderedList', false, null);
            notifyContentChanged();
        };
        
        window.insertOrderedList = function() {
            document.execCommand('insertOrderedList', false, null);
            notifyContentChanged();
        };
        
        window.insertLink = function() {
            const url = prompt('Enter URL:');
            if (url) {
                document.execCommand('createLink', false, url);
                notifyContentChanged();
            }
        };
        
        window.insertImage = function() {
            const url = prompt('Enter image URL:');
            if (url) {
                document.execCommand('insertImage', false, url);
                notifyContentChanged();
            }
        };
        
        window.getContent = function() {
            if (isHtmlMode) {
                return htmlEditor.value;
            }
            return editor.innerHTML;
        };
        
        window.setContent = function(html) {
            if (isHtmlMode) {
                htmlEditor.value = html;
            } else {
                editor.innerHTML = html;
            }
            notifyContentChanged();
        };
        
        window.setHtmlMode = function(enabled) {
            isHtmlMode = enabled;
            if (enabled) {
                editor.style.display = 'none';
                htmlEditor.style.display = 'block';
                htmlEditor.value = editor.innerHTML;
                htmlEditor.focus();
            } else {
                htmlEditor.style.display = 'none';
                editor.style.display = 'block';
                editor.innerHTML = htmlEditor.value;
                editor.focus();
            }
        };
        
        window.setPreviewMode = function(enabled) {
            if (enabled) {
                editor.contentEditable = 'false';
                htmlEditor.style.display = 'none';
                editor.style.display = 'block';
            } else {
                editor.contentEditable = 'true';
                if (!isHtmlMode) {
                    editor.style.display = 'block';
                }
            }
        };
        
        // Undo/Redo support
        window.canUndo = function() {
            return document.queryCommandEnabled('undo');
        };
        
        window.canRedo = function() {
            return document.queryCommandEnabled('redo');
        };
        
        window.undo = function() {
            document.execCommand('undo', false, null);
            notifyContentChanged();
        };
        
        window.redo = function() {
            document.execCommand('redo', false, null);
            notifyContentChanged();
        };
        
        window.insertFormMarker = function(formId) {
            var markerHtml = '<div data-smartbook-form="' + formId + '"></div>';
            var selection = window.getSelection();
            if (selection.rangeCount > 0) {
                var range = selection.getRangeAt(0);
                range.deleteContents();
                var div = document.createElement('div');
                div.innerHTML = markerHtml;
                var fragment = document.createDocumentFragment();
                while (div.firstChild) {
                    fragment.appendChild(div.firstChild);
                }
                range.insertNode(fragment);
                notifyContentChanged();
            } else {
                // Insert at end if no selection
                var editor = document.getElementById('editor');
                if (editor) {
                    editor.innerHTML += markerHtml;
                    notifyContentChanged();
                }
            }
        };
    </script>
</body>
</html>)";

    m_webView->setHtml(editorHTML);
}

void ContentEditor::onLoadFinished(bool success) {
    if (success && !m_currentContent.isEmpty()) {
        // Escape HTML content for JavaScript
        QString escaped = m_currentContent;
        escaped.replace("\\", "\\\\");
        escaped.replace("'", "\\'");
        escaped.replace("\n", "\\n");
        escaped.replace("\r", "\\r");
        executeJavaScript(QString("setContent('%1');").arg(escaped));
    }
}

void ContentEditor::loadContent(const QString& htmlContent) {
    m_currentContent = htmlContent;
    if (!m_webView || !m_webView->page() || m_webView->page()->url().isEmpty()) {
        // Page not loaded yet, will be set in onLoadFinished
        return;
    }
    // Escape HTML content for JavaScript
    QString escaped = htmlContent;
    escaped.replace("\\", "\\\\");
    escaped.replace("'", "\\'");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    executeJavaScript(QString("setContent('%1');").arg(escaped));
}

QString ContentEditor::getContent() const {
    // Note: This is a const method, but we need to get content from JavaScript
    // In a real implementation, we'd use a WebChannel for async communication
    // For now, return cached content or use a workaround
    return m_currentContent;
}

void ContentEditor::setHtmlMode(bool enabled) {
    m_htmlMode = enabled;
    m_htmlModeAction->setChecked(enabled);
    executeJavaScript(QString("setHtmlMode(%1);").arg(enabled ? "true" : "false"));
}

void ContentEditor::setPreviewMode(bool enabled) {
    m_previewMode = enabled;
    m_previewAction->setChecked(enabled);
    executeJavaScript(QString("setPreviewMode(%1);").arg(enabled ? "true" : "false"));
}

void ContentEditor::cut() {
    executeJavaScript("document.execCommand('cut', false, null);");
}

void ContentEditor::copy() {
    executeJavaScript("document.execCommand('copy', false, null);");
}

void ContentEditor::paste() {
    executeJavaScript("document.execCommand('paste', false, null);");
}

void ContentEditor::undo() {
    executeJavaScript("undo();");
    emit contentChanged();
}

void ContentEditor::redo() {
    executeJavaScript("redo();");
    emit contentChanged();
}

void ContentEditor::selectAll() {
    executeJavaScript("document.execCommand('selectAll', false, null);");
}

void ContentEditor::bold() {
    executeJavaScript("formatBold();");
    emit contentChanged();
}

void ContentEditor::italic() {
    executeJavaScript("formatItalic();");
    emit contentChanged();
}

void ContentEditor::underline() {
    executeJavaScript("formatUnderline();");
    emit contentChanged();
}

void ContentEditor::insertUnorderedList() {
    executeJavaScript("insertUnorderedList();");
    emit contentChanged();
}

void ContentEditor::insertOrderedList() {
    executeJavaScript("insertOrderedList();");
    emit contentChanged();
}

void ContentEditor::insertLink() {
    executeJavaScript("insertLink();");
    emit contentChanged();
}

void ContentEditor::insertImage() {
    executeJavaScript("insertImage();");
    emit contentChanged();
}

void ContentEditor::onContentChanged() {
    // Update content cache when content changes
    updateContentCache();
    emit contentChanged();
}

void ContentEditor::updateContentCache() {
    if (!m_webView || !m_webView->page()) {
        return;
    }
    
    // Get content from JavaScript asynchronously
    m_webView->page()->runJavaScript("getContent();", [this](const QVariant& result) {
        if (result.isValid() && result.typeId() == QMetaType::QString) {
            m_currentContent = result.toString();
        }
    });
}

void ContentEditor::updateContentFromJavaScript(const QString& content) {
    m_currentContent = content;
}

void ContentEditor::executeJavaScript(const QString& script) {
    if (m_webView && m_webView->page()) {
        m_webView->page()->runJavaScript(script);
    }
}

QString ContentEditor::getContentForSave() {
    // Update cache first, then return
    updateContentCache();
    // Process events to allow async update to complete
    QApplication::processEvents();
    // Note: Content is retrieved asynchronously, so m_currentContent
    // may not be immediately updated. In production, this should use
    // WebChannel for synchronous content retrieval.
    return m_currentContent;
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
    
    QString marker = QString(R"(<div data-smartbook-form="%1"></div>)").arg(formId);
    QString script = QString("insertFormMarker('%1');").arg(marker);
    executeJavaScript(script);
    emit contentChanged();
    return true;
}

} // namespace creator
} // namespace smartbook
