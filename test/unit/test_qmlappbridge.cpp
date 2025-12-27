/**
 * @file test_qmlappbridge.cpp
 * @brief Unit tests for QmlAppBridge
 * 
 * Tests for C++/QML communication bridge for embedded applications.
 * 
 * Test Cases:
 * T-BRIDGE-01: Expose bridge to QML (property access)
 * T-BRIDGE-02: Save form data from QML
 * T-BRIDGE-03: Load form data from QML
 * T-BRIDGE-04: Request consent from QML
 * T-BRIDGE-05: Sandbox file operations from QML
 * T-BRIDGE-06: Log message from QML
 * T-BRIDGE-07: Signal emission to QML
 */

#include <QtTest>
#include "smartbook/reader/QmlAppBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/security/TrustRegistry.h"
#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QDebug>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::security;

// Test helper functions
namespace {
    QString createTestCartridge(const QString& guid)
    {
        QTemporaryFile tempFile;
        if (!tempFile.open()) {
            return QString();
        }
        
        QString path = tempFile.fileName();
        tempFile.close();
        tempFile.remove();
        
        // Create SQLite database
        QString connectionName = "test_bridge_" + guid;
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(path);
        
        if (!db.open()) {
            return QString();
        }
        
        QSqlQuery query(db);
        
        // Create Metadata table
        query.exec(R"(
            CREATE TABLE Metadata (
                cartridge_guid TEXT PRIMARY KEY,
                title TEXT NOT NULL,
                author TEXT,
                publisher TEXT,
                version TEXT,
                schema_version TEXT,
                publication_year INTEGER
            )
        )");
        
        query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publisher, version, schema_version, publication_year) VALUES (?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(guid);
        query.addBindValue("Test Cartridge");
        query.addBindValue("Test Author");
        query.addBindValue("Test Publisher");
        query.addBindValue("1.0");
        query.addBindValue("1.0");
        query.addBindValue(2024);
        query.exec();
        
        // Create User_Data table
        query.exec(R"(
            CREATE TABLE User_Data (
                form_id TEXT PRIMARY KEY,
                data_json TEXT NOT NULL,
                last_updated TEXT NOT NULL
            )
        )");
        
        // Create Content_Pages table (required for validation)
        query.exec(R"(
            CREATE TABLE Content_Pages (
                page_id INTEGER PRIMARY KEY AUTOINCREMENT,
                page_order INTEGER NOT NULL,
                html_content TEXT,
                associated_css TEXT
            )
        )");
        
        query.exec("INSERT INTO Content_Pages (page_order, html_content) VALUES (1, '<p>Test</p>')");
        
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        
        return path;
    }
    
    void cleanupSandbox(const QString& cartridgeGuid, const QString& appId)
    {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataPath.isEmpty()) {
            return;
        }
        
        QDir appDataDir(appDataPath);
        QString sandboxBase = appDataDir.filePath("sandbox");
        QString cartridgeSandbox = QDir(sandboxBase).filePath(cartridgeGuid);
        QString appSandbox = QDir(cartridgeSandbox).filePath(appId);
        
        if (QDir(appSandbox).exists()) {
            QDir(appSandbox).removeRecursively();
        }
    }
}

class TestQmlAppBridge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // T-BRIDGE-01: Expose bridge to QML (property access)
    void testPropertyAccess();
    void testPropertySignals();
    
    // T-BRIDGE-02: Save form data from QML
    void testSaveFormData();
    void testSaveFormDataNoCartridge();
    void testSaveFormDataInvalidCartridge();
    
    // T-BRIDGE-03: Load form data from QML
    void testLoadFormData();
    void testLoadFormDataNotFound();
    void testLoadFormDataNoCartridge();
    
    // T-BRIDGE-04: Request consent from QML
    void testRequestAppConsentPersistentTrust();
    void testRequestAppConsentRevoked();
    void testRequestAppConsentNoCartridge();
    
    // T-BRIDGE-05: Sandbox file operations from QML
    void testSaveSandboxFile();
    void testLoadSandboxFile();
    void testListSandboxFiles();
    void testDeleteSandboxFile();
    void testSandboxFilenameValidation();
    void testSandboxNoAppId();
    
    // T-BRIDGE-06: Log message from QML
    void testLogMessage();
    
    // T-BRIDGE-07: Signal emission to QML
    void testSignalEmission();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    QString m_testCartridgePath;
    QString m_testCartridgeGuid;
    QString m_testAppId;
};

