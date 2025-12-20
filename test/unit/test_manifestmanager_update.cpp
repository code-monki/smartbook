#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QSqlQuery>

using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestManifestUpdate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testManifestUpdateOnImport();  // T-PERS-04: Manifest update on import (FR-2.5.2/2.5.3)

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
};

void TestManifestUpdate::initTestCase()
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

void TestManifestUpdate::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

// T-PERS-04: Manifest update on import
// Requirement: FR-2.5.2 (Mandatory metadata storage), FR-2.5.3 (Manifest update)
// Test Plan: Related to T-PERS-01
// AC: When a cartridge is imported/updated, the manifest entry is updated with
//     current metadata (title, author, publication_year, etc.)
void TestManifestUpdate::testManifestUpdateOnImport()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Create initial manifest entry
    ManifestManager::ManifestEntry entry1;
    entry1.cartridgeGuid = guid;
    entry1.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry1.localPath = "/path/to/cartridge.sqlite";
    entry1.title = "Original Title";
    entry1.author = "Original Author";
    entry1.publicationYear = "2024";
    
    bool created = m_manifestManager->createManifestEntry(entry1);
    QVERIFY(created);
    
    // Verify initial entry
    ManifestManager::ManifestEntry retrieved1 = m_manifestManager->getManifestEntry(guid);
    QCOMPARE(retrieved1.title, entry1.title);
    QCOMPARE(retrieved1.author, entry1.author);
    QCOMPARE(retrieved1.publicationYear, entry1.publicationYear);
    
    // Simulate import update with new metadata
    ManifestManager::ManifestEntry entry2;
    entry2.cartridgeGuid = guid; // Same GUID
    entry2.cartridgeHash = QByteArray::fromHex("b2c3d4e5f6a1"); // Updated hash
    entry2.localPath = "/path/to/cartridge_v2.sqlite"; // Updated path
    entry2.title = "Updated Title";
    entry2.author = "Updated Author";
    entry2.publicationYear = "2025"; // Updated year
    entry2.publisher = "New Publisher";
    entry2.version = "2.0";
    
    // Update the manifest entry
    bool updated = m_manifestManager->updateManifestEntry(entry2);
    QVERIFY(updated);
    
    // Verify updated entry
    ManifestManager::ManifestEntry retrieved2 = m_manifestManager->getManifestEntry(guid);
    QVERIFY(retrieved2.isValid());
    QCOMPARE(retrieved2.cartridgeGuid, guid);
    QCOMPARE(retrieved2.title, entry2.title);
    QCOMPARE(retrieved2.author, entry2.author);
    QCOMPARE(retrieved2.publicationYear, entry2.publicationYear);
    QCOMPARE(retrieved2.localPath, entry2.localPath);
    QCOMPARE(retrieved2.cartridgeHash, entry2.cartridgeHash);
    QCOMPARE(retrieved2.publisher, entry2.publisher);
    QCOMPARE(retrieved2.version, entry2.version);
}

QTEST_MAIN(TestManifestUpdate)
#include "test_manifestmanager_update.moc"
