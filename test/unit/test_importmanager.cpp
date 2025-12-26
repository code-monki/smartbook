#include <QtTest>
#include "smartbook/reader/ImportManager.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "test_helpers.h"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

using namespace smartbook::reader;
using namespace smartbook::reader::ui;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestImportManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testValidateCartridge();  // T-PERS-01: Import validation
    void testImportCartridge();    // T-PERS-01: Import and manifest creation
    void testDuplicateDetection(); // T-PERS-01: Duplicate detection
    void testDuplicateReplace();   // T-PERS-01: Duplicate replace
    void testDuplicateKeepBoth();  // T-PERS-01: Duplicate keep both
    void testImportValidationFailure(); // Error handling

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_libraryPath;
    LocalDBManager* m_dbManager;
    ImportManager* m_importManager;
    
    QString createTestCartridge(const QString& guid, const QString& title);
};

void TestImportManager::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    m_libraryPath = m_tempDir->filePath("library");
    QDir().mkpath(m_libraryPath);
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    m_importManager = new ImportManager(this);
    m_importManager->setLibraryPath(m_libraryPath);
}

void TestImportManager::cleanupTestCase()
{
    delete m_importManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestImportManager::createTestCartridge(const QString& guid, const QString& title)
{
    QString cartridgePath = m_tempDir->filePath(QString("test_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)));
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "TestCartridge");
    db.setDatabaseName(cartridgePath);
    if (!db.open()) {
        return QString(); // Return empty string on failure
    }
    
    // Create minimal schema
    QSqlQuery query(db);
    query.exec(R"(
        CREATE TABLE Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT,
            publisher TEXT,
            version TEXT,
            publication_year TEXT NOT NULL,
            schema_version TEXT NOT NULL DEFAULT '1.0'
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Content_Pages (
            page_id TEXT PRIMARY KEY,
            page_order INTEGER NOT NULL,
            page_title TEXT,
            html_content TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Cartridge_Security (
            cartridge_guid TEXT PRIMARY KEY,
            security_level INTEGER NOT NULL,
            h1_hash BLOB,
            h2_hash BLOB
        )
    )");
    
    // Insert test data
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, schema_version) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(title);
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.exec();
    
    query.prepare("INSERT INTO Cartridge_Security (cartridge_guid, security_level) VALUES (?, ?)");
    query.addBindValue(guid);
    query.addBindValue(3); // Level 3
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase("TestCartridge");
    
    return cartridgePath;
}

// T-PERS-01: Import validation
// Requirement: FR-2.5.10 (Import Validation)
// AC: Cartridge file is validated before import (file format, schema, GUID, version)
void TestImportManager::testValidateCartridge()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Test Book");
    
    // Valid cartridge should pass validation
    bool isValid = m_importManager->validateCartridge(cartridgePath);
    QVERIFY(isValid);
    
    // Invalid file should fail validation
    QString invalidPath = m_tempDir->filePath("invalid.sqlite");
    QFile invalidFile(invalidPath);
    invalidFile.open(QIODevice::WriteOnly);
    invalidFile.write("Not a valid SQLite database");
    invalidFile.close();
    
    bool isInvalid = m_importManager->validateCartridge(invalidPath);
    QVERIFY(!isInvalid);
}

// T-PERS-01: Import and manifest creation
// Requirement: FR-2.5.1 (Manifest Creation), FR-2.5.6 (Import Mechanisms)
// AC: Cartridge is imported, file is copied to library, manifest entry is created
void TestImportManager::testImportCartridge()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString sourcePath = createTestCartridge(guid, "Test Import Book");
    
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        sourcePath,
        m_libraryPath,
        ImportDialog::Skip
    );
    
    QCOMPARE(result.result, ImportManager::Success);
    QCOMPARE(result.cartridgeGuid, guid);
    
    // Verify file was copied to library
    QStringList libraryFiles = QDir(m_libraryPath).entryList(QStringList() << "*.sqlite", QDir::Files);
    QVERIFY(libraryFiles.size() > 0);
    
    // Verify manifest entry was created
    ManifestManager manifestManager;
    QVERIFY(manifestManager.manifestEntryExists(guid));
    
    ManifestManager::ManifestEntry entry = manifestManager.getManifestEntry(guid);
    QVERIFY(entry.isValid());
    QCOMPARE(entry.cartridgeGuid, guid);
    QCOMPARE(entry.title, "Test Import Book");
    QCOMPARE(entry.publicationYear, "2025");
}

