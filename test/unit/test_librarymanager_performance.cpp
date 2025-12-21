#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QElapsedTimer>
#include <QSqlQuery>
#include <QDebug>

using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestLibraryManagerPerformance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testLibraryLoadPerformance();  // T-UI-01: Library Performance (NFR-3.1)

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    void createBulkManifestEntries(int count);
};

void TestLibraryManagerPerformance::initTestCase()
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

void TestLibraryManagerPerformance::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

void TestLibraryManagerPerformance::createBulkManifestEntries(int count)
{
    for (int i = 0; i < count; ++i) {
        ManifestManager::ManifestEntry entry;
        entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
        entry.localPath = QString("/path/to/cartridge_%1.sqlite").arg(i);
        entry.title = QString("Test Book %1").arg(i);
        entry.author = QString("Author %1").arg(i % 10); // Vary authors
        entry.publicationYear = QString("%1").arg(2020 + (i % 5)); // Vary years
        entry.publisher = QString("Publisher %1").arg(i % 5);
        entry.version = "1.0";
        
        m_manifestManager->createManifestEntry(entry);
    }
}

// T-UI-01: Library Performance
// Requirement: NFR-3.1
// Test Plan: test-plan.adoc lines 127-134
// AC: The Library View loads and displays all required metadata fields
//     (Title, Author, Year, etc.) within 500 milliseconds
void TestLibraryManagerPerformance::testLibraryLoadPerformance()
{
    // Create 100 manifest entries (simulating 100 cartridges in library)
    const int cartridgeCount = 100;
    createBulkManifestEntries(cartridgeCount);
    
    QElapsedTimer timer;
    timer.start();
    
    // Test LibraryManager::loadLibraryData() query - query all manifest entries
    // with required fields for display (Title, Author, Year, etc.)
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare(R"(
        SELECT cartridge_guid, title, author, publication_year, 
               publisher, version, local_path, cover_image_data
        FROM Local_Library_Manifest
        ORDER BY title
    )");
    
    QVERIFY(query.exec());
    
    int count = 0;
    QList<QString> titles;
    QList<QString> authors;
    QList<QString> years;
    
    while (query.next()) {
        // Simulate processing each entry for display (as LibraryManager does)
        QString guid = query.value(0).toString();
        QString title = query.value(1).toString();
        QString author = query.value(2).toString();
        QString year = query.value(3).toString();
        QString publisher = query.value(4).toString();
        QString version = query.value(5).toString();
        QString path = query.value(6).toString();
        QByteArray coverImage = query.value(7).toByteArray();
        
        // Verify required fields are present
        QVERIFY(!guid.isEmpty());
        QVERIFY(!title.isEmpty());
        QVERIFY(!author.isEmpty());
        QVERIFY(!year.isEmpty());
        
        // Store for verification
        titles.append(title);
        authors.append(author);
        years.append(year);
        
        count++;
    }
    
    qint64 elapsed = timer.elapsed();
    
    // Verify we loaded all entries
    QVERIFY(count >= cartridgeCount);
    QCOMPARE(titles.size(), count);
    QCOMPARE(authors.size(), count);
    QCOMPARE(years.size(), count);
    
    // Performance requirement: < 500ms for 100+ cartridges
    qDebug() << "Loaded" << count << "cartridges in" << elapsed << "ms";
    QVERIFY(elapsed < 500); // NFR-3.1 requirement
}

QTEST_MAIN(TestLibraryManagerPerformance)
#include "test_librarymanager_performance.moc"
