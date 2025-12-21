#include <QtTest>
#include "smartbook/creator/ContentEditor.h"
#include "smartbook/creator/PageManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>

using namespace smartbook::creator;
using namespace smartbook::common::database;

class TestContentEditorPageManagerIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Integration test: Save content from ContentEditor to PageManager
    void testContentEditorToPageManagerSave();

private:
    QTemporaryDir* m_tempDir;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    ContentEditor* m_editor;
    PageManager* m_pageManager;
    
    QString createTestCartridge();
    void waitForEditorLoad();
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestContentEditorPageManagerIntegration tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestContentEditorPageManagerIntegration::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge();
    QVERIFY(QFile::exists(m_cartridgePath));
    
    m_editor = new ContentEditor();
    m_pageManager = new PageManager(this);
    
    bool opened = m_pageManager->openCartridge(m_cartridgePath);
    QVERIFY(opened);
    
    waitForEditorLoad();
}

void TestContentEditorPageManagerIntegration::cleanupTestCase()
{
    if (m_pageManager) {
        m_pageManager->closeCartridge();
        delete m_pageManager;
        m_pageManager = nullptr;
    }
    
    if (m_editor) {
        delete m_editor;
        m_editor = nullptr;
    }
    
    QApplication::processEvents();
    delete m_tempDir;
}

void TestContentEditorPageManagerIntegration::waitForEditorLoad()
{
    QTest::qWait(500);
    QApplication::processEvents();
    QTest::qWait(200);
    QApplication::processEvents();
}

QString TestContentEditorPageManagerIntegration::createTestCartridge()
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_Integration_" + m_cartridgeGuid;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
        QSqlDatabase::removeDatabase(connectionName);
        return QString();
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL
        )
    )");
    
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year) VALUES (?, ?, ?, ?)");
    query.addBindValue(m_cartridgeGuid);
    query.addBindValue("Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    // Create Content_Pages table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_order INTEGER NOT NULL UNIQUE,
            chapter_title TEXT,
            html_content TEXT NOT NULL,
            associated_css TEXT
        )
    )");
    
    // Insert initial page
    query.prepare(R"(
        INSERT INTO Content_Pages (page_order, chapter_title, html_content, associated_css)
        VALUES (?, ?, ?, ?)
    )");
    
    query.addBindValue(1);
    query.addBindValue("Introduction");
    query.addBindValue("<p>Initial content</p>");
    query.addBindValue("");
    query.exec();
    
    db.close();
    QApplication::processEvents();
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

// Integration test: Save content from ContentEditor to PageManager
void TestContentEditorPageManagerIntegration::testContentEditorToPageManagerSave()
{
    QVERIFY(m_editor != nullptr);
    QVERIFY(m_pageManager != nullptr);
    
    // Get the first page
    QList<PageInfo> pages = m_pageManager->getPages();
    QVERIFY(!pages.isEmpty());
    
    int pageId = pages[0].pageId;
    QVERIFY(pageId > 0);
    
    // Load page content into editor
    PageInfo page = m_pageManager->getPage(pageId);
    m_editor->loadContent(page.htmlContent);
    waitForEditorLoad();
    
    // Edit content (simulate user editing)
    QString newContent = "<p>Updated content from ContentEditor</p>";
    m_editor->loadContent(newContent);
    waitForEditorLoad();
    
    // Save content to page via PageManager
    bool saved = m_editor->saveToPage(m_pageManager, pageId);
    QVERIFY(saved);
    
    // Verify content was saved
    PageInfo updatedPage = m_pageManager->getPage(pageId);
    QCOMPARE(updatedPage.htmlContent, newContent);
}

#include "test_contenteditor_pagemanager_integration.moc"
