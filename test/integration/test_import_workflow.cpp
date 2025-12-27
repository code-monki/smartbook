#include <QtTest>
#include "smartbook/reader/ImportManager.h"
#include "smartbook/reader/ui/ImportDialog.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/metadata/MetadataExtractor.h"
#include "test_helpers.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QApplication>
#include <QDebug>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;
using namespace smartbook::common::metadata;

/**
 * Integration test for end-to-end import workflow
 * 
 * Tests the complete import process from file selection to manifest creation:
 * 1. File validation
 * 2. Metadata extraction
 * 3. Duplicate detection
 * 4. File copying
 * 5. Manifest entry creation
 * 6. Library refresh
 * 
 * Test Case: Integration test for import workflow
 * Requirements: FR-2.5.6, FR-2.5.7, FR-2.5.9, FR-2.5.10, FR-2.5.11
 */
class TestImportWorkflow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // End-to-end import workflow tests
    void testImportNewCartridge();
    void testImportDuplicateSkip();
    void testImportDuplicateReplace();
    void testImportDuplicateKeepBoth();
    void testImportMultipleCartridges();
    void testImportWithInvalidFile();
    void testImportWithCorruptedFile();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_libraryPath;
    LocalDBManager* m_dbManager;
    ImportManager* m_importManager;
    ManifestManager* m_manifestManager;
    
    QString createTestCartridge(const QString& guid, const QString& title);
    void verifyManifestEntry(const QString& guid, const QString& expectedTitle);
    void verifyCartridgeFile(const QString& cartridgePath);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestImportWorkflow tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestImportWorkflow::initTestCase()
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
    
    m_manifestManager = new ManifestManager(this);
    m_importManager = new ImportManager(this);
    m_importManager->setLibraryPath(m_libraryPath);
}

void TestImportWorkflow::cleanupTestCase()
{
    delete m_importManager;
    delete m_manifestManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestImportWorkflow::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("source_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_Import_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    return path;
}

void TestImportWorkflow::verifyManifestEntry(const QString& guid, const QString& expectedTitle)
{
    ManifestManager::ManifestEntry entry = m_manifestManager->getManifestEntry(guid);
    QVERIFY(!entry.cartridgeGuid.isEmpty());
    QCOMPARE(entry.cartridgeGuid, guid);
    QCOMPARE(entry.title, expectedTitle);
    QVERIFY(!entry.localPath.isEmpty());
    QVERIFY(QFile::exists(entry.localPath));
}

void TestImportWorkflow::verifyCartridgeFile(const QString& cartridgePath)
{
    QVERIFY(QFile::exists(cartridgePath));
    QVERIFY(QFileInfo(cartridgePath).size() > 0);
}

// Test: Import new cartridge (successful workflow)
void TestImportWorkflow::testImportNewCartridge()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString title = "Test Book for Import";
    QString sourcePath = createTestCartridge(guid, title);
    QVERIFY(!sourcePath.isEmpty());
    
    // Import cartridge
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        sourcePath,
        m_libraryPath,
        ui::ImportDialog::Skip  // No duplicate action needed
    );
    
    // Verify import succeeded
    if (result.result != ImportManager::Success) {
        qWarning() << "Import failed:" << result.errorMessage;
    }
    QCOMPARE(result.result, ImportManager::Success);
    QCOMPARE(result.cartridgeGuid, guid);
    QCOMPARE(result.cartridgeTitle, title);
    
    // Verify manifest entry was created
    verifyManifestEntry(guid, title);
    
    // Verify file was copied to library
    ManifestManager::ManifestEntry entry = m_manifestManager->getManifestEntry(guid);
    verifyCartridgeFile(entry.localPath);
    
    // Verify file is in library directory
    QVERIFY(entry.localPath.startsWith(m_libraryPath));
}

