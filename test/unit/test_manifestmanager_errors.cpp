#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>

using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestManifestErrors : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCreateWithEmptyGuid();
    void testCreateWithEmptyTitle();
    void testUpdateNonExistentEntry();
    void testGetNonExistentEntry();
    void testDuplicateGuidPrevention();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
};

void TestManifestErrors::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    m_manifestManager = new ManifestManager(this);
}

void TestManifestErrors::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

void TestManifestErrors::testCreateWithEmptyGuid()
{
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = ""; // Empty GUID
    entry.title = "Test Book";
    entry.author = "Test Author";
    entry.publicationYear = "2025";
    entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry.localPath = "/path/to/book.sqlite";
    
    // Should fail due to empty GUID (validation or database constraint)
    bool result = m_manifestManager->createManifestEntry(entry);
    QVERIFY(!result); // Should fail - either validation or database constraint
}

void TestManifestErrors::testCreateWithEmptyTitle()
{
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.title = ""; // Empty title
    entry.author = "Test Author";
    entry.publicationYear = "2025";
    entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry.localPath = "/path/to/book.sqlite";
    
    // Should fail due to empty title (validation or database constraint)
    bool result = m_manifestManager->createManifestEntry(entry);
    QVERIFY(!result); // Should fail - either validation or database constraint
}

void TestManifestErrors::testUpdateNonExistentEntry()
{
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces); // Non-existent GUID
    entry.title = "Updated Title";
    entry.author = "Updated Author";
    entry.publicationYear = "2025";
    entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry.localPath = "/path/to/book.sqlite";
    
    // Should fail - entry doesn't exist
    bool result = m_manifestManager->updateManifestEntry(entry);
    QVERIFY(!result);
}

void TestManifestErrors::testGetNonExistentEntry()
{
    QString nonExistentGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    ManifestManager::ManifestEntry entry = m_manifestManager->getManifestEntry(nonExistentGuid);
    QVERIFY(!entry.isValid());
    QVERIFY(entry.cartridgeGuid.isEmpty());
}

void TestManifestErrors::testDuplicateGuidPrevention()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    ManifestManager::ManifestEntry entry1;
    entry1.cartridgeGuid = guid;
    entry1.title = "First Book";
    entry1.author = "Author 1";
    entry1.publicationYear = "2025";
    entry1.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry1.localPath = "/path/to/book1.sqlite";
    
    bool created1 = m_manifestManager->createManifestEntry(entry1);
    QVERIFY(created1);
    
    // Try to create another entry with same GUID
    ManifestManager::ManifestEntry entry2;
    entry2.cartridgeGuid = guid; // Same GUID
    entry2.title = "Second Book";
    entry2.author = "Author 2";
    entry2.publicationYear = "2025";
    entry2.cartridgeHash = QByteArray::fromHex("b2c3d4e5f6a1");
    entry2.localPath = "/path/to/book2.sqlite";
    
    // Should fail due to UNIQUE constraint
    bool created2 = m_manifestManager->createManifestEntry(entry2);
    QVERIFY(!created2);
    
    // Verify original entry still exists
    ManifestManager::ManifestEntry retrieved = m_manifestManager->getManifestEntry(guid);
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.title, entry1.title); // Should still be first entry
}

QTEST_MAIN(TestManifestErrors)
#include "test_manifestmanager_errors.moc"
