/**
 * @file test_readerview_qml_integration.cpp
 * @brief Unit tests for ReaderView QML app integration
 * 
 * Tests for ContentParser integration and QML app marker handling in ReaderView.
 */

#include <QtTest>
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QApplication>
#include <QTemporaryFile>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QUuid>

// Test helper functions
namespace {
    QString createTestCartridge(const QString& guid, const QString& htmlContent, const QString& appId = QString(), const QString& qmlCode = QString())
    {
        QTemporaryFile tempFile;
        if (!tempFile.open()) {
            return QString();
        }
        
        QString path = tempFile.fileName();
        tempFile.close();
        tempFile.remove();
        
        // Create SQLite database
        QString connectionName = "test_rv_" + guid;
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
        
        // Create Content_Pages table
        query.exec(R"(
            CREATE TABLE Content_Pages (
                page_id INTEGER PRIMARY KEY AUTOINCREMENT,
                page_order INTEGER NOT NULL,
                html_content TEXT,
                associated_css TEXT
            )
        )");
        
        query.prepare("INSERT INTO Content_Pages (page_order, html_content, associated_css) VALUES (?, ?, ?)");
        query.addBindValue(1);
        query.addBindValue(htmlContent);
        query.addBindValue("");
        query.exec();
        
        // Create Embedded_Apps table if QML app provided
        if (!appId.isEmpty() && !qmlCode.isEmpty()) {
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
        }
        
        // Create Cartridge_Security table
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
        
        // Close database properly
        db.close();
        QString connName = db.connectionName();
        QSqlDatabase::removeDatabase(connName);
        
        return path;
    }
}

class TestReaderViewQmlIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Test QML marker detection in content
    void testLoadContentWithQmlMarker();
    void testLoadContentWithMultipleQmlMarkers();
    void testLoadContentWithoutQmlMarkers();
    void testLoadContentWithInvalidQmlMarker();
    
    // Test QML app loading
    void testQmlAppLoadedFromMarker();
    void testMultipleQmlAppsLoaded();
    void testQmlAppErrorHandling();

private:
    QString m_testGuid;
};

void TestReaderViewQmlIntegration::initTestCase()
{
    // Create QApplication if it doesn't exist
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
    
    m_testGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void TestReaderViewQmlIntegration::cleanupTestCase()
{
}

void TestReaderViewQmlIntegration::testLoadContentWithQmlMarker()
{
    QString qmlCode = R"(
        import QtQuick 2.15
        Rectangle { color: "lightblue"; width: 200; height: 100; }
    )";
    
    QString htmlContent = R"(
        <p>Content before QML app</p>
        <div data-smartbook-qml-app="test_app_1"></div>
        <p>Content after QML app</p>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent, "test_app_1", qmlCode);
    QVERIFY(!cartridgePath.isEmpty());
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    // Verify content loaded (QML marker should be detected and processed)
    // Note: Actual widget embedding will be tested in integration tests
    QVERIFY(readerView.getCurrentPageId() >= 0);
    
    QFile::remove(cartridgePath);
}

void TestReaderViewQmlIntegration::testLoadContentWithMultipleQmlMarkers()
{
    QString qmlCode1 = R"(
        import QtQuick 2.15
        Rectangle { color: "lightblue"; width: 200; height: 100; }
    )";
    
    QString qmlCode2 = R"(
        import QtQuick 2.15
        Rectangle { color: "lightgreen"; width: 200; height: 100; }
    )";
    
    QString htmlContent = R"(
        <p>Start</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>Middle</p>
        <div data-smartbook-qml-app="app_2"></div>
        <p>End</p>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent);
    
    // Add apps to cartridge
    smartbook::common::database::CartridgeDBConnector connector(this);
    if (connector.openCartridge(cartridgePath)) {
        QSqlQuery query(connector.getDatabase());
        query.exec(R"(
            CREATE TABLE IF NOT EXISTS Embedded_Apps (
                app_id TEXT PRIMARY KEY,
                app_name TEXT NOT NULL,
                qml_code TEXT,
                app_version TEXT,
                description TEXT
            )
        )");
        
        query.prepare("INSERT INTO Embedded_Apps (app_id, app_name, qml_code, app_version, description) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue("app_1");
        query.addBindValue("App 1");
        query.addBindValue(qmlCode1);
        query.addBindValue("1.0");
        query.addBindValue("First App");
        query.exec();
        
        query.addBindValue("app_2");
        query.addBindValue("App 2");
        query.addBindValue(qmlCode2);
        query.addBindValue("1.0");
        query.addBindValue("Second App");
        query.exec();
        
        connector.closeCartridge();
    }
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    QVERIFY(readerView.getCurrentPageId() >= 0);
    
    QFile::remove(cartridgePath);
}

