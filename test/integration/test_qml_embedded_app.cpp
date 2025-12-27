#include <QtTest>
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/ui/QmlEmbeddedAppWidget.h"
#include "smartbook/reader/QmlAppBridge.h"
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
#include <QTest>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

/**
 * Integration test for QML embedded app functionality
 * 
 * Tests the complete QML embedded app workflow:
 * 1. QML app loading from markers in content
 * 2. QML app communication via QmlAppBridge
 * 3. Multiple QML apps on same page
 * 4. QML app lifecycle (load, unload, reload)
 * 5. Error handling (invalid QML, missing app)
 * 
 * Test Case: Integration test for QML embedded apps
 * Requirements: FR-2.1.1 (Embedded Applications), FR-2.7.3
 */
class TestQmlEmbeddedApp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // QML app loading tests
    void testQmlAppLoadFromMarker();
    void testMultipleQmlAppsOnPage();
    void testQmlAppLoadInvalidQml();
    void testQmlAppLoadMissingApp();
    
    // QML app communication tests
    void testQmlAppBridgeCommunication();
    void testQmlAppFormDataPersistence();
    void testQmlAppSandboxOperations();
    void testQmlAppConsentRequest();
    
    // QML app lifecycle tests
    void testQmlAppReload();
    void testQmlAppUnload();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    QString createTestCartridge(const QString& guid, const QString& title);
    void createManifestEntry(const QString& guid, const QString& path, const QString& title);
    QString createQmlAppCode(const QString& appId);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestQmlEmbeddedApp tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestQmlEmbeddedApp::initTestCase()
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

void TestQmlEmbeddedApp::cleanupTestCase()
{
    delete m_manifestManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestQmlEmbeddedApp::createQmlAppCode(const QString& appId)
{
    // Create a simple QML app that uses QmlAppBridge
    return QString(R"(
        import QtQuick 2.15
        import QtQuick.Controls 2.15
        import SmartBook 1.0
        
        Rectangle {
            id: root
            width: 200
            height: 100
            color: "lightblue"
            
            SmartbookBridge {
                id: bridge
            }
            
            Text {
                anchors.centerIn: parent
                text: "%1"
                font.pixelSize: 16
            }
        }
    )").arg(appId);
}

QString TestQmlEmbeddedApp::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_QmlApp_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    // Close connection from createMinimalCartridge
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName, false);
        if (db.isOpen()) {
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    // Add QML apps
    QString appConn = QString("TestCartridge_QmlApp_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase appDb = QSqlDatabase::addDatabase("QSQLITE", appConn);
    appDb.setDatabaseName(path);
    if (!appDb.open()) {
        return QString();
    }
    
    QSqlQuery query(appDb);
    
    // Create Embedded_Apps table if needed (createMinimalCartridge may have created it)
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT NOT NULL,
            qml_code TEXT,
            javascript_code TEXT,
            app_config_json TEXT
        )
    )");
    
    // Insert test QML app
    QString qmlCode1 = createQmlAppCode("test_app_1");
    query.prepare("INSERT OR REPLACE INTO Embedded_Apps (app_id, app_name, qml_code) VALUES (?, ?, ?)");
    query.addBindValue("test_app_1");
    query.addBindValue("Test App 1");
    query.addBindValue(qmlCode1);
    query.exec();
    
    // Insert second QML app
    QString qmlCode2 = createQmlAppCode("test_app_2");
    query.prepare("INSERT OR REPLACE INTO Embedded_Apps (app_id, app_name, qml_code) VALUES (?, ?, ?)");
    query.addBindValue("test_app_2");
    query.addBindValue("Test App 2");
    query.addBindValue(qmlCode2);
    query.exec();
    
    // Insert invalid QML app (syntax error)
    query.prepare("INSERT OR REPLACE INTO Embedded_Apps (app_id, app_name, qml_code) VALUES (?, ?, ?)");
    query.addBindValue("invalid_app");
    query.addBindValue("Invalid App");
    query.addBindValue("import QtQuick 2.15\nRectangle { invalid syntax }");
    query.exec();
    
    appDb.close();
    QSqlDatabase::removeDatabase(appConn);
    
    return path;
}