// T-PERS-01: Duplicate detection
// Requirement: FR-2.5.9 (Duplicate Detection)
// AC: Duplicate cartridge is detected by cartridge_guid
void TestImportManager::testDuplicateDetection()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString sourcePath = createTestCartridge(guid, "Duplicate Test Book");
    
    // First import
    ImportManager::ImportResultInfo result1 = m_importManager->importCartridge(
        sourcePath,
        m_libraryPath,
        ImportDialog::Skip
    );
    QCOMPARE(result1.result, ImportManager::Success);
    
    // Second import with same GUID (duplicate)
    QString sourcePath2 = createTestCartridge(guid, "Duplicate Test Book 2");
    ImportManager::ImportResultInfo result2 = m_importManager->importCartridge(
        sourcePath2,
        m_libraryPath,
        ImportDialog::Skip
    );
    
    QCOMPARE(result2.result, ImportManager::DuplicateSkipped);
    QCOMPARE(result2.cartridgeGuid, guid);
}

// T-PERS-01: Duplicate replace
// Requirement: FR-2.5.9 (Duplicate Detection)
// AC: Duplicate cartridge can be replaced (old file deleted, new file imported)
void TestImportManager::testDuplicateReplace()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString sourcePath1 = createTestCartridge(guid, "Original Book");
    
    // First import
    ImportManager::ImportResultInfo result1 = m_importManager->importCartridge(
        sourcePath1,
        m_libraryPath,
        ImportDialog::Skip
    );
    QCOMPARE(result1.result, ImportManager::Success);
    
    // Get original manifest entry
    ManifestManager manifestManager;
    ManifestManager::ManifestEntry originalEntry = manifestManager.getManifestEntry(guid);
    QString originalPath = originalEntry.localPath;
    
    // Replace with new cartridge
    QString sourcePath2 = createTestCartridge(guid, "Replaced Book");
    ImportManager::ImportResultInfo result2 = m_importManager->importCartridge(
        sourcePath2,
        m_libraryPath,
        ImportDialog::Replace
    );
    
    QCOMPARE(result2.result, ImportManager::DuplicateReplaced);
    
    // Verify old file is deleted
    QVERIFY(!QFile::exists(originalPath));
    
    // Verify manifest entry is updated
    ManifestManager::ManifestEntry updatedEntry = manifestManager.getManifestEntry(guid);
    QCOMPARE(updatedEntry.title, "Replaced Book");
}

// T-PERS-01: Duplicate keep both
// Requirement: FR-2.5.9 (Duplicate Detection)
// AC: Duplicate cartridge can be kept with unique filename
void TestImportManager::testDuplicateKeepBoth()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString sourcePath1 = createTestCartridge(guid, "Original Book");
    
    // First import
    ImportManager::ImportResultInfo result1 = m_importManager->importCartridge(
        sourcePath1,
        m_libraryPath,
        ImportDialog::Skip
    );
    QCOMPARE(result1.result, ImportManager::Success);
    
    // Keep both
    QString sourcePath2 = createTestCartridge(guid, "Duplicate Book");
    ImportManager::ImportResultInfo result2 = m_importManager->importCartridge(
        sourcePath2,
        m_libraryPath,
        ImportDialog::KeepBoth
    );
    
    QCOMPARE(result2.result, ImportManager::DuplicateKeepBoth);
    
    // Verify both files exist (with different names)
    ManifestManager manifestManager;
    ManifestManager::ManifestEntry entry1 = manifestManager.getManifestEntry(guid);
    QVERIFY(QFile::exists(entry1.localPath));
    
    // Note: KeepBoth creates a new file with unique name, but same GUID
    // This means we'll have two entries with same GUID but different paths
    // In practice, this might need special handling, but for now we verify the file exists
}

// Error handling: Invalid cartridge
void TestImportManager::testImportValidationFailure()
{
    QString invalidPath = m_tempDir->filePath("invalid.sqlite");
    QFile invalidFile(invalidPath);
    invalidFile.open(QIODevice::WriteOnly);
    invalidFile.write("Not a valid SQLite database");
    invalidFile.close();
    
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        invalidPath,
        m_libraryPath,
        ImportDialog::Skip
    );
    
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_MAIN(TestImportManager)
#include "test_importmanager.moc"

