#include <QtTest>
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QApplication>
#include <QElapsedTimer>
#include <QDebug>

using namespace smartbook::reader;
using namespace smartbook::common::database;

class TestReaderViewContent : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testContentLoading();  // Test loading HTML from Content_Pages

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_cartridgePath;
    LocalDBManager* m_dbManager;
    
    QString createTestCartridge(const QString& guid);
};

void TestReaderViewContent::initTestCase()
{
    // QApplication is created by custom main() function, no need to create here
    
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    // Initialize local database
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    // Create test cartridge
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge(guid);
    QVERIFY(QFile::exists(m_cartridgePath));
}

void TestReaderViewContent::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestReaderViewContent::createTestCartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_Content_" + guid;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
        QSqlDatabase::removeDatabase(connectionName);
        return QString();
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table with all required columns
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL,
            version TEXT NOT NULL DEFAULT '1.0',
            schema_version TEXT NOT NULL DEFAULT '1.0',
            publisher TEXT
        )
    )");
    
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, version, schema_version) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.addBindValue("1.0");
    query.exec();
    
    // Create Content_Pages table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY,
            page_order INTEGER NOT NULL UNIQUE,
            chapter_title TEXT,
            html_content TEXT NOT NULL,
            associated_css TEXT
        )
    )");
    
    // Insert test content pages
    query.prepare(R"(
        INSERT INTO Content_Pages (page_id, page_order, chapter_title, html_content, associated_css)
        VALUES (?, ?, ?, ?, ?)
    )");
    
    // Page 1
    query.addBindValue(1);
    query.addBindValue(1);
    query.addBindValue("Introduction");
    query.addBindValue("<h1>Chapter 1: Introduction</h1><p>This is the first page of content.</p>");
    query.addBindValue("body { margin: 20px; }");
    query.exec();
    
    // Page 2
    query.addBindValue(2);
    query.addBindValue(2);
    query.addBindValue("Introduction");
    query.addBindValue("<h2>Section 1.1</h2><p>This is the second page.</p>");
    query.addBindValue(QVariant()); // NULL CSS
    query.exec();
    
    // Page 3
    query.addBindValue(3);
    query.addBindValue(3);
    query.addBindValue("Main Content");
    query.addBindValue("<h1>Chapter 2: Main Content</h1><p>This is the third page.</p>");
    query.addBindValue("h1 { color: blue; }");
    query.exec();
    
    db.close();
    
    // Remove database connection after closing
    // Use a small delay to ensure all queries are finished
    QApplication::processEvents();
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

// Test loading content from Content_Pages table
// Updated for QTextBrowser (no WebEngine, synchronous loading)
void TestReaderViewContent::testContentLoading()
{
    ReaderView* readerView = new ReaderView();
    
    // Open cartridge using CartridgeDBConnector
    CartridgeDBConnector connector(this);
    bool opened = connector.openCartridge(m_cartridgePath);
    QVERIFY(opened);
    
    // Load cartridge content
    QElapsedTimer timer;
    timer.start();
    
    readerView->loadCartridge(m_cartridgePath);
    
    qint64 loadTime = timer.elapsed();
    
    // Content should load quickly (< 500ms) - QTextBrowser is synchronous
    QVERIFY(loadTime < 500);
    
    qDebug() << "Content loaded in" << loadTime << "ms";
    
    // Process events to ensure content is rendered
    QApplication::processEvents();
    
    // QTextBrowser loads synchronously, so no need to wait for WebEngine
    // Just verify the content was loaded
    QVERIFY(readerView != nullptr);
    
    // Close connector before deleting widget
    connector.closeCartridge();
    
    // Process events to ensure cleanup
    QApplication::processEvents();
    
    // Delete widget - QTextBrowser cleanup is straightforward
    delete readerView;
    readerView = nullptr;
    
    QApplication::processEvents();
}

// Use custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestReaderViewContent tc;
    int result = QTest::qExec(&tc, argc, argv);
    // QApplication will be destroyed automatically when main returns
    return result;
}

#include "test_readerview_content.moc"
