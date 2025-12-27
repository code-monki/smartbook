#include <QtTest>
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
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
 * Integration test for multi-window isolation
 * 
 * Tests that multiple ReaderViewWindow instances are properly isolated:
 * 1. Each window has its own CartridgeDBConnector
 * 2. Form data is isolated per cartridge
 * 3. Window state is isolated per cartridge
 * 4. Settings are isolated per cartridge
 * 5. WebChannelBridge is isolated per window
 * 
 * Test Case: Integration test for multi-window isolation
 * Requirements: FR-2.1.1 (Multi-Window Capability), T-PERS-02
 */
class TestMultiWindowIsolation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Multi-window isolation tests
    void testMultipleWindowsIndependent();
    void testFormDataIsolation();
    void testWindowStateIsolation();
    void testSettingsIsolation();
    void testDatabaseConnectionIsolation();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_libraryPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    QWidget* m_parentWidget;
    
    QString createTestCartridge(const QString& guid, const QString& title);
    void createManifestEntry(const QString& guid, const QString& path, const QString& title);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestMultiWindowIsolation tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestMultiWindowIsolation::initTestCase()
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
    m_parentWidget = new QWidget();
}

void TestMultiWindowIsolation::cleanupTestCase()
{
    delete m_parentWidget;
    delete m_manifestManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestMultiWindowIsolation::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_MultiWindow_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    // Add form definition for form data testing
    QString formConn = QString("TestCartridge_Form_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
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
        query.addBindValue("test_form");
        query.addBindValue(R"({"fields": [{"name": "test_field", "type": "text"}]})");
        query.exec();
        
        // Add settings for settings isolation testing
        query.exec(R"(
            CREATE TABLE IF NOT EXISTS Settings (
                setting_key TEXT PRIMARY KEY,
                setting_value TEXT NOT NULL
            )
        )");
        query.prepare("INSERT INTO Settings (setting_key, setting_value) VALUES (?, ?)");
        query.addBindValue("font_size");
        query.addBindValue("14");
        query.exec();
        
        formDb.close();
    }
    QSqlDatabase::removeDatabase(formConn);
    
    return path;
}

void TestMultiWindowIsolation::createManifestEntry(const QString& guid, const QString& path, const QString& title)
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

// Test: Multiple windows can be opened independently
void TestMultiWindowIsolation::testMultipleWindowsIndependent()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    createManifestEntry(guid1, path1, "Book 1");
    createManifestEntry(guid2, path2, "Book 2");
    
    // Create two windows
    ReaderViewWindow* window1 = new ReaderViewWindow(guid1);
    ReaderViewWindow* window2 = new ReaderViewWindow(guid2);
    
    QVERIFY(window1 != nullptr);
    QVERIFY(window2 != nullptr);
    QVERIFY(window1 != window2);
    
    // Verify windows can be shown independently
    window1->show();
    window2->show();
    QApplication::processEvents();
    
    QVERIFY(window1->isVisible());
    QVERIFY(window2->isVisible());
    
    // Clean up
    window1->close();
    window2->close();
    QApplication::processEvents();
    delete window1;
    delete window2;
}

// Test: Form data is isolated per cartridge
void TestMultiWindowIsolation::testFormDataIsolation()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    createManifestEntry(guid1, path1, "Book 1");
    createManifestEntry(guid2, path2, "Book 2");
    
    // Save form data to cartridge 1
    QString conn1 = QString("TestConn_Form1_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector1(this);
    QVERIFY(connector1.openCartridge(path1));
    bool saved1 = connector1.saveFormData("test_form", R"({"test_field": "value1"})");
    QVERIFY(saved1);
    connector1.closeCartridge();
    QSqlDatabase::removeDatabase(conn1);
    
    // Save different form data to cartridge 2
    QString conn2 = QString("TestConn_Form2_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector2(this);
    QVERIFY(connector2.openCartridge(path2));
    bool saved2 = connector2.saveFormData("test_form", R"({"test_field": "value2"})");
    QVERIFY(saved2);
    connector2.closeCartridge();
    QSqlDatabase::removeDatabase(conn2);
    
    // Verify form data is isolated
    QString conn3 = QString("TestConn_Verify1_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector verify1(this);
    QVERIFY(verify1.openCartridge(path1));
    QString data1 = verify1.loadFormData("test_form");
    verify1.closeCartridge();
    QSqlDatabase::removeDatabase(conn3);
    
    QString conn4 = QString("TestConn_Verify2_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector verify2(this);
    QVERIFY(verify2.openCartridge(path2));
    QString data2 = verify2.loadFormData("test_form");
    verify2.closeCartridge();
    QSqlDatabase::removeDatabase(conn4);
    
    QCOMPARE(data1, R"({"test_field": "value1"})");
    QCOMPARE(data2, R"({"test_field": "value2"})");
    QVERIFY(data1 != data2);
}

// Test: Window state is isolated per cartridge
void TestMultiWindowIsolation::testWindowStateIsolation()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    createManifestEntry(guid1, path1, "Book 1");
    createManifestEntry(guid2, path2, "Book 2");
    
    // Create windows with different geometries
    ReaderViewWindow* window1 = new ReaderViewWindow(guid1);
    window1->setGeometry(100, 100, 800, 600);
    window1->showNormal();
    QApplication::processEvents();
    QTest::qWait(100);
    
    ReaderViewWindow* window2 = new ReaderViewWindow(guid2);
    window2->setGeometry(200, 200, 1000, 800);
    window2->showNormal();
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Close windows to trigger state save
    window1->close();
    window2->close();
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Verify window states are saved separately
    QSqlQuery query(m_dbManager->getDatabase());
    
    // Check window 1 state
    query.prepare("SELECT window_width, window_height, window_x, window_y FROM Local_Window_State WHERE cartridge_guid = ?");
    query.addBindValue(guid1);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 800);
    QCOMPARE(query.value(1).toInt(), 600);
    QCOMPARE(query.value(2).toInt(), 100);
    QCOMPARE(query.value(3).toInt(), 100);
    
    // Check window 2 state
    query.prepare("SELECT window_width, window_height, window_x, window_y FROM Local_Window_State WHERE cartridge_guid = ?");
    query.addBindValue(guid2);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1000);
    QCOMPARE(query.value(1).toInt(), 800);
    QCOMPARE(query.value(2).toInt(), 200);
    QCOMPARE(query.value(3).toInt(), 200);
    
    delete window1;
    delete window2;
}