void TestQmlAppBridge::initTestCase()
{
    // Initialize temporary directory
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    // Initialize local database
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    // Create test cartridge
    m_testCartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testCartridgePath = createTestCartridge(m_testCartridgeGuid);
    QVERIFY(!m_testCartridgePath.isEmpty());
    
    m_testAppId = "test_app_1";
    
    // Cleanup any existing sandbox
    cleanupSandbox(m_testCartridgeGuid, m_testAppId);
}

void TestQmlAppBridge::cleanupTestCase()
{
    // Cleanup sandbox
    cleanupSandbox(m_testCartridgeGuid, m_testAppId);
    
    // Remove test cartridge file
    if (QFile::exists(m_testCartridgePath)) {
        QFile::remove(m_testCartridgePath);
    }
    
    // Close database
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

// T-BRIDGE-01: Expose bridge to QML (property access)
void TestQmlAppBridge::testPropertyAccess()
{
    QmlAppBridge bridge(this);
    
    // Test initial values
    QCOMPARE(bridge.cartridgePath(), QString());
    QCOMPARE(bridge.cartridgeGuid(), QString());
    
    // Test setting values
    bridge.setCartridgePath(m_testCartridgePath);
    QCOMPARE(bridge.cartridgePath(), m_testCartridgePath);
    
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    QCOMPARE(bridge.cartridgeGuid(), m_testCartridgeGuid);
}

void TestQmlAppBridge::testPropertySignals()
{
    QmlAppBridge bridge(this);
    
    QSignalSpy pathSpy(&bridge, &QmlAppBridge::cartridgePathChanged);
    QSignalSpy guidSpy(&bridge, &QmlAppBridge::cartridgeGuidChanged);
    
    // Setting same value should not emit signal
    bridge.setCartridgePath(QString());
    QCOMPARE(pathSpy.count(), 0);
    
    // Setting new value should emit signal
    bridge.setCartridgePath(m_testCartridgePath);
    QCOMPARE(pathSpy.count(), 1);
    
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    QCOMPARE(guidSpy.count(), 1);
}

// T-BRIDGE-02: Save form data from QML
void TestQmlAppBridge::testSaveFormData()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgePath(m_testCartridgePath);
    
    QSignalSpy spy(&bridge, &QmlAppBridge::formDataSaved);
    
    QString formId = "test_form_1";
    QString dataJson = R"({"name": "Test", "value": 123})";
    
    bridge.saveFormData(formId, dataJson);
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), formId);
    QCOMPARE(arguments.at(1).toBool(), true);
    QVERIFY(arguments.at(2).toString().isEmpty());
    
    // Verify data was saved
    CartridgeDBConnector connector(this);
    QVERIFY(connector.openCartridge(m_testCartridgePath));
    
    QSqlQuery query(connector.getDatabase());
    query.prepare("SELECT data_json FROM User_Data WHERE form_id = ?");
    query.addBindValue(formId);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), dataJson);
    
    connector.closeCartridge();
}

void TestQmlAppBridge::testSaveFormDataNoCartridge()
{
    QmlAppBridge bridge(this);
    // Don't set cartridge path
    
    QSignalSpy spy(&bridge, &QmlAppBridge::formDataSaved);
    
    bridge.saveFormData("form1", R"({"test": "data"})");
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toBool(), false);
    QVERIFY(!arguments.at(2).toString().isEmpty());
}

void TestQmlAppBridge::testSaveFormDataInvalidCartridge()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgePath("/nonexistent/path/to/cartridge.db");
    
    QSignalSpy spy(&bridge, &QmlAppBridge::formDataSaved);
    
    bridge.saveFormData("form1", R"({"test": "data"})");
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toBool(), false);
    QVERIFY(!arguments.at(2).toString().isEmpty());
}

// T-BRIDGE-03: Load form data from QML
void TestQmlAppBridge::testLoadFormData()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgePath(m_testCartridgePath);
    
    // First save some data
    QString formId = "test_form_2";
    QString dataJson = R"({"name": "Loaded", "value": 456})";
    
    QSignalSpy saveSpy(&bridge, &QmlAppBridge::formDataSaved);
    bridge.saveFormData(formId, dataJson);
    QApplication::processEvents();
    QCOMPARE(saveSpy.count(), 1);
    
    // Now load it
    QSignalSpy loadSpy(&bridge, &QmlAppBridge::formDataLoaded);
    bridge.loadFormData(formId);
    
    QApplication::processEvents();
    QCOMPARE(loadSpy.count(), 1);
    
    QList<QVariant> arguments = loadSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), formId);
    QCOMPARE(arguments.at(1).toString(), dataJson);
    QVERIFY(arguments.at(2).toString().isEmpty());
}

