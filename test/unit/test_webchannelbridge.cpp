#include <QtTest>
#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/security/TrustRegistry.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QApplication>
#include <QDebug>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::security;

class TestWebChannelBridge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Form data persistence tests
    void testSaveFormData();          // T-WEB-01: Form data save
    void testLoadFormData();          // T-WEB-02: Form data load
    void testLoadFormDataNotFound();  // T-WEB-03: Form data not found
    void testSaveFormDataNoCartridge(); // T-WEB-04: Error handling - no cartridge
    
    // Sandbox file operations tests
    void testSaveSandboxFile();       // T-WEB-05: Sandbox file save
    void testLoadSandboxFile();       // T-WEB-06: Sandbox file load
    void testListSandboxFiles();      // T-WEB-07: Sandbox file list
    void testDeleteSandboxFile();     // T-WEB-08: Sandbox file delete
    void testSandboxFilenameValidation(); // T-WEB-09: Filename validation (directory traversal)
    void testSandboxNoAppId();        // T-WEB-10: Error handling - no app ID
    
    // App consent tests
    void testRequestAppConsentPersistentTrust(); // T-WEB-11: Consent with persistent trust
    void testRequestAppConsentRevoked();         // T-WEB-12: Consent with revoked trust
    void testRequestAppConsentNoCartridge();     // T-WEB-13: Error handling - no cartridge
    
    // Log message test
    void testLogMessage();            // T-WEB-14: Log message

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    LocalDBManager* m_dbManager;
    TrustRegistry* m_trustRegistry;
    
    QString createTestCartridge(const QString& guid);
    void cleanupSandbox();
};

void TestWebChannelBridge::initTestCase()
{
    // QTEST_MAIN creates QApplication automatically, no need to create manually
    
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    // Initialize local database
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    // Initialize trust registry
    m_trustRegistry = new TrustRegistry(this);
    
    // Create test cartridge (use UUID with dashes for validation)
    m_cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge(m_cartridgeGuid);
    QVERIFY(QFile::exists(m_cartridgePath));
    
    // Verify cartridge can be opened (validation passes)
    CartridgeDBConnector testConnector(this);
    bool canOpen = testConnector.openCartridge(m_cartridgePath);
    if (!canOpen) {
        qWarning() << "Test cartridge validation failed:" << testConnector.getValidationError();
    }
    testConnector.closeCartridge();
    
    // Clean up any existing sandbox directories
    cleanupSandbox();
}

void TestWebChannelBridge::cleanupTestCase()
{
    cleanupSandbox();
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

void TestWebChannelBridge::cleanupSandbox()
{
    // Clean up sandbox directories created during tests
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString sandboxBase = QDir(appDataDir).filePath("sandbox");
    if (QDir(sandboxBase).exists()) {
        QDir(sandboxBase).removeRecursively();
    }
}

QString TestWebChannelBridge::createTestCartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_WebChannel_" + guid;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
        QSqlDatabase::removeDatabase(connectionName);
        return QString();
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table (with all required columns for validation)
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL,
            version TEXT NOT NULL DEFAULT '1.0',
            schema_version TEXT NOT NULL DEFAULT '1.0'
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
            html_content TEXT NOT NULL
        )
    )");
    
    query.exec("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (1, 1, '<p>Test content</p>')");
    
    // Create User_Data table (for form data)
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS User_Data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            form_id TEXT NOT NULL,
            data_json TEXT NOT NULL,
            saved_timestamp INTEGER NOT NULL,
            UNIQUE(form_id)
        )
    )");
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

// T-WEB-01: Form data save
// Requirement: FR-2.7.4 (Form Data Persistence to Cartridge)
// AC: Form data is saved to User_Data table in cartridge
void TestWebChannelBridge::testSaveFormData()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    
    QSignalSpy spy(&bridge, &WebChannelBridge::formDataSaved);
    
    QString formId = "test_form_1";
    QString dataJson = R"({"name": "Test User", "age": 30})";
    
    bridge.saveFormData(formId, dataJson, QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), formId);
    QVERIFY(arguments.at(1).toBool()); // success
    QVERIFY(arguments.at(2).toString().isEmpty()); // no error
    
    // Verify data was saved to cartridge
    CartridgeDBConnector connector(this);
    QVERIFY(connector.openCartridge(m_cartridgePath));
    
    QString loadedData = connector.loadFormData(formId);
    QCOMPARE(loadedData, dataJson);
    
    connector.closeCartridge();
}

// T-WEB-02: Form data load
// Requirement: FR-2.7.6 (Form Data Loading)
// AC: Form data is loaded from User_Data table
void TestWebChannelBridge::testLoadFormData()
{
    // First save some data
    CartridgeDBConnector connector(this);
    QVERIFY(connector.openCartridge(m_cartridgePath));
    
    QString formId = "test_form_2";
    QString dataJson = R"({"name": "Test User 2", "score": 100})";
    QVERIFY(connector.saveFormData(formId, dataJson));
    
    connector.closeCartridge();
    
    // Now test loading
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    
    QSignalSpy spy(&bridge, &WebChannelBridge::formDataLoaded);
    
    bridge.loadFormData(formId, QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), formId);
    QCOMPARE(arguments.at(1).toString(), dataJson);
    QVERIFY(arguments.at(2).toString().isEmpty()); // no error
}

