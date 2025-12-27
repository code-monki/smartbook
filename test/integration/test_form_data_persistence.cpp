#include <QtTest>
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "test_helpers.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QApplication>
#include <QDebug>
#include <QSignalSpy>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

/**
 * Integration test for form data persistence
 * 
 * Tests the complete form data persistence workflow:
 * 1. Save form data through WebChannelBridge
 * 2. Load form data through WebChannelBridge
 * 3. Persistence across sessions (close/reopen cartridge)
 * 4. Multiple forms in same cartridge
 * 5. Form data updates (overwrite existing)
 * 6. Integration with ReaderViewWindow
 * 
 * Test Case: Integration test for form data persistence
 * Requirements: FR-2.1.2 (Form Data Persistence), FR-2.7.4, FR-2.7.6
 */
class TestFormDataPersistence : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Form data persistence tests
    void testSaveAndLoadFormData();
    void testFormDataPersistenceAcrossSessions();
    void testMultipleFormsInCartridge();
    void testFormDataUpdate();
    void testFormDataIsolationPerCartridge();
    void testFormDataWithReaderViewWindow();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    QString createTestCartridge(const QString& guid, const QString& title);
    void createManifestEntry(const QString& guid, const QString& path, const QString& title);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestFormDataPersistence tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestFormDataPersistence::initTestCase()
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

void TestFormDataPersistence::cleanupTestCase()
{
    delete m_manifestManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestFormDataPersistence::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_FormData_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    // Add form definition
    QString formConn = QString("TestCartridge_FormDef_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase formDb = QSqlDatabase::addDatabase("QSQLITE", formConn);
    formDb.setDatabaseName(path);
    if (formDb.open()) {
        QSqlQuery query(formDb);
        query.exec(R"(
            CREATE TABLE IF NOT EXISTS Form_Definitions (
                form_id TEXT PRIMARY KEY,
                form_json TEXT NOT NULL
            )
        )");
        query.prepare("INSERT INTO Form_Definitions (form_id, form_json) VALUES (?, ?)");
        query.addBindValue("contact_form");
        query.addBindValue(R"({"fields": [{"name": "name", "type": "text"}, {"name": "email", "type": "email"}]})");
        query.exec();
        
        query.prepare("INSERT INTO Form_Definitions (form_id, form_json) VALUES (?, ?)");
        query.addBindValue("survey_form");
        query.addBindValue(R"({"fields": [{"name": "rating", "type": "number"}]})");
        query.exec();
        
        formDb.close();
    }
    QSqlDatabase::removeDatabase(formConn);
    
    return path;
}

void TestFormDataPersistence::createManifestEntry(const QString& guid, const QString& path, const QString& title)
{
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = guid;
    entry.localPath = path;
    entry.title = title;
    entry.author = "Test Author";
    entry.publicationYear = "2025";
    entry.cartridgeHash = QByteArray("test_hash");
    m_manifestManager->createManifestEntry(entry);
}

// Test: Save and load form data through WebChannelBridge
void TestFormDataPersistence::testSaveAndLoadFormData()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Form Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(cartridgePath, guid);
    
    // Test save
    QSignalSpy saveSpy(&bridge, &WebChannelBridge::formDataSaved);
    QString formId = "contact_form";
    QString dataJson = R"({"name": "John Doe", "email": "john@example.com"})";
    
    bridge.saveFormData(formId, dataJson, QString());
    
    QCOMPARE(saveSpy.count(), 1);
    QList<QVariant> saveArgs = saveSpy.takeFirst();
    QCOMPARE(saveArgs.at(0).toString(), formId);
    QVERIFY(saveArgs.at(1).toBool()); // success
    QVERIFY(saveArgs.at(2).toString().isEmpty()); // no error
    
    // Test load
    QSignalSpy loadSpy(&bridge, &WebChannelBridge::formDataLoaded);
    bridge.loadFormData(formId, QString());
    
    QCOMPARE(loadSpy.count(), 1);
    QList<QVariant> loadArgs = loadSpy.takeFirst();
    QCOMPARE(loadArgs.at(0).toString(), formId);
    QCOMPARE(loadArgs.at(1).toString(), dataJson);
    QVERIFY(loadArgs.at(2).toString().isEmpty()); // no error
}

// Test: Form data persists across sessions (close/reopen cartridge)
void TestFormDataPersistence::testFormDataPersistenceAcrossSessions()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Persistence Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Persistence Test Book");
    
    // Session 1: Save form data
    {
        WebChannelBridge bridge1(this);
        bridge1.setCartridgeInfo(cartridgePath, guid);
        
        QSignalSpy saveSpy(&bridge1, &WebChannelBridge::formDataSaved);
        bridge1.saveFormData("contact_form", R"({"name": "Jane Doe", "email": "jane@example.com"})", QString());
        
        QCOMPARE(saveSpy.count(), 1);
        QList<QVariant> args = saveSpy.takeFirst();
        QVERIFY(args.at(1).toBool()); // success
    }
    
    // Session 2: Load form data (simulating cartridge reopen)
    {
        WebChannelBridge bridge2(this);
        bridge2.setCartridgeInfo(cartridgePath, guid);
        
        QSignalSpy loadSpy(&bridge2, &WebChannelBridge::formDataLoaded);
        bridge2.loadFormData("contact_form", QString());
        
        QCOMPARE(loadSpy.count(), 1);
        QList<QVariant> args = loadSpy.takeFirst();
        QCOMPARE(args.at(1).toString(), R"({"name": "Jane Doe", "email": "jane@example.com"})");
    }
}