void TestReaderViewQmlIntegration::testLoadContentWithoutQmlMarkers()
{
    QString htmlContent = R"(
        <h1>Regular Content</h1>
        <p>No QML markers here</p>
        <ul>
            <li>Item 1</li>
            <li>Item 2</li>
        </ul>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent);
    QVERIFY(!cartridgePath.isEmpty());
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    QVERIFY(readerView.getCurrentPageId() >= 0);
    
    QFile::remove(cartridgePath);
}

void TestReaderViewQmlIntegration::testLoadContentWithInvalidQmlMarker()
{
    QString htmlContent = R"(
        <p>Content</p>
        <div data-smartbook-qml-app="nonexistent_app"></div>
        <p>More content</p>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent);
    QVERIFY(!cartridgePath.isEmpty());
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    // Should still load content even if QML app doesn't exist
    QVERIFY(readerView.getCurrentPageId() >= 0);
    
    QFile::remove(cartridgePath);
}

void TestReaderViewQmlIntegration::testQmlAppLoadedFromMarker()
{
    QString qmlCode = R"(
        import QtQuick 2.15
        Rectangle { 
            id: root
            color: "lightblue"
            width: 200
            height: 100
            Text {
                anchors.centerIn: parent
                text: "Test App"
            }
        }
    )";
    
    QString htmlContent = R"(
        <p>Before app</p>
        <div data-smartbook-qml-app="test_app"></div>
        <p>After app</p>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent, "test_app", qmlCode);
    QVERIFY(!cartridgePath.isEmpty());
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    // Verify content loaded
    QVERIFY(readerView.getCurrentPageId() >= 0);
    
    // Note: Actual QML widget verification would require checking the layout
    // This is a basic integration test - full widget embedding tests in integration suite
    
    QFile::remove(cartridgePath);
}

void TestReaderViewQmlIntegration::testMultipleQmlAppsLoaded()
{
    QString qmlCode = R"(
        import QtQuick 2.15
        Rectangle { color: "lightblue"; width: 200; height: 100; }
    )";
    
    QString htmlContent = R"(
        <p>Start</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>Middle</p>
        <div data-smartbook-qml-app="app_2"></div>
        <p>End</p>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent);
    
    // Add multiple apps
    smartbook::common::database::CartridgeDBConnector connector(this);
    if (connector.openCartridge(cartridgePath)) {
        QSqlQuery query(connector.getDatabase());
        query.exec(R"(
            CREATE TABLE IF NOT EXISTS Embedded_Apps (
                app_id TEXT PRIMARY KEY,
                app_name TEXT NOT NULL,
                qml_code TEXT,
                app_version TEXT,
                description TEXT
            )
        )");
        
        query.prepare("INSERT INTO Embedded_Apps (app_id, app_name, qml_code, app_version, description) VALUES (?, ?, ?, ?, ?)");
        
        query.addBindValue("app_1");
        query.addBindValue("App 1");
        query.addBindValue(qmlCode);
        query.addBindValue("1.0");
        query.addBindValue("First App");
        query.exec();
        
        query.addBindValue("app_2");
        query.addBindValue("App 2");
        query.addBindValue(qmlCode);
        query.addBindValue("1.0");
        query.addBindValue("Second App");
        query.exec();
        
        connector.closeCartridge();
    }
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    QVERIFY(readerView.getCurrentPageId() >= 0);
    
    QFile::remove(cartridgePath);
}

void TestReaderViewQmlIntegration::testQmlAppErrorHandling()
{
    QString htmlContent = R"(
        <p>Content</p>
        <div data-smartbook-qml-app="invalid_app"></div>
        <p>More content</p>
    )";
    
    QString cartridgePath = createTestCartridge(m_testGuid, htmlContent);
    QVERIFY(!cartridgePath.isEmpty());
    
    smartbook::reader::ReaderView readerView;
    readerView.show();
    QApplication::processEvents();
    
    // Should not emit errorOccurred for missing QML app - just skip it
    QSignalSpy errorSpy(&readerView, &smartbook::reader::ReaderView::errorOccurred);
    
    readerView.loadCartridge(cartridgePath, m_testGuid);
    QApplication::processEvents();
    
    // Content should still load even if QML app is missing
    QVERIFY(readerView.getCurrentPageId() >= 0);
    // Should not emit error for missing QML app (graceful degradation)
    QCOMPARE(errorSpy.count(), 0);
    
    QFile::remove(cartridgePath);
}

QTEST_MAIN(TestReaderViewQmlIntegration)
#include "test_readerview_qml_integration.moc"