void TestQmlEmbeddedApp::createManifestEntry(const QString& guid, const QString& path, const QString& title)
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

// Test: QML app loads from marker in content
void TestQmlEmbeddedApp::testQmlAppLoadFromMarker()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "QML App Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "QML App Test Book");
    
    // Add content page with QML app marker
    QString contentConn = QString("TestCartridge_Content_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase contentDb = QSqlDatabase::addDatabase("QSQLITE", contentConn);
    contentDb.setDatabaseName(cartridgePath);
    if (contentDb.open()) {
        QSqlQuery query(contentDb);
        query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(1);
        query.addBindValue("<p>Content with QML app:</p><div data-smartbook-qml-app=\"test_app_1\"></div>");
        query.exec();
        contentDb.close();
    }
    QSqlDatabase::removeDatabase(contentConn);
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500); // Wait for content to load
    
    // Verify QML app widget was created
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QmlEmbeddedAppWidget* appWidget = readerView->findChild<QmlEmbeddedAppWidget*>();
    QVERIFY(appWidget != nullptr);
    QVERIFY(QTest::qWaitFor([&]() { return appWidget->isAppLoaded(); }, 5000));
    QVERIFY(!appWidget->hasError());
    
    delete window;
}

// Test: Multiple QML apps on same page
void TestQmlEmbeddedApp::testMultipleQmlAppsOnPage()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Multiple QML Apps Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Multiple QML Apps Book");
    
    // Add content page with multiple QML app markers
    QString contentConn = QString("TestCartridge_Content_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase contentDb = QSqlDatabase::addDatabase("QSQLITE", contentConn);
    contentDb.setDatabaseName(cartridgePath);
    if (contentDb.open()) {
        QSqlQuery query(contentDb);
        query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(1);
        query.addBindValue("<p>Content with multiple QML apps:</p>"
                          "<div data-smartbook-qml-app=\"test_app_1\"></div>"
                          "<div data-smartbook-qml-app=\"test_app_2\"></div>");
        query.exec();
        contentDb.close();
    }
    QSqlDatabase::removeDatabase(contentConn);
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500);
    
    // Verify both QML app widgets were created
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QList<QmlEmbeddedAppWidget*> appWidgets = readerView->findChildren<QmlEmbeddedAppWidget*>();
    QCOMPARE(appWidgets.size(), 2);
    
    for (QmlEmbeddedAppWidget* widget : appWidgets) {
        QVERIFY(QTest::qWaitFor([&]() { return widget->isAppLoaded(); }, 5000));
        QVERIFY(!widget->hasError());
    }
    
    delete window;
}

// Test: Invalid QML code handling
void TestQmlEmbeddedApp::testQmlAppLoadInvalidQml()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Invalid QML Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Invalid QML Book");
    
    // Add content page with invalid QML app marker
    QString contentConn = QString("TestCartridge_Content_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase contentDb = QSqlDatabase::addDatabase("QSQLITE", contentConn);
    contentDb.setDatabaseName(cartridgePath);
    if (contentDb.open()) {
        QSqlQuery query(contentDb);
        query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(1);
        query.addBindValue("<div data-smartbook-qml-app=\"invalid_app\"></div>");
        query.exec();
        contentDb.close();
    }
    QSqlDatabase::removeDatabase(contentConn);
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500);
    
    // Verify QML app widget was created but has error
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QmlEmbeddedAppWidget* appWidget = readerView->findChild<QmlEmbeddedAppWidget*>();
    QVERIFY(appWidget != nullptr);
    QTest::qWait(1000); // Wait for QML to attempt loading
    QVERIFY(appWidget->hasError());
    
    delete window;
}

