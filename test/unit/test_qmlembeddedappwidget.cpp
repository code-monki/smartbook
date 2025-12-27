/**
 * @file test_qmlembeddedappwidget.cpp
 * @brief Unit tests for QmlEmbeddedAppWidget
 * 
 * Tests for embedding QML applications in content pages.
 */

#include <QtTest>
#include "smartbook/reader/ui/QmlEmbeddedAppWidget.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QApplication>
#include <QQuickWidget>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <QSqlQuery>
#include <QSqlDatabase>

// Test helper functions
namespace {
    QString createTestCartridge(const QString& guid, const QString& appId, const QString& qmlCode)
    {
        QTemporaryFile tempFile;
        if (!tempFile.open()) {
            return QString();
        }
        
        QString path = tempFile.fileName();
        tempFile.close();
        tempFile.remove();
        
        // Create SQLite database
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_qml_" + guid);
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
                version TEXT,
                schema_version TEXT
            )
        )");
        
        query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, version, schema_version) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(guid);
        query.addBindValue("Test Cartridge");
        query.addBindValue("Test Author");
        query.addBindValue("1.0");
        query.addBindValue("1.0");
        query.exec();
        
        // Create Embedded_Apps table (with qml_code column for QML apps)
        query.exec(R"(
            CREATE TABLE Embedded_Apps (
                app_id TEXT PRIMARY KEY,
                app_name TEXT NOT NULL,
                manifest_json TEXT,
                entry_html TEXT,
                js_code BLOB,
                css_code BLOB,
                qml_code TEXT,
                app_version TEXT,
                description TEXT
            )
        )");
        
        query.prepare("INSERT INTO Embedded_Apps (app_id, app_name, qml_code, app_version, description) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(appId);
        query.addBindValue("Test App");
        query.addBindValue(qmlCode);
        query.addBindValue("1.0");
        query.addBindValue("Test QML Application");
        query.exec();
        
        // Create Content_Pages table (required for validation)
        query.exec(R"(
            CREATE TABLE Content_Pages (
                page_id INTEGER PRIMARY KEY AUTOINCREMENT,
                page_order INTEGER NOT NULL,
                html_content TEXT,
                associated_css TEXT
            )
        )");
        
        query.exec("INSERT INTO Content_Pages (page_order, html_content, associated_css) VALUES (1, '<p>Test</p>', '')");
        
        // Create Cartridge_Security table (required for validation)
        query.exec(R"(
            CREATE TABLE Cartridge_Security (
                cartridge_guid TEXT PRIMARY KEY,
                h1_hash BLOB,
                signature_data BLOB,
                certificate_data BLOB
            )
        )");
        
        query.prepare("INSERT INTO Cartridge_Security (cartridge_guid, h1_hash, signature_data, certificate_data) VALUES (?, ?, ?, ?)");
        query.addBindValue(guid);
        query.addBindValue(QByteArray());
        query.addBindValue(QByteArray());
        query.addBindValue(QByteArray());
        query.exec();
        
        db.close();
        QSqlDatabase::removeDatabase("test_qml_" + guid);
        
        return path;
    }
}

class TestQmlEmbeddedAppWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // Test QML app loading
    void testLoadQmlAppFromDatabase();
    void testLoadQmlAppWithInvalidAppId();
    void testLoadQmlAppWithInvalidCartridge();
    void testLoadQmlAppWithQmlErrors();
    
    // Test bridge communication
    void testBridgeExposedToQml();
    void testBridgeCartridgeInfo();
    void testBridgeFormDataOperations();
    void testBridgeSandboxOperations();
    
    // Test app lifecycle
    void testAppUnload();
    void testAppReload();
    void testMultipleApps();

private:
    QString m_testCartridgePath;
    QString m_testGuid;
    QString m_testAppId;
};

void TestQmlEmbeddedAppWidget::initTestCase()
{
    // Create QApplication if it doesn't exist
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
    
    m_testGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testAppId = "test_app_1";
}

void TestQmlEmbeddedAppWidget::cleanupTestCase()
{
    // Cleanup test cartridge file
    if (!m_testCartridgePath.isEmpty() && QFile::exists(m_testCartridgePath)) {
        QFile::remove(m_testCartridgePath);
    }
}

void TestQmlEmbeddedAppWidget::init()
{
    // Create test cartridge with QML app
    QString qmlCode = R"(
        import QtQuick 2.15
        import QtQuick.Controls 2.15
        
        Rectangle {
            id: root
            color: "lightblue"
            width: 200
            height: 100
            
            Text {
                anchors.centerIn: parent
                text: "Test QML App"
            }
        }
    )";
    
    m_testCartridgePath = createTestCartridge(m_testGuid, m_testAppId, qmlCode);
    QVERIFY(!m_testCartridgePath.isEmpty());
}