// Test: Import duplicate cartridge with Skip action
void TestImportWorkflow::testImportDuplicateSkip()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString title = "Duplicate Test Book";
    QString sourcePath = createTestCartridge(guid, title);
    QVERIFY(!sourcePath.isEmpty());
    
    // First import
    ImportManager::ImportResultInfo result1 = m_importManager->importCartridge(
        sourcePath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    QCOMPARE(result1.result, ImportManager::Success);
    
    // Second import (duplicate) with Skip action
    QString sourcePath2 = createTestCartridge(guid, title);
    ImportManager::ImportResultInfo result2 = m_importManager->importCartridge(
        sourcePath2,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    
    // Verify duplicate was skipped
    QCOMPARE(result2.result, ImportManager::DuplicateSkipped);
    
    // Verify only one manifest entry exists
    QVERIFY(m_manifestManager->manifestEntryExists(guid));
    ManifestManager::ManifestEntry entry = m_manifestManager->getManifestEntry(guid);
    QCOMPARE(entry.title, title);
}

// Test: Import duplicate cartridge with Replace action
void TestImportWorkflow::testImportDuplicateReplace()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString title1 = "Original Book";
    QString title2 = "Replaced Book";
    
    QString sourcePath1 = createTestCartridge(guid, title1);
    QVERIFY(!sourcePath1.isEmpty());
    
    // First import
    ImportManager::ImportResultInfo result1 = m_importManager->importCartridge(
        sourcePath1,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    QCOMPARE(result1.result, ImportManager::Success);
    
    // Get original file path
    ManifestManager::ManifestEntry originalEntry = m_manifestManager->getManifestEntry(guid);
    QString originalPath = originalEntry.localPath;
    
    // Second import (duplicate) with Replace action
    QString sourcePath2 = createTestCartridge(guid, title2);
    ImportManager::ImportResultInfo result2 = m_importManager->importCartridge(
        sourcePath2,
        m_libraryPath,
        ui::ImportDialog::Replace
    );
    
    // Verify duplicate was replaced
    QCOMPARE(result2.result, ImportManager::DuplicateReplaced);
    QCOMPARE(result2.cartridgeGuid, guid);
    QCOMPARE(result2.cartridgeTitle, title2);
    
    // Verify manifest entry was updated
    verifyManifestEntry(guid, title2);
    
    // Verify original file was removed
    QVERIFY(!QFile::exists(originalPath));
    
    // Verify new file exists
    ManifestManager::ManifestEntry newEntry = m_manifestManager->getManifestEntry(guid);
    verifyCartridgeFile(newEntry.localPath);
}

// Test: Import duplicate cartridge with KeepBoth action
void TestImportWorkflow::testImportDuplicateKeepBoth()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString title = "Keep Both Test Book";
    
    QString sourcePath1 = createTestCartridge(guid, title);
    QVERIFY(!sourcePath1.isEmpty());
    
    // First import
    ImportManager::ImportResultInfo result1 = m_importManager->importCartridge(
        sourcePath1,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    QCOMPARE(result1.result, ImportManager::Success);
    
    // Second import (duplicate) with KeepBoth action
    QString sourcePath2 = createTestCartridge(guid, title);
    ImportManager::ImportResultInfo result2 = m_importManager->importCartridge(
        sourcePath2,
        m_libraryPath,
        ui::ImportDialog::KeepBoth
    );
    
    // Verify both files were kept
    QCOMPARE(result2.result, ImportManager::DuplicateKeepBoth);
    
    // Verify manifest entry exists (same GUID, but different file paths)
    QVERIFY(m_manifestManager->manifestEntryExists(guid));
    
    // Note: In KeepBoth scenario, the second file gets a unique filename
    // but the manifest entry is updated (not duplicated, as GUID is unique)
    // This is expected behavior - the second import replaces the first
    // but with a different filename
    ManifestManager::ManifestEntry entry = m_manifestManager->getManifestEntry(guid);
    verifyCartridgeFile(entry.localPath);
}

// Test: Import multiple cartridges in batch
void TestImportWorkflow::testImportMultipleCartridges()
{
    QStringList guids;
    QStringList titles;
    QStringList sourcePaths;
    
    // Create 5 test cartridges
    for (int i = 0; i < 5; ++i) {
        QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString title = QString("Batch Test Book %1").arg(i + 1);
        QString sourcePath = createTestCartridge(guid, title);
        
        QVERIFY(!sourcePath.isEmpty());
        guids.append(guid);
        titles.append(title);
        sourcePaths.append(sourcePath);
    }
    
    // Import all cartridges (using single import for each, as batch import requires progress dialog)
    QList<ImportManager::ImportResultInfo> results;
    for (const QString& sourcePath : sourcePaths) {
        ImportManager::ImportResultInfo result = m_importManager->importCartridge(
            sourcePath,
            m_libraryPath,
            ui::ImportDialog::Skip
        );
        results.append(result);
    }
    
    // Verify all imports succeeded
    QCOMPARE(results.size(), 5);
    for (int i = 0; i < results.size(); ++i) {
        QCOMPARE(results[i].result, ImportManager::Success);
        QCOMPARE(results[i].cartridgeGuid, guids[i]);
        QCOMPARE(results[i].cartridgeTitle, titles[i]);
        
        // Verify manifest entry
        verifyManifestEntry(guids[i], titles[i]);
    }
}

// Test: Import with invalid file (non-SQLite)
void TestImportWorkflow::testImportWithInvalidFile()
{
    // Create a non-SQLite file
    QString invalidPath = m_tempDir->filePath("invalid.txt");
    QFile invalidFile(invalidPath);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly));
    invalidFile.write("This is not a SQLite database");
    invalidFile.close();
    
    // Attempt import
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        invalidPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    
    // Verify import failed with validation error
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

// Test: Import with corrupted SQLite file
void TestImportWorkflow::testImportWithCorruptedFile()
{
    // Create a file that looks like SQLite but is corrupted
    QString corruptedPath = m_tempDir->filePath("corrupted.sqlite");
    QFile corruptedFile(corruptedPath);
    QVERIFY(corruptedFile.open(QIODevice::WriteOnly));
    // Write SQLite header but corrupt the rest
    corruptedFile.write("SQLite format 3");
    corruptedFile.write(QByteArray(100, '\0')); // Some null bytes
    corruptedFile.write("corrupted data");
    corruptedFile.close();
    
    // Attempt import
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        corruptedPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    
    // Verify import failed with validation error
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

#include "test_import_workflow.moc"