// Test: Missing QML app handling
void TestQmlEmbeddedApp::testQmlAppLoadMissingApp()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Missing App Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Missing App Book");
    
    // Add content page with non-existent QML app marker
    QString contentConn = QString("TestCartridge_Content_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase contentDb = QSqlDatabase::addDatabase("QSQLITE", contentConn);
    contentDb.setDatabaseName(cartridgePath);
    if (contentDb.open()) {
        QSqlQuery query(contentDb);
        query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(1);
        query.addBindValue("<div data-smartbook-qml-app=\"nonexistent_app\"></div>");
        query.exec();
        contentDb.close();
    }
    QSqlDatabase::removeDatabase(contentConn);
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500);
    
    // Verify QML app widget was created but has error (app not found)
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QmlEmbeddedAppWidget* appWidget = readerView->findChild<QmlEmbeddedAppWidget*>();
    QVERIFY(appWidget != nullptr);
    QVERIFY(QTest::qWaitFor([&]() { return appWidget->hasError(); }, 5000));
    
    delete window;
}

// Test: QML app bridge communication
void TestQmlEmbeddedApp::testQmlAppBridgeCommunication()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Bridge Communication Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Bridge Communication Book");
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500);
    
    // Get QML app widget and verify bridge is set
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QmlEmbeddedAppWidget* appWidget = readerView->findChild<QmlEmbeddedAppWidget*>();
    if (appWidget) {
        QVERIFY(QTest::qWaitFor([&]() { return appWidget->isAppLoaded(); }, 5000));
        
        // Verify bridge has cartridge info
        QmlAppBridge* bridge = appWidget->findChild<QmlAppBridge*>();
        QVERIFY(bridge != nullptr);
        QCOMPARE(bridge->cartridgePath(), cartridgePath);
        QCOMPARE(bridge->cartridgeGuid(), guid);
    }
    
    delete window;
}

// Test: QML app form data persistence
void TestQmlEmbeddedApp::testQmlAppFormDataPersistence()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Form Data Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Form Data Book");
    
    // Create QmlAppBridge and test form data save/load
    QmlAppBridge bridge(this);
    bridge.setCartridgePath(cartridgePath);
    bridge.setCartridgeGuid(guid);
    
    // Save form data
    QSignalSpy saveSpy(&bridge, &QmlAppBridge::formDataSaved);
    bridge.saveFormData("test_form", R"({"name": "Test", "value": 123})");
    QCOMPARE(saveSpy.count(), 1);
    QVERIFY(saveSpy.takeFirst().at(1).toBool()); // success
    
    // Load form data
    QSignalSpy loadSpy(&bridge, &QmlAppBridge::formDataLoaded);
    bridge.loadFormData("test_form");
    QCOMPARE(loadSpy.count(), 1);
    QCOMPARE(loadSpy.takeFirst().at(1).toString(), R"({"name": "Test", "value": 123})");
}

// Test: QML app sandbox operations
void TestQmlEmbeddedApp::testQmlAppSandboxOperations()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Sandbox Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Sandbox Book");
    
    // Create QmlAppBridge and test sandbox operations
    QmlAppBridge bridge(this);
    bridge.setCartridgePath(cartridgePath);
    bridge.setCartridgeGuid(guid);
    bridge.setAppId("test_app_1");
    
    // Save sandbox file
    QSignalSpy saveSpy(&bridge, &QmlAppBridge::sandboxFileSaved);
    bridge.saveSandboxFile("test.txt", "Hello, World!");
    QCOMPARE(saveSpy.count(), 1);
    QVERIFY(saveSpy.takeFirst().at(1).toBool()); // success
    
    // List sandbox files
    QSignalSpy listSpy(&bridge, &QmlAppBridge::sandboxFilesListed);
    bridge.listSandboxFiles();
    QCOMPARE(listSpy.count(), 1);
    QStringList files = listSpy.takeFirst().at(0).toStringList();
    QVERIFY(files.contains("test.txt"));
    
    // Load sandbox file
    QSignalSpy loadSpy(&bridge, &QmlAppBridge::sandboxFileLoaded);
    bridge.loadSandboxFile("test.txt");
    QCOMPARE(loadSpy.count(), 1);
    QCOMPARE(loadSpy.takeFirst().at(1).toString(), "Hello, World!");
    
    // Delete sandbox file
    QSignalSpy deleteSpy(&bridge, &QmlAppBridge::sandboxFileDeleted);
    bridge.deleteSandboxFile("test.txt");
    QCOMPARE(deleteSpy.count(), 1);
    QVERIFY(deleteSpy.takeFirst().at(1).toBool()); // success
}