void TestQmlAppBridge::testLoadFormDataNotFound()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgePath(m_testCartridgePath);
    
    QSignalSpy spy(&bridge, &QmlAppBridge::formDataLoaded);
    bridge.loadFormData("nonexistent_form");
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toString(), QString());
    QVERIFY(arguments.at(2).toString().isEmpty()); // Not an error
}

void TestQmlAppBridge::testLoadFormDataNoCartridge()
{
    QmlAppBridge bridge(this);
    // Don't set cartridge path
    
    QSignalSpy spy(&bridge, &QmlAppBridge::formDataLoaded);
    bridge.loadFormData("form1");
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toString(), QString());
    QVERIFY(!arguments.at(2).toString().isEmpty());
}

// T-BRIDGE-04: Request consent from QML
void TestQmlAppBridge::testRequestAppConsentPersistentTrust()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    
    // Set persistent trust
    TrustRegistry trustRegistry(this);
    trustRegistry.storeTrustDecision(
        m_testCartridgeGuid,
        TrustRegistry::TrustPolicy::PERSISTENT
    );
    
    QSignalSpy spy(&bridge, &QmlAppBridge::appConsentGranted);
    bridge.requestAppConsent(m_testAppId);
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), m_testAppId);
    
    // Cleanup - revoke trust
    trustRegistry.revokeTrust(m_testCartridgeGuid);
}

void TestQmlAppBridge::testRequestAppConsentRevoked()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    
    // Set revoked trust
    TrustRegistry trustRegistry(this);
    trustRegistry.storeTrustDecision(
        m_testCartridgeGuid,
        TrustRegistry::TrustPolicy::REVOKED
    );
    
    QSignalSpy spy(&bridge, &QmlAppBridge::appConsentDenied);
    bridge.requestAppConsent(m_testAppId);
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), m_testAppId);
    
    // Cleanup - revoke trust
    trustRegistry.revokeTrust(m_testCartridgeGuid);
}

void TestQmlAppBridge::testRequestAppConsentNoCartridge()
{
    QmlAppBridge bridge(this);
    // Don't set cartridge GUID
    
    QSignalSpy spy(&bridge, &QmlAppBridge::appConsentDenied);
    bridge.requestAppConsent(m_testAppId);
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
}

// T-BRIDGE-05: Sandbox file operations from QML
void TestQmlAppBridge::testSaveSandboxFile()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    bridge.setAppId(m_testAppId);
    
    QSignalSpy spy(&bridge, &QmlAppBridge::sandboxFileSaved);
    
    QString filename = "test_file.txt";
    QString data = "Test file content";
    
    bridge.saveSandboxFile(filename, data);
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), filename);
    QCOMPARE(arguments.at(1).toBool(), true);
    QVERIFY(arguments.at(2).toString().isEmpty());
    
    // Verify file was created
    QString sandboxPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDataDir(sandboxPath);
    QString filePath = appDataDir.filePath("sandbox/" + m_testCartridgeGuid + "/" + m_testAppId + "/" + filename);
    QVERIFY(QFile::exists(filePath));
    
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    QCOMPARE(in.readAll(), data);
    file.close();
}

void TestQmlAppBridge::testLoadSandboxFile()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    bridge.setAppId(m_testAppId);
    
    // First save a file
    QString filename = "load_test.txt";
    QString data = "Load test content";
    
    QSignalSpy saveSpy(&bridge, &QmlAppBridge::sandboxFileSaved);
    bridge.saveSandboxFile(filename, data);
    QApplication::processEvents();
    QCOMPARE(saveSpy.count(), 1);
    
    // Now load it
    QSignalSpy loadSpy(&bridge, &QmlAppBridge::sandboxFileLoaded);
    bridge.loadSandboxFile(filename);
    
    QApplication::processEvents();
    QCOMPARE(loadSpy.count(), 1);
    
    QList<QVariant> arguments = loadSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), filename);
    QCOMPARE(arguments.at(1).toString(), data);
    QVERIFY(arguments.at(2).toString().isEmpty());
}

void TestQmlAppBridge::testListSandboxFiles()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    bridge.setAppId(m_testAppId);
    
    // Save multiple files
    QSignalSpy saveSpy(&bridge, &QmlAppBridge::sandboxFileSaved);
    bridge.saveSandboxFile("file1.txt", "Content 1");
    QApplication::processEvents();
    QCOMPARE(saveSpy.count(), 1);
    bridge.saveSandboxFile("file2.txt", "Content 2");
    QApplication::processEvents();
    QCOMPARE(saveSpy.count(), 2);
    
    // List files
    QSignalSpy listSpy(&bridge, &QmlAppBridge::sandboxFilesListed);
    bridge.listSandboxFiles();
    
    QApplication::processEvents();
    QCOMPARE(listSpy.count(), 1);
    
    QList<QVariant> arguments = listSpy.takeFirst();
    QStringList files = arguments.at(0).toStringList();
    QVERIFY(files.contains("file1.txt"));
    QVERIFY(files.contains("file2.txt"));
    QVERIFY(arguments.at(1).toString().isEmpty());
}

