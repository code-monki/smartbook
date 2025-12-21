#include <QtTest>
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

class TestPageManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // T-CT-06: Page Selection
    void testPageSelection();
    
    // T-CT-07: Page CRUD Operations
    void testPageCRUD();
    
    // T-CT-08: Page Ordering
    void testPageOrdering();
    
    // T-CT-09: Chapter Organization
    void testChapterOrganization();

private:
    QTemporaryDir* m_tempDir;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    PageManager* m_pageManager;
    
    QString createTestCartridge();
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestPageManager tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestPageManager::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge();
    QVERIFY(QFile::exists(m_cartridgePath));
    
    m_pageManager = new PageManager(this);
    bool opened = m_pageManager->openCartridge(m_cartridgePath);
    QVERIFY(opened);
}

void TestPageManager::cleanupTestCase()
{
    if (m_pageManager) {
        m_pageManager->closeCartridge();
        delete m_pageManager;
        m_pageManager = nullptr;
    }
    delete m_tempDir;
}

QString TestPageManager::createTestCartridge()
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_PageManager_" + m_cartridgeGuid;
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
    
    // Insert test pages
    query.prepare(R"(
        INSERT INTO Content_Pages (page_order, chapter_title, html_content, associated_css)
        VALUES (?, ?, ?, ?)
    )");
    
    query.addBindValue(1);
    query.addBindValue("Introduction");
    query.addBindValue("<p>Introduction content</p>");
    query.addBindValue("");
    query.exec();
    
    query.addBindValue(2);
    query.addBindValue(QVariant()); // NULL chapter title
    query.addBindValue("<p>Page 2 content</p>");
    query.addBindValue("");
    query.exec();
    
    query.addBindValue(3);
    query.addBindValue("Chapter 1");
    query.addBindValue("<p>Chapter 1 content</p>");
    query.addBindValue("");
    query.exec();
    
    db.close();
    QApplication::processEvents();
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

// T-CT-06: Page Selection
// Requirement: FR-CT-3.6
// Test Plan: test-plan.adoc lines 233-240
// AC: Selected page loads in the editor. Page selection mechanism functions correctly.
void TestPageManager::testPageSelection()
{
    QVERIFY(m_pageManager != nullptr);
    
    // Get all pages
    QList<PageInfo> pages = m_pageManager->getPages();
    QVERIFY(pages.size() >= 3);
    
    // Test selecting a page
    int firstPageId = pages[0].pageId;
    m_pageManager->setCurrentPage(firstPageId);
    QCOMPARE(m_pageManager->getCurrentPageId(), firstPageId);
    
    // Test getting page by ID
    PageInfo page = m_pageManager->getPage(firstPageId);
    QVERIFY(page.isValid());
    QCOMPARE(page.pageId, firstPageId);
}

// T-CT-07: Page CRUD Operations
// Requirement: FR-CT-3.7
// Test Plan: test-plan.adoc lines 245-255
// AC: New page is created and added to Content_Pages table. Page can be edited and saved. Deleted page is removed.
void TestPageManager::testPageCRUD()
{
    QVERIFY(m_pageManager != nullptr);
    
    // Create a new page
    int newPageId = m_pageManager->createPage("Test Chapter");
    QVERIFY(newPageId > 0);
    
    // Verify page exists
    PageInfo page = m_pageManager->getPage(newPageId);
    QVERIFY(page.isValid());
    QCOMPARE(page.chapterTitle, "Test Chapter");
    
    // Update page content
    QString newContent = "<p>Updated content</p>";
    bool updated = m_pageManager->updatePageContent(newPageId, newContent);
    QVERIFY(updated);
    
    // Verify content was updated
    page = m_pageManager->getPage(newPageId);
    QCOMPARE(page.htmlContent, newContent);
    
    // Delete the page
    bool deleted = m_pageManager->deletePage(newPageId);
    QVERIFY(deleted);
    
    // Verify page no longer exists
    page = m_pageManager->getPage(newPageId);
    QVERIFY(!page.isValid());
}

// T-CT-08: Page Ordering
// Requirement: FR-CT-3.8
// Test Plan: test-plan.adoc lines 260-268
// AC: page_order field is updated correctly. Pages display in the new order. Order persists.
void TestPageManager::testPageOrdering()
{
    QVERIFY(m_pageManager != nullptr);
    
    QList<PageInfo> pages = m_pageManager->getPages();
    QVERIFY(pages.size() >= 2);
    
    // Get page IDs in current order
    QList<int> pageIds;
    for (const PageInfo& page : pages) {
        pageIds.append(page.pageId);
    }
    
    // Reverse the order
    QList<int> reversedIds;
    for (int i = pageIds.size() - 1; i >= 0; --i) {
        reversedIds.append(pageIds[i]);
    }
    
    // Reorder pages
    bool reordered = m_pageManager->reorderPages(reversedIds);
    QVERIFY(reordered);
    
    // Verify new order
    QList<PageInfo> reorderedPages = m_pageManager->getPages();
    QCOMPARE(reorderedPages.size(), pageIds.size());
    
    // Check that first page is now last
    QCOMPARE(reorderedPages.first().pageId, pageIds.last());
    QCOMPARE(reorderedPages.last().pageId, pageIds.first());
}

// T-CT-09: Chapter Organization
// Requirement: FR-CT-3.9
// Test Plan: test-plan.adoc lines 273-282
// AC: chapter_title field is updated. Chapter organization is preserved. Chapter titles display correctly.
void TestPageManager::testChapterOrganization()
{
    QVERIFY(m_pageManager != nullptr);
    
    // Get a page without chapter title
    QList<PageInfo> pages = m_pageManager->getPages();
    int pageId = -1;
    for (const PageInfo& page : pages) {
        if (page.chapterTitle.isEmpty()) {
            pageId = page.pageId;
            break;
        }
    }
    
    QVERIFY(pageId > 0);
    
    // Assign chapter title
    QString chapterTitle = "New Chapter";
    bool updated = m_pageManager->updatePageMetadata(pageId, chapterTitle);
    QVERIFY(updated);
    
    // Verify chapter title was set
    PageInfo page = m_pageManager->getPage(pageId);
    QCOMPARE(page.chapterTitle, chapterTitle);
    
    // Clear chapter title
    updated = m_pageManager->updatePageMetadata(pageId, QString());
    QVERIFY(updated);
    
    // Verify chapter title was cleared
    page = m_pageManager->getPage(pageId);
    QVERIFY(page.chapterTitle.isEmpty());
}

#include "test_pagemanager.moc"
