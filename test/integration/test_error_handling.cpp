#include <QtTest>
#include "smartbook/reader/ImportManager.h"
#include "smartbook/reader/ui/ImportDialog.h"
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/security/SignatureVerifier.h"
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
using namespace smartbook::common::security;

/**
 * Integration test for error handling
 * 
 * Tests error handling across the application:
 * 1. File not found errors
 * 2. Invalid cartridge format errors
 * 3. Database connection errors
 * 4. Signature verification errors
 * 5. Import errors
 * 
 * Test Case: Integration test for error handling
 * Requirements: T-ERR-01 through T-ERR-49 (as applicable for Phase 1)
 */
class TestErrorHandling : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Error handling tests
    void testFileNotFoundError();
    void testInvalidCartridgeFormat();
    void testCorruptedCartridgeFile();
    void testDatabaseConnectionError();
    void testSignatureVerificationError();
    void testImportFileNotFound();
    void testImportInvalidFormat();
    void testCartridgeOpenError();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_libraryPath;
    LocalDBManager* m_dbManager;
    ImportManager* m_importManager;
    
    QString createTestCartridge(const QString& guid, const QString& title);
    QString createTestCartridge(SecurityLevel level, const QString& guid, const QString& /* title */);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestErrorHandling tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestErrorHandling::initTestCase()
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

void TestErrorHandling::cleanupTestCase()
{
    delete m_importManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestErrorHandling::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_Error_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    return path;
}

QString TestErrorHandling::createTestCartridge(SecurityLevel level, const QString& guid, const QString& /* title */)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_Error_Sec_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = false;
    switch (level) {
        case SecurityLevel::LEVEL_1:
            TestHelpers::createL1Cartridge(path, guid, connectionName);
            created = true;
            break;
        case SecurityLevel::LEVEL_2:
            TestHelpers::createL2Cartridge(path, guid, connectionName);
            created = true;
            break;
        case SecurityLevel::LEVEL_3:
            TestHelpers::createL3Cartridge(path, guid, connectionName);
            created = true;
            break;
        default:
            break;
    }
    
    if (!created) {
        return QString();
    }
    
    // Calculate and update H1 hash
    QByteArray h2Hash = smartbook::common::metadata::MetadataExtractor::calculateContentHash(path);
    TestHelpers::updateCartridgeH1Hash(path, h2Hash, connectionName + "_update");
    
    return path;
}

// Test: File not found error (T-ERR-01)
void TestErrorHandling::testFileNotFoundError()
{
    QString nonExistentPath = "/nonexistent/path/cartridge.sqlite";
    
    // Test with CartridgeDBConnector
    CartridgeDBConnector connector(this);
    bool opened = connector.openCartridge(nonExistentPath);
    QVERIFY(!opened);
    QVERIFY(!connector.isOpen());
    
    // Test with ImportManager
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        nonExistentPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

// Test: Invalid cartridge format (T-ERR-02)
void TestErrorHandling::testInvalidCartridgeFormat()
{
    // Create a non-SQLite file
    QString invalidPath = m_tempDir->filePath("invalid.txt");
    QFile invalidFile(invalidPath);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly));
    invalidFile.write("This is not a SQLite database");
    invalidFile.close();
    
    // Test with CartridgeDBConnector
    CartridgeDBConnector connector(this);
    bool opened = connector.openCartridge(invalidPath);
    // SQLite may open the file, but validation should fail
    if (opened) {
        // Validation should fail (no Metadata table)
        QString guid = connector.getCartridgeGuid();
        QVERIFY(guid.isEmpty());
    } else {
        // Or it may not open at all
        QVERIFY(!connector.isOpen());
    }
    
    // Test with ImportManager
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        invalidPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