void TestQmlAppBridge::testDeleteSandboxFile()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    bridge.setAppId(m_testAppId);
    
    // First save a file
    QString filename = "delete_test.txt";
    QSignalSpy saveSpy(&bridge, &QmlAppBridge::sandboxFileSaved);
    bridge.saveSandboxFile(filename, "Delete test");
    QApplication::processEvents();
    QCOMPARE(saveSpy.count(), 1);
    
    // Verify it exists
    QString sandboxPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDataDir(sandboxPath);
    QString filePath = appDataDir.filePath("sandbox/" + m_testCartridgeGuid + "/" + m_testAppId + "/" + filename);
    QVERIFY(QFile::exists(filePath));
    
    // Delete it
    QSignalSpy deleteSpy(&bridge, &QmlAppBridge::sandboxFileDeleted);
    bridge.deleteSandboxFile(filename);
    
    QApplication::processEvents();
    QCOMPARE(deleteSpy.count(), 1);
    
    QList<QVariant> arguments = deleteSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), filename);
    QCOMPARE(arguments.at(1).toBool(), true);
    QVERIFY(arguments.at(2).toString().isEmpty());
    
    // Verify it's gone
    QVERIFY(!QFile::exists(filePath));
}

void TestQmlAppBridge::testSandboxFilenameValidation()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    bridge.setAppId(m_testAppId);
    
    QSignalSpy spy(&bridge, &QmlAppBridge::sandboxFileSaved);
    
    // Try to save with invalid filename (directory traversal)
    bridge.saveSandboxFile("../invalid.txt", "Test");
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toBool(), false);
    QVERIFY(!arguments.at(2).toString().isEmpty());
}

void TestQmlAppBridge::testSandboxNoAppId()
{
    QmlAppBridge bridge(this);
    bridge.setCartridgeGuid(m_testCartridgeGuid);
    // Don't set app ID
    
    QSignalSpy spy(&bridge, &QmlAppBridge::sandboxFileSaved);
    bridge.saveSandboxFile("test.txt", "Test");
    
    QApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toBool(), false);
    QVERIFY(!arguments.at(2).toString().isEmpty());
}

// T-BRIDGE-06: Log message from QML
void TestQmlAppBridge::testLogMessage()
{
    QmlAppBridge bridge(this);
    
    // Test different log levels (just verify no crash)
    bridge.logMessage("debug", "Debug message");
    bridge.logMessage("info", "Info message");
    bridge.logMessage("warn", "Warning message");
    bridge.logMessage("error", "Error message");
    
    // If we get here, test passes
    QVERIFY(true);
}

// T-BRIDGE-07: Signal emission to QML
void TestQmlAppBridge::testSignalEmission()
{
    QmlAppBridge bridge(this);
    
    // Test that all signals are properly declared and can be connected
    QSignalSpy formSavedSpy(&bridge, &QmlAppBridge::formDataSaved);
    QSignalSpy formLoadedSpy(&bridge, &QmlAppBridge::formDataLoaded);
    QSignalSpy consentGrantedSpy(&bridge, &QmlAppBridge::appConsentGranted);
    QSignalSpy consentDeniedSpy(&bridge, &QmlAppBridge::appConsentDenied);
    QSignalSpy fileSavedSpy(&bridge, &QmlAppBridge::sandboxFileSaved);
    QSignalSpy fileLoadedSpy(&bridge, &QmlAppBridge::sandboxFileLoaded);
    QSignalSpy filesListedSpy(&bridge, &QmlAppBridge::sandboxFilesListed);
    QSignalSpy fileDeletedSpy(&bridge, &QmlAppBridge::sandboxFileDeleted);
    
    // Verify all spies are valid (signals exist)
    QVERIFY(formSavedSpy.isValid());
    QVERIFY(formLoadedSpy.isValid());
    QVERIFY(consentGrantedSpy.isValid());
    QVERIFY(consentDeniedSpy.isValid());
    QVERIFY(fileSavedSpy.isValid());
    QVERIFY(fileLoadedSpy.isValid());
    QVERIFY(filesListedSpy.isValid());
    QVERIFY(fileDeletedSpy.isValid());
}

QTEST_MAIN(TestQmlAppBridge)
#include "test_qmlappbridge.moc"

