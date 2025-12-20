#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QDateTime>

using namespace smartbook::common::database;

// Forward declaration - ManifestManager will be implemented
class ManifestManager {
public:
    struct ManifestEntry {
        QString cartridgeGuid;
        QByteArray cartridgeHash;
        QString localPath;
        QString title;
        QString author;
        QString publisher;
        QString version;
        QString publicationYear;
        QByteArray coverImageData;
        bool isValid() const { return !cartridgeGuid.isEmpty() && !title.isEmpty(); }
    };

    bool createManifestEntry(const ManifestEntry& entry);
    ManifestEntry getManifestEntry(const QString& cartridgeGuid);
};

class TestManifestManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testManifestEntryCreation();  // T-PERS-01: Manifest Creation (FR-2.5.1)

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
};

void TestManifestManager::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
}

void TestManifestManager::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

// T-PERS-01: Manifest Creation
// Requirement: FR-2.5.1
// Test Plan: test-plan.adoc lines 90-97
// AC: A new entry exists containing the correct cartridge_guid, cartridge_hash (H2),
//     local_path, and the required publication_year
void TestManifestManager::testManifestEntryCreation()
{
    ManifestManager manager;
    
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");  // Simulated H2 hash
    entry.localPath = "/path/to/test_cartridge.sqlite";
    entry.title = "Test Book";
    entry.author = "Test Author";
    entry.publisher = "Test Publisher";
    entry.version = "1.0";
    entry.publicationYear = "2025";
    
    // Create manifest entry
    bool result = manager.createManifestEntry(entry);
    QVERIFY(result);
    
    // Verify entry exists and has correct data
    ManifestManager::ManifestEntry retrieved = manager.getManifestEntry(entry.cartridgeGuid);
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.cartridgeGuid, entry.cartridgeGuid);
    QCOMPARE(retrieved.title, entry.title);
    QCOMPARE(retrieved.author, entry.author);
    QCOMPARE(retrieved.publicationYear, entry.publicationYear);
    QCOMPARE(retrieved.localPath, entry.localPath);
    QCOMPARE(retrieved.cartridgeHash, entry.cartridgeHash);
}

QTEST_MAIN(TestManifestManager)
#include "test_manifestmanager.moc"