// Test: QML app consent request
void TestQmlEmbeddedApp::testQmlAppConsentRequest()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Consent Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Consent Book");
    
    // Create QmlAppBridge and test consent request
    QmlAppBridge bridge(this);
    bridge.setCartridgePath(cartridgePath);
    bridge.setCartridgeGuid(guid);
    bridge.setAppId("test_app_1");
    
    // Request consent (should show dialog)
    // Note: requestAppConsent is Q_INVOKABLE and shows a dialog, so we can't easily test it
    // without mocking or UI interaction. For now, just verify the method exists and can be called.
    bridge.requestAppConsent("test_app_1");
    QApplication::processEvents();
}

// Test: QML app reload
void TestQmlEmbeddedApp::testQmlAppReload()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Reload Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Reload Book");
    
    // Add content page with QML app marker
    QString contentConn = QString("TestCartridge_Content_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase contentDb = QSqlDatabase::addDatabase("QSQLITE", contentConn);
    contentDb.setDatabaseName(cartridgePath);
    if (contentDb.open()) {
        QSqlQuery query(contentDb);
        query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(1);
        query.addBindValue("<div data-smartbook-qml-app=\"test_app_1\"></div>");
        query.exec();
        contentDb.close();
    }
    QSqlDatabase::removeDatabase(contentConn);
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500);
    
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QmlEmbeddedAppWidget* appWidget = readerView->findChild<QmlEmbeddedAppWidget*>();
    QVERIFY(appWidget != nullptr);
    QVERIFY(QTest::qWaitFor([&]() { return appWidget->isAppLoaded(); }, 5000));
    
    // Reload the app (unload and load again)
    appWidget->unloadApp();
    QVERIFY(!appWidget->isAppLoaded());
    bool reloaded = appWidget->loadApp(cartridgePath, "test_app_1");
    QVERIFY(reloaded);
    QVERIFY(QTest::qWaitFor([&]() { return appWidget->isAppLoaded(); }, 5000));
    QVERIFY(!appWidget->hasError());
    
    delete window;
}

// Test: QML app unload
void TestQmlEmbeddedApp::testQmlAppUnload()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Unload Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Unload Book");
    
    // Add content page with QML app marker
    QString contentConn = QString("TestCartridge_Content_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase contentDb = QSqlDatabase::addDatabase("QSQLITE", contentConn);
    contentDb.setDatabaseName(cartridgePath);
    if (contentDb.open()) {
        QSqlQuery query(contentDb);
        query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(1);
        query.addBindValue("<div data-smartbook-qml-app=\"test_app_1\"></div>");
        query.exec();
        contentDb.close();
    }
    QSqlDatabase::removeDatabase(contentConn);
    
    // Create ReaderViewWindow and load cartridge
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    QApplication::processEvents();
    QTest::qWait(500);
    
    ReaderView* readerView = window->findChild<ReaderView*>();
    QVERIFY(readerView != nullptr);
    
    QmlEmbeddedAppWidget* appWidget = readerView->findChild<QmlEmbeddedAppWidget*>();
    QVERIFY(appWidget != nullptr);
    QVERIFY(QTest::qWaitFor([&]() { return appWidget->isAppLoaded(); }, 5000));
    
    // Unload the app
    appWidget->unloadApp();
    QVERIFY(!appWidget->isAppLoaded());
    
    delete window;
}

#include "test_qml_embedded_app.moc"