// Test: Corrupted cartridge file (T-ERR-03)
void TestErrorHandling::testCorruptedCartridgeFile()
{
    // Create a corrupted SQLite file
    QString corruptedPath = m_tempDir->filePath("corrupted.sqlite");
    QFile corruptedFile(corruptedPath);
    QVERIFY(corruptedFile.open(QIODevice::WriteOnly));
    corruptedFile.write("SQLite format 3");
    corruptedFile.write(QByteArray(100, '\0'));
    corruptedFile.write("corrupted data");
    corruptedFile.close();
    
    // Test with CartridgeDBConnector
    CartridgeDBConnector connector(this);
    bool opened = connector.openCartridge(corruptedPath);
    if (opened) {
        // Validation should fail
        QString guid = connector.getCartridgeGuid();
        QVERIFY(guid.isEmpty());
    } else {
        QVERIFY(!connector.isOpen());
    }
    
    // Test with ImportManager
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        corruptedPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

// Test: Database connection error (T-ERR-04)
void TestErrorHandling::testDatabaseConnectionError()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Connection Test");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Open cartridge
    CartridgeDBConnector connector(this);
    QVERIFY(connector.openCartridge(cartridgePath));
    
    // Close database connection (simulating connection error)
    connector.closeCartridge();
    
    // Try to use connector after close
    bool saved = connector.saveFormData("test_form", R"({"data": "test"})");
    QVERIFY(!saved); // Should fail gracefully
    
    QString loaded = connector.loadFormData("test_form");
    QVERIFY(loaded.isEmpty()); // Should return empty gracefully
}

// Test: Signature verification error (T-ERR-05)
void TestErrorHandling::testSignatureVerificationError()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(SecurityLevel::LEVEL_2, guid, "Signature Error Test");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Corrupt the signature data
    QString conn = QString("TestConn_SigError_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", conn);
    db.setDatabaseName(cartridgePath);
    QVERIFY(db.open());
    
    QSqlQuery query(db);
    // Corrupt digital signature
    QByteArray corruptedSig = QByteArray("INVALID_SIGNATURE");
    query.prepare("UPDATE Cartridge_Security SET digital_signature = ?");
    query.addBindValue(corruptedSig);
    query.exec();
    db.close();
    QSqlDatabase::removeDatabase(conn);
    
    // Verify cartridge - should handle signature error gracefully
    SignatureVerifier verifier(this);
    VerificationResult result = verifier.verifyCartridge(cartridgePath, guid);
    
    // Should either reject or require consent (depending on signature verification)
    QVERIFY(result.effectivePolicy == TrustPolicy::REJECTED || 
            result.effectivePolicy == TrustPolicy::CONSENT_REQUIRED);
}

// Test: Import file not found
void TestErrorHandling::testImportFileNotFound()
{
    QString nonExistentPath = "/nonexistent/path/cartridge.sqlite";
    
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        nonExistentPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

// Test: Import invalid format
void TestErrorHandling::testImportInvalidFormat()
{
    // Create invalid file
    QString invalidPath = m_tempDir->filePath("invalid.sqlite");
    QFile invalidFile(invalidPath);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly));
    invalidFile.write("Not a valid SQLite database");
    invalidFile.close();
    
    ImportManager::ImportResultInfo result = m_importManager->importCartridge(
        invalidPath,
        m_libraryPath,
        ui::ImportDialog::Skip
    );
    
    QCOMPARE(result.result, ImportManager::ValidationFailed);
    QVERIFY(!result.errorMessage.isEmpty());
}

// Test: Cartridge open error
void TestErrorHandling::testCartridgeOpenError()
{
    // Test with non-existent file
    QString nonExistentPath = "/nonexistent/path/cartridge.sqlite";
    
    CartridgeDBConnector connector(this);
    bool opened = connector.openCartridge(nonExistentPath);
    QVERIFY(!opened);
    QVERIFY(!connector.isOpen());
    QVERIFY(connector.getCartridgeGuid().isEmpty());
    
    // Test with invalid file
    QString invalidPath = m_tempDir->filePath("invalid.sqlite");
    QFile invalidFile(invalidPath);
    QVERIFY(invalidFile.open(QIODevice::WriteOnly));
    invalidFile.write("Invalid data");
    invalidFile.close();
    
    CartridgeDBConnector connector2(this);
    bool opened2 = connector2.openCartridge(invalidPath);
    // May open but validation should fail
    if (opened2) {
        QString guid = connector2.getCartridgeGuid();
        QVERIFY(guid.isEmpty()); // No valid metadata
    } else {
        QVERIFY(!connector2.isOpen());
    }
}

#include "test_error_handling.moc"

