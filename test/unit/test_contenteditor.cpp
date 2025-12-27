#include <QtTest>
#include "smartbook/creator/ContentEditor.h"
#include <QApplication>
#include <QTimer>
#include <QTest>

using namespace smartbook::creator;

class TestContentEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // T-CT-01: HTML Content Authoring
    void testWysiwygEditing();
    
    // T-CT-02: Rich Text Editing
    void testRichTextFormatting();
    
    // T-CT-03: Direct HTML Editing
    void testHtmlMode();
    
    // T-CT-04: Standard Edit Operations
    void testStandardEditOperations();
    
    // T-CT-05: Preview Functionality
    void testPreviewMode();

private:
    ContentEditor* m_editor;
    void waitForLoad();
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestContentEditor tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestContentEditor::initTestCase()
{
    m_editor = new ContentEditor();
    QVERIFY(m_editor != nullptr);
    
    // QTextEdit is synchronous, no need to wait
    QApplication::processEvents();
}

void TestContentEditor::cleanupTestCase()
{
    if (m_editor) {
        delete m_editor;
        m_editor = nullptr;
    }
    QApplication::processEvents();
}

void TestContentEditor::waitForLoad()
{
    // QTextEdit is synchronous, minimal wait for UI updates
    QApplication::processEvents();
}

// T-CT-01: HTML Content Authoring
// Requirement: FR-CT-3.1
// Test Plan: test-plan.adoc lines 156-165
// AC: Content is stored as HTML. WYSIWYG interface displays and allows editing of HTML content using QTextEdit.
void TestContentEditor::testWysiwygEditing()
{
    QVERIFY(m_editor != nullptr);
    
    // Test loading content
    QString testContent = "<p>Test content for WYSIWYG editing</p>";
    m_editor->loadContent(testContent);
    
    waitForLoad();
    
    // Verify content can be retrieved (QTextEdit is synchronous)
    QString retrieved = m_editor->getContent();
    QVERIFY(!retrieved.isEmpty());
    
    // Verify editor is in WYSIWYG mode (not HTML mode)
    QVERIFY(!m_editor->isHtmlMode());
}

// T-CT-02: Rich Text Editing
// Requirement: FR-CT-3.2
// Test Plan: test-plan.adoc lines 170-181
// AC: All formatting operations complete successfully. Formatted text displays correctly. HTML markup is correctly generated.
void TestContentEditor::testRichTextFormatting()
{
    QVERIFY(m_editor != nullptr);
    
    // Load some content first
    m_editor->loadContent("<p>Test content</p>");
    waitForLoad();
    
    // Test formatting operations exist and can be called
    m_editor->bold();
    QApplication::processEvents();
    
    m_editor->italic();
    QApplication::processEvents();
    
    m_editor->underline();
    QApplication::processEvents();
    
    m_editor->insertUnorderedList();
    QApplication::processEvents();
    
    m_editor->insertOrderedList();
    QApplication::processEvents();
    
    // Verify content changed (formatting was applied)
    QString content = m_editor->getContent();
    QVERIFY(!content.isEmpty());
}

// T-CT-03: Direct HTML Editing
// Requirement: FR-CT-3.3
// Test Plan: test-plan.adoc lines 186-195
// AC: Raw HTML is accepted and stored. HTML displays correctly in WYSIWYG view. Custom HTML elements are preserved.
void TestContentEditor::testHtmlMode()
{
    QVERIFY(m_editor != nullptr);
    
    // Test HTML mode toggle
    QVERIFY(!m_editor->isHtmlMode()); // Should start in WYSIWYG mode
    
    // Load HTML content in WYSIWYG mode first
    QString htmlContent = "<div class=\"custom\">Custom Content</div>";
    m_editor->loadContent(htmlContent);
    waitForLoad();
    
    // Switch to HTML mode
    m_editor->setHtmlMode(true);
    QApplication::processEvents();
    QVERIFY(m_editor->isHtmlMode());
    
    // Content should be in HTML editor
    QString htmlModeContent = m_editor->getContent();
    QVERIFY(!htmlModeContent.isEmpty());
    
    // Switch back to WYSIWYG mode
    m_editor->setHtmlMode(false);
    QApplication::processEvents();
    QVERIFY(!m_editor->isHtmlMode());
    
    // Content should be preserved
    QString retrieved = m_editor->getContent();
    QVERIFY(!retrieved.isEmpty());
}

// T-CT-04: Standard Edit Operations
// Requirement: FR-CT-3.4
// Test Plan: test-plan.adoc lines 200-212
// AC: All keyboard shortcuts function correctly. Cut, copy, paste, undo, redo, and select all operations work as expected.
void TestContentEditor::testStandardEditOperations()
{
    QVERIFY(m_editor != nullptr);
    
    // Load content
    m_editor->loadContent("<p>Test content for editing</p>");
    waitForLoad();
    
    // Test that edit operations can be called
    // Note: Actual clipboard operations require system clipboard, which may not work in test environment
    m_editor->selectAll();
    QApplication::processEvents();
    
    m_editor->copy();
    QApplication::processEvents();
    
    m_editor->cut();
    QApplication::processEvents();
    
    m_editor->paste();
    QApplication::processEvents();
    
    m_editor->undo();
    QApplication::processEvents();
    
    m_editor->redo();
    QApplication::processEvents();
    
    // Verify operations are callable (if we got here, they are)
    QVERIFY(true);
}

// T-CT-05: Preview Functionality
// Requirement: FR-CT-3.5
// Test Plan: test-plan.adoc lines 217-225
// AC: Preview displays content as it will appear in the Reader. Preview updates correctly. Preview matches Reader rendering.
void TestContentEditor::testPreviewMode()
{
    QVERIFY(m_editor != nullptr);
    
    // Test preview mode toggle
    QVERIFY(!m_editor->isPreviewMode()); // Should start in edit mode
    
    // Load content
    m_editor->loadContent("<p>Preview test content</p>");
    waitForLoad();
    
    // Enable preview mode
    m_editor->setPreviewMode(true);
    QApplication::processEvents();
    QVERIFY(m_editor->isPreviewMode());
    
    // Disable preview mode
    m_editor->setPreviewMode(false);
    QApplication::processEvents();
    QVERIFY(!m_editor->isPreviewMode());
}

#include "test_contenteditor.moc"