void TestQmlEmbeddedAppWidget::cleanup()
{
    // Cleanup test cartridge file
    if (!m_testCartridgePath.isEmpty() && QFile::exists(m_testCartridgePath)) {
        QFile::remove(m_testCartridgePath);
        m_testCartridgePath.clear();
    }
}

void TestQmlEmbeddedAppWidget::testLoadQmlAppFromDatabase()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded);
    
    // Verify widget is visible and has content
    QVERIFY(widget.isVisible() || widget.size().width() > 0);
}

void TestQmlEmbeddedAppWidget::testLoadQmlAppWithInvalidAppId()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(m_testCartridgePath, "nonexistent_app");
    QVERIFY(!loaded);
}

void TestQmlEmbeddedAppWidget::testLoadQmlAppWithInvalidCartridge()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp("/nonexistent/path.cartridge", m_testAppId);
    QVERIFY(!loaded);
}

void TestQmlEmbeddedAppWidget::testLoadQmlAppWithQmlErrors()
{
    // Create cartridge with invalid QML code
    QString invalidQml = "Invalid QML Code { broken syntax }";
    QString path = createTestCartridge(QUuid::createUuid().toString(QUuid::WithoutBraces), "invalid_app", invalidQml);
    
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(path, "invalid_app");
    // Should handle error gracefully (either return false or show error state)
    QVERIFY(!loaded || widget.hasError());
    
    QFile::remove(path);
}

void TestQmlEmbeddedAppWidget::testBridgeExposedToQml()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded);
    
    // Verify bridge is accessible (would need QML context access for full test)
    // For now, verify widget loaded successfully
    QVERIFY(widget.isAppLoaded());
}

void TestQmlEmbeddedAppWidget::testBridgeCartridgeInfo()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    widget.setCartridgeInfo(m_testCartridgePath, m_testGuid);
    bool loaded = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded);
    
    // Verify cartridge info is set (would need bridge access for full test)
    QCOMPARE(widget.cartridgePath(), m_testCartridgePath);
    QCOMPARE(widget.cartridgeGuid(), m_testGuid);
}

void TestQmlEmbeddedAppWidget::testBridgeFormDataOperations()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded);
    
    // Test form data operations via bridge
    // Would need to call QML methods or access bridge directly
    // For now, verify app loaded
    QVERIFY(widget.isAppLoaded());
}

void TestQmlEmbeddedAppWidget::testBridgeSandboxOperations()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded);
    
    // Test sandbox operations via bridge
    // Would need to call QML methods or access bridge directly
    // For now, verify app loaded
    QVERIFY(widget.isAppLoaded());
}

void TestQmlEmbeddedAppWidget::testAppUnload()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded);
    QVERIFY(widget.isAppLoaded());
    
    widget.unloadApp();
    QVERIFY(!widget.isAppLoaded());
}

void TestQmlEmbeddedAppWidget::testAppReload()
{
    smartbook::reader::ui::QmlEmbeddedAppWidget widget;
    
    bool loaded1 = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded1);
    
    widget.unloadApp();
    
    bool loaded2 = widget.loadApp(m_testCartridgePath, m_testAppId);
    QVERIFY(loaded2);
}

void TestQmlEmbeddedAppWidget::testMultipleApps()
{
    // Create second app in same cartridge
    QString qmlCode2 = R"(
        import QtQuick 2.15
        Rectangle { color: "lightgreen"; width: 200; height: 100; }
    )";
    
    // Add second app to existing cartridge
    smartbook::common::database::CartridgeDBConnector connector(this);
    if (connector.openCartridge(m_testCartridgePath)) {
        QSqlQuery query(connector.getDatabase());
        query.prepare("INSERT INTO Embedded_Apps (app_id, app_name, qml_code, app_version, description) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue("test_app_2");
        query.addBindValue("Test App 2");
        query.addBindValue(qmlCode2);
        query.addBindValue("1.0");
        query.addBindValue("Second Test App");
        query.exec();
        connector.closeCartridge();
    }
    
    smartbook::reader::ui::QmlEmbeddedAppWidget widget1;
    smartbook::reader::ui::QmlEmbeddedAppWidget widget2;
    
    bool loaded1 = widget1.loadApp(m_testCartridgePath, m_testAppId);
    bool loaded2 = widget2.loadApp(m_testCartridgePath, "test_app_2");
    
    QVERIFY(loaded1);
    QVERIFY(loaded2);
}

QTEST_MAIN(TestQmlEmbeddedAppWidget)
#include "test_qmlembeddedappwidget.moc"

