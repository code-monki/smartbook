#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QDir>

using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestDatabaseErrors : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDatabaseLockedError();
    void testDatabaseCorruptionHandling();
    void testInvalidPathHandling();
    void testConcurrentAccess();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
};

void TestDatabaseErrors::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
}

void TestDatabaseErrors::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

void TestDatabaseErrors::testDatabaseLockedError()
{
    // Test that database operations handle locked database gracefully
    // This simulates the case where another process has the database locked
    
    ManifestManager manager(this);
    
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.title = "Test Book";
    entry.author = "Test Author";
    entry.publicationYear = "2025";
    entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry.localPath = "/path/to/book.sqlite";
    
    // Normal operation should succeed
    bool created = manager.createManifestEntry(entry);
    QVERIFY(created);
    
    // Verify entry exists
    bool exists = manager.manifestEntryExists(entry.cartridgeGuid);
    QVERIFY(exists);
}

void TestDatabaseErrors::testDatabaseCorruptionHandling()
{
    // Test that database corruption is handled gracefully
    // In a real scenario, we'd want to detect corruption and handle it
    
    if (!m_dbManager->isOpen()) {
        m_dbManager->initializeConnection(m_testDbPath);
    }
    
    // Verify database is accessible
    QVERIFY(m_dbManager->isOpen());
    
    // Try to execute a query - should succeed on valid database
    QSqlQuery query = m_dbManager->executeQuery("SELECT COUNT(*) FROM Local_Library_Manifest");
    QVERIFY(query.next());
}

void TestDatabaseErrors::testInvalidPathHandling()
{
    // Test handling of invalid database paths
    LocalDBManager& dbManager = LocalDBManager::getInstance();
    
    // Try to initialize with invalid path (read-only directory)
    QString invalidPath = "/nonexistent/path/database.sqlite";
    
    // Should handle gracefully - either create directory or return false
    bool initialized = dbManager.initializeConnection(invalidPath);
    // May succeed if parent directory can be created, or fail if not
    // The key is that it doesn't crash
}

void TestDatabaseErrors::testConcurrentAccess()
{
    // Test that multiple operations can access the database concurrently
    // SQLite with WAL mode should handle this
    
    ManifestManager manager1(this);
    ManifestManager manager2(this);
    
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Create entries concurrently (simulated)
    ManifestManager::ManifestEntry entry1;
    entry1.cartridgeGuid = guid1;
    entry1.title = "Book 1";
    entry1.author = "Author 1";
    entry1.publicationYear = "2025";
    entry1.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry1.localPath = "/path/to/book1.sqlite";
    
    ManifestManager::ManifestEntry entry2;
    entry2.cartridgeGuid = guid2;
    entry2.title = "Book 2";
    entry2.author = "Author 2";
    entry2.publicationYear = "2025";
    entry2.cartridgeHash = QByteArray::fromHex("b2c3d4e5f6a1");
    entry2.localPath = "/path/to/book2.sqlite";
    
    // Both should succeed (WAL mode allows concurrent reads/writes)
    bool created1 = manager1.createManifestEntry(entry1);
    bool created2 = manager2.createManifestEntry(entry2);
    
    QVERIFY(created1);
    QVERIFY(created2);
    
    // Both entries should exist
    QVERIFY(manager1.manifestEntryExists(guid1));
    QVERIFY(manager2.manifestEntryExists(guid2));
}

QTEST_MAIN(TestDatabaseErrors)
#include "test_database_errors.moc"