// T-WEB-03: Form data not found
// AC: Returns empty data when form ID doesn't exist
void TestWebChannelBridge::testLoadFormDataNotFound()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    
    QSignalSpy spy(&bridge, &WebChannelBridge::formDataLoaded);
    
    bridge.loadFormData("nonexistent_form", QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toString(), QString()); // empty data
    QVERIFY(arguments.at(2).toString().isEmpty()); // no error (not found is not an error)
}

// T-WEB-04: Error handling - no cartridge
// AC: Emits error signal when cartridge path is not set
void TestWebChannelBridge::testSaveFormDataNoCartridge()
{
    WebChannelBridge bridge(this);
    // Don't set cartridge info
    
    QSignalSpy spy(&bridge, &WebChannelBridge::formDataSaved);
    
    bridge.saveFormData("test_form", R"({"data": "test"})", QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(!arguments.at(1).toBool()); // failure
    QVERIFY(!arguments.at(2).toString().isEmpty()); // error message
}

// T-WEB-05: Sandbox file save
// Requirement: FR-2.7.5 (Form Data Persistence to Sandbox)
// AC: File is saved to sandbox directory
void TestWebChannelBridge::testSaveSandboxFile()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    bridge.setAppId("test_app_1");
    
    QSignalSpy spy(&bridge, &WebChannelBridge::sandboxFileSaved);
    
    QString filename = "test_file.txt";
    QByteArray data = "Test file content";
    
    bridge.saveSandboxFile(filename, data, QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), filename);
    QVERIFY(arguments.at(1).toBool()); // success
    QVERIFY(arguments.at(2).toString().isEmpty()); // no error
    
    // Verify file exists
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString sandboxPath = QDir(appDataDir).filePath(QString("sandbox/%1/test_app_1/sandbox").arg(m_cartridgeGuid));
    QString filePath = QDir(sandboxPath).filePath(filename);
    
    QVERIFY(QFile::exists(filePath));
    
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), data);
    file.close();
}

// T-WEB-06: Sandbox file load
// AC: File is loaded from sandbox directory
void TestWebChannelBridge::testLoadSandboxFile()
{
    // First save a file
    WebChannelBridge bridge1(this);
    bridge1.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    bridge1.setAppId("test_app_2");
    
    QString filename = "test_load.txt";
    QByteArray savedData = "Loaded file content";
    
    QSignalSpy spy1(&bridge1, &WebChannelBridge::sandboxFileSaved);
    bridge1.saveSandboxFile(filename, savedData, QString());
    QCOMPARE(spy1.count(), 1);
    
    // Now test loading
    WebChannelBridge bridge2(this);
    bridge2.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    bridge2.setAppId("test_app_2");
    
    QSignalSpy spy2(&bridge2, &WebChannelBridge::sandboxFileLoaded);
    
    bridge2.loadSandboxFile(filename, QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy2.count(), 1);
    
    QList<QVariant> arguments = spy2.takeFirst();
    QCOMPARE(arguments.at(0).toString(), filename);
    QCOMPARE(arguments.at(1).toByteArray(), savedData);
    QVERIFY(arguments.at(2).toString().isEmpty()); // no error
}

// T-WEB-07: Sandbox file list
// AC: Lists all files in sandbox directory
void TestWebChannelBridge::testListSandboxFiles()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    bridge.setAppId("test_app_3");
    
    // Save multiple files
    QSignalSpy spySave(&bridge, &WebChannelBridge::sandboxFileSaved);
    bridge.saveSandboxFile("file1.txt", "Content 1", QString());
    QCOMPARE(spySave.count(), 1);
    bridge.saveSandboxFile("file2.txt", "Content 2", QString());
    QCOMPARE(spySave.count(), 2);
    bridge.saveSandboxFile("file3.dat", "Content 3", QString());
    QCOMPARE(spySave.count(), 3);
    
    // List files
    QSignalSpy spyList(&bridge, &WebChannelBridge::sandboxFilesListed);
    
    bridge.listSandboxFiles(QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spyList.count(), 1);
    
    QList<QVariant> arguments = spyList.takeFirst();
    QStringList files = arguments.at(0).toStringList();
    QVERIFY(files.contains("file1.txt"));
    QVERIFY(files.contains("file2.txt"));
    QVERIFY(files.contains("file3.dat"));
    QVERIFY(arguments.at(1).toString().isEmpty()); // no error
}

