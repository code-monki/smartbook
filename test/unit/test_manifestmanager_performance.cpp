#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QElapsedTimer>
#include <QDebug>

using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestManifestPerformance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testBulkManifestCreation();  // Performance test for bulk operations
    void testManifestQueryPerformance();  // NFR-3.1: Library Performance

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    void createBulkManifestEntries(int count);
};

void TestManifestPerformance::initTestCase()
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

void TestManifestPerformance::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

void TestManifestPerformance::createBulkManifestEntries(int count)
{
    for (int i = 0; i < count; ++i) {
        ManifestManager::ManifestEntry entry;
        entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
        entry.localPath = QString("/path/to/cartridge_%1.sqlite").arg(i);
        entry.title = QString("Test Book %1").arg(i);
        entry.author = "Test Author";
        entry.publicationYear = "2025";
        
        m_manifestManager->createManifestEntry(entry);
    }
}

void TestManifestPerformance::testBulkManifestCreation()
{
    const int entryCount = 100;
    
    QElapsedTimer timer;
    timer.start();
    
    createBulkManifestEntries(entryCount);
    
    qint64 elapsed = timer.elapsed();
    
    // Verify all entries were created
    QSqlQuery query(m_dbManager->getDatabase());
    query.exec("SELECT COUNT(*) FROM Local_Library_Manifest");
    if (query.next()) {
        int count = query.value(0).toInt();
        QVERIFY(count >= entryCount);
    }
    
    // Performance check: 100 entries should be created quickly
    // This is a basic performance test - in production, we'd want < 500ms for 100 entries
    qDebug() << "Created" << entryCount << "manifest entries in" << elapsed << "ms";
    QVERIFY(elapsed < 5000); // Should complete in under 5 seconds (generous for test)
}

// NFR-3.1: Library Performance
// Requirement: Library View loads within 500ms for 100 cartridges
// Test Plan: T-UI-01 (test-plan.adoc lines 127-134)
// AC: The Library View loads and displays all required metadata fields within 500 milliseconds
void TestManifestPerformance::testManifestQueryPerformance()
{
    // Create 100 manifest entries
    const int entryCount = 100;
    createBulkManifestEntries(entryCount);
    
    QElapsedTimer timer;
    timer.start();
    
    // Simulate library load: Query all manifest entries with required fields
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare(R"(
        SELECT cartridge_guid, title, author, publication_year, local_path
        FROM Local_Library_Manifest
        ORDER BY title
    )");
    
    QVERIFY(query.exec());
    
    int count = 0;
    while (query.next()) {
        // Simulate processing each entry (as Library View would do)
        QString guid = query.value(0).toString();
        QString title = query.value(1).toString();
        QString author = query.value(2).toString();
        QString year = query.value(3).toString();
        QString path = query.value(4).toString();
        
        // Verify we have required fields
        QVERIFY(!guid.isEmpty());
        QVERIFY(!title.isEmpty());
        QVERIFY(!author.isEmpty());
        QVERIFY(!year.isEmpty());
        
        count++;
    }
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(count >= entryCount);
    
    // Performance requirement: < 500ms for 100 cartridges
    qDebug() << "Loaded" << count << "manifest entries in" << elapsed << "ms";
    QVERIFY(elapsed < 500); // NFR-3.1 requirement: < 500ms
}

QTEST_MAIN(TestManifestPerformance)
#include "test_manifestmanager_performance.moc"