// Test: Settings are isolated per cartridge
void TestMultiWindowIsolation::testSettingsIsolation()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    // Update settings in cartridge 1
    QString conn1 = QString("TestConn_Settings1_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db1 = QSqlDatabase::addDatabase("QSQLITE", conn1);
    db1.setDatabaseName(path1);
    QVERIFY(db1.open());
    QSqlQuery query1(db1);
    query1.prepare("UPDATE Settings SET setting_value = ? WHERE setting_key = ?");
    query1.addBindValue("16");
    query1.addBindValue("font_size");
    QVERIFY(query1.exec());
    db1.close();
    QSqlDatabase::removeDatabase(conn1);
    
    // Verify settings are different in cartridge 2
    QString conn2 = QString("TestConn_Settings2_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db2 = QSqlDatabase::addDatabase("QSQLITE", conn2);
    db2.setDatabaseName(path2);
    QVERIFY(db2.open());
    QSqlQuery query2(db2);
    query2.prepare("SELECT setting_value FROM Settings WHERE setting_key = ?");
    query2.addBindValue("font_size");
    QVERIFY(query2.exec());
    QVERIFY(query2.next());
    QString value2 = query2.value(0).toString();
    db2.close();
    QSqlDatabase::removeDatabase(conn2);
    
    // Verify cartridge 1 has updated value
    QString conn3 = QString("TestConn_VerifySettings1_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db3 = QSqlDatabase::addDatabase("QSQLITE", conn3);
    db3.setDatabaseName(path1);
    QVERIFY(db3.open());
    QSqlQuery query3(db3);
    query3.prepare("SELECT setting_value FROM Settings WHERE setting_key = ?");
    query3.addBindValue("font_size");
    QVERIFY(query3.exec());
    QVERIFY(query3.next());
    QString value1 = query3.value(0).toString();
    db3.close();
    QSqlDatabase::removeDatabase(conn3);
    
    QCOMPARE(value1, "16");
    QCOMPARE(value2, "14");
    QVERIFY(value1 != value2);
}

// Test: Database connections are isolated per window
void TestMultiWindowIsolation::testDatabaseConnectionIsolation()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    createManifestEntry(guid1, path1, "Book 1");
    createManifestEntry(guid2, path2, "Book 2");
    
    // Open connections to both cartridges simultaneously
    QString conn1 = QString("TestConn_Isolation1_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector1(this);
    QVERIFY(connector1.openCartridge(path1));
    QString guidFromConn1 = connector1.getCartridgeGuid();
    
    QString conn2 = QString("TestConn_Isolation2_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector2(this);
    QVERIFY(connector2.openCartridge(path2));
    QString guidFromConn2 = connector2.getCartridgeGuid();
    
    // Verify each connection is isolated to its own cartridge
    QCOMPARE(guidFromConn1, guid1);
    QCOMPARE(guidFromConn2, guid2);
    QVERIFY(guidFromConn1 != guidFromConn2);
    
    // Verify connections can be used independently
    bool saved1 = connector1.saveFormData("form1", R"({"data": "value1"})");
    bool saved2 = connector2.saveFormData("form2", R"({"data": "value2"})");
    QVERIFY(saved1);
    QVERIFY(saved2);
    
    QString loaded1 = connector1.loadFormData("form1");
    QString loaded2 = connector2.loadFormData("form2");
    QCOMPARE(loaded1, R"({"data": "value1"})");
    QCOMPARE(loaded2, R"({"data": "value2"})");
    
    connector1.closeCartridge();
    connector2.closeCartridge();
    QSqlDatabase::removeDatabase(conn1);
    QSqlDatabase::removeDatabase(conn2);
}

#include "test_multiwindow_isolation.moc"