// T-WEB-08: Sandbox file delete
// AC: File is deleted from sandbox directory
void TestWebChannelBridge::testDeleteSandboxFile()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    bridge.setAppId("test_app_4");
    
    // Save a file first
    QSignalSpy spySave(&bridge, &WebChannelBridge::sandboxFileSaved);
    bridge.saveSandboxFile("delete_me.txt", "To be deleted", QString());
    QCOMPARE(spySave.count(), 1);
    
    // Verify file exists
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString sandboxPath = QDir(appDataDir).filePath(QString("sandbox/%1/test_app_4/sandbox").arg(m_cartridgeGuid));
    QString filePath = QDir(sandboxPath).filePath("delete_me.txt");
    QVERIFY(QFile::exists(filePath));
    
    // Delete file
    QSignalSpy spyDelete(&bridge, &WebChannelBridge::sandboxFileDeleted);
    
    bridge.deleteSandboxFile("delete_me.txt", QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spyDelete.count(), 1);
    
    QList<QVariant> arguments = spyDelete.takeFirst();
    QCOMPARE(arguments.at(0).toString(), "delete_me.txt");
    QVERIFY(arguments.at(1).toBool()); // success
    QVERIFY(arguments.at(2).toString().isEmpty()); // no error
    
    // Verify file is deleted
    QVERIFY(!QFile::exists(filePath));
}

// T-WEB-09: Filename validation (directory traversal)
// AC: Invalid filenames (with .. or /) are rejected
void TestWebChannelBridge::testSandboxFilenameValidation()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    bridge.setAppId("test_app_5");
    
    // Test directory traversal attempt
    QSignalSpy spy(&bridge, &WebChannelBridge::sandboxFileSaved);
    
    bridge.saveSandboxFile("../../../etc/passwd", "malicious", QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(!arguments.at(1).toBool()); // failure
    QVERIFY(arguments.at(2).toString().contains("Invalid")); // error message
}

// T-WEB-10: Error handling - no app ID
// AC: Emits error signal when app ID is not set
void TestWebChannelBridge::testSandboxNoAppId()
{
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    // Don't set app ID
    
    QSignalSpy spy(&bridge, &WebChannelBridge::sandboxFileSaved);
    
    bridge.saveSandboxFile("test.txt", "data", QString());
    
    // Signal is emitted synchronously
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(!arguments.at(1).toBool()); // failure
    QVERIFY(!arguments.at(2).toString().isEmpty()); // error message
}

// T-WEB-11: Consent with persistent trust
// AC: Consent is granted immediately if persistent trust exists
void TestWebChannelBridge::testRequestAppConsentPersistentTrust()
{
    // Set up persistent trust
    m_trustRegistry->storeTrustDecision(m_cartridgeGuid, TrustRegistry::TrustPolicy::PERSISTENT);
    
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    
    QSignalSpy spy(&bridge, &WebChannelBridge::consentGranted);
    
    bridge.requestAppConsent("test_app", QString());
    
    // Signal is emitted synchronously (for persistent trust case)
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), "test_app");
    QVERIFY(arguments.at(1).toBool()); // granted
}

// T-WEB-12: Consent with revoked trust
// AC: Consent is denied immediately if trust is revoked
void TestWebChannelBridge::testRequestAppConsentRevoked()
{
    // Set up revoked trust
    m_trustRegistry->storeTrustDecision(m_cartridgeGuid, TrustRegistry::TrustPolicy::REVOKED);
    
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(m_cartridgePath, m_cartridgeGuid);
    
    QSignalSpy spy(&bridge, &WebChannelBridge::consentGranted);
    
    bridge.requestAppConsent("test_app", QString());
    
    // Signal is emitted synchronously (for persistent trust case)
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), "test_app");
    QVERIFY(!arguments.at(1).toBool()); // denied
}

// T-WEB-13: Error handling - no cartridge
// AC: Emits denied signal when cartridge GUID is not set
void TestWebChannelBridge::testRequestAppConsentNoCartridge()
{
    WebChannelBridge bridge(this);
    // Don't set cartridge info
    
    QSignalSpy spy(&bridge, &WebChannelBridge::consentGranted);
    
    bridge.requestAppConsent("test_app", QString());
    
    // Signal is emitted synchronously (for persistent trust case)
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(!arguments.at(1).toBool()); // denied
}

// T-WEB-14: Log message
// AC: Messages are logged with appropriate level
void TestWebChannelBridge::testLogMessage()
{
    WebChannelBridge bridge(this);
    
    // Test different log levels
    // Note: We can't easily test qDebug/qWarning/qCritical output,
    // but we can verify the method doesn't crash
    bridge.logMessage("debug", "Debug message");
    bridge.logMessage("info", "Info message");
    bridge.logMessage("warn", "Warning message");
    bridge.logMessage("error", "Error message");
    
    // If we get here without crashing, the test passes
    QVERIFY(true);
}

QTEST_MAIN(TestWebChannelBridge)
#include "test_webchannelbridge.moc"