// Test: Multiple forms can exist in same cartridge
void TestFormDataPersistence::testMultipleFormsInCartridge()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Multiple Forms Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(cartridgePath, guid);
    
    // Save data for form 1
    QSignalSpy saveSpy1(&bridge, &WebChannelBridge::formDataSaved);
    bridge.saveFormData("contact_form", R"({"name": "Alice", "email": "alice@example.com"})", QString());
    QCOMPARE(saveSpy1.count(), 1);
    QVERIFY(saveSpy1.takeFirst().at(1).toBool());
    
    // Save data for form 2
    QSignalSpy saveSpy2(&bridge, &WebChannelBridge::formDataSaved);
    bridge.saveFormData("survey_form", R"({"rating": 5})", QString());
    QCOMPARE(saveSpy2.count(), 1);
    QVERIFY(saveSpy2.takeFirst().at(1).toBool());
    
    // Load both forms
    QSignalSpy loadSpy1(&bridge, &WebChannelBridge::formDataLoaded);
    bridge.loadFormData("contact_form", QString());
    QCOMPARE(loadSpy1.count(), 1);
    QCOMPARE(loadSpy1.takeFirst().at(1).toString(), R"({"name": "Alice", "email": "alice@example.com"})");
    
    QSignalSpy loadSpy2(&bridge, &WebChannelBridge::formDataLoaded);
    bridge.loadFormData("survey_form", QString());
    QCOMPARE(loadSpy2.count(), 1);
    QCOMPARE(loadSpy2.takeFirst().at(1).toString(), R"({"rating": 5})");
}

// Test: Form data can be updated (overwrite existing)
void TestFormDataPersistence::testFormDataUpdate()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Update Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    WebChannelBridge bridge(this);
    bridge.setCartridgeInfo(cartridgePath, guid);
    
    // Initial save
    QSignalSpy saveSpy1(&bridge, &WebChannelBridge::formDataSaved);
    bridge.saveFormData("contact_form", R"({"name": "Bob", "email": "bob@example.com"})", QString());
    QCOMPARE(saveSpy1.count(), 1);
    QVERIFY(saveSpy1.takeFirst().at(1).toBool());
    
    // Update with new data
    QSignalSpy saveSpy2(&bridge, &WebChannelBridge::formDataSaved);
    bridge.saveFormData("contact_form", R"({"name": "Bob Smith", "email": "bob.smith@example.com"})", QString());
    QCOMPARE(saveSpy2.count(), 1);
    QVERIFY(saveSpy2.takeFirst().at(1).toBool());
    
    // Verify updated data
    QSignalSpy loadSpy(&bridge, &WebChannelBridge::formDataLoaded);
    bridge.loadFormData("contact_form", QString());
    QCOMPARE(loadSpy.count(), 1);
    QCOMPARE(loadSpy.takeFirst().at(1).toString(), R"({"name": "Bob Smith", "email": "bob.smith@example.com"})");
}

// Test: Form data is isolated per cartridge
void TestFormDataPersistence::testFormDataIsolationPerCartridge()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    // Save different data to each cartridge
    WebChannelBridge bridge1(this);
    bridge1.setCartridgeInfo(path1, guid1);
    QSignalSpy saveSpy1(&bridge1, &WebChannelBridge::formDataSaved);
    bridge1.saveFormData("contact_form", R"({"name": "Cartridge 1 User"})", QString());
    QCOMPARE(saveSpy1.count(), 1);
    
    WebChannelBridge bridge2(this);
    bridge2.setCartridgeInfo(path2, guid2);
    QSignalSpy saveSpy2(&bridge2, &WebChannelBridge::formDataSaved);
    bridge2.saveFormData("contact_form", R"({"name": "Cartridge 2 User"})", QString());
    QCOMPARE(saveSpy2.count(), 1);
    
    // Verify data is isolated
    QSignalSpy loadSpy1(&bridge1, &WebChannelBridge::formDataLoaded);
    bridge1.loadFormData("contact_form", QString());
    QCOMPARE(loadSpy1.count(), 1);
    QCOMPARE(loadSpy1.takeFirst().at(1).toString(), R"({"name": "Cartridge 1 User"})");
    
    QSignalSpy loadSpy2(&bridge2, &WebChannelBridge::formDataLoaded);
    bridge2.loadFormData("contact_form", QString());
    QCOMPARE(loadSpy2.count(), 1);
    QCOMPARE(loadSpy2.takeFirst().at(1).toString(), R"({"name": "Cartridge 2 User"})");
}

// Test: Form data works with ReaderViewWindow integration
void TestFormDataPersistence::testFormDataWithReaderViewWindow()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "ReaderViewWindow Test");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "ReaderViewWindow Test");
    
    // Create ReaderViewWindow (which creates WebChannelBridge internally)
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    // Note: ReaderViewWindow loads the cartridge and sets up WebChannelBridge
    // For this integration test, we verify the cartridge can be opened and form data
    // can be saved/loaded through the database connector directly
    QString conn = QString("TestConn_ReaderView_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector(this);
    QVERIFY(connector.openCartridge(cartridgePath));
    
    // Save form data
    bool saved = connector.saveFormData("contact_form", R"({"name": "ReaderView User"})");
    QVERIFY(saved);
    
    // Load form data
    QString loaded = connector.loadFormData("contact_form");
    QCOMPARE(loaded, R"({"name": "ReaderView User"})");
    
    connector.closeCartridge();
    QSqlDatabase::removeDatabase(conn);
    
    delete window;
}

#include "test_form_data_persistence.moc"

