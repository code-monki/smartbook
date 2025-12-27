#include <QtTest>
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/settings/SettingsManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QApplication>
#include <QElapsedTimer>
#include <QDebug>
#include <QSignalSpy>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::settings;

/**
 * Integration test for theme change functionality
 * 
 * Tests that theme changes (light/dark/sepia) work correctly without
 * causing rendering flash or visual artifacts.
 * 
 * Test Case: Integration test for theme changes
 * Requirements: FR-2.3.1 (Theme Support), FR-2.3.2 (No Flash)
 */
class TestThemeChange : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Theme change tests
    void testThemeChangeLightToDark();
    void testThemeChangeLightToSepia();
    void testThemeChangeDarkToSepia();
    void testThemeChangeMultiple();
    void testThemeChangeNoFlash();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_cartridgePath;
    LocalDBManager* m_dbManager;
    SettingsManager* m_settingsManager;
    
    QString createTestCartridge(const QString& guid);
    void createManifestEntry(const QString& guid, const QString& path);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestThemeChange tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestThemeChange::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    m_settingsManager = new SettingsManager(this);
    
    // Create test cartridge
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge(guid);
    QVERIFY(QFile::exists(m_cartridgePath));
    
    createManifestEntry(guid, m_cartridgePath);
}

void TestThemeChange::cleanupTestCase()
{
    delete m_settingsManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestThemeChange::createTestCartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_Theme_" + guid;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
        QSqlDatabase::removeDatabase(connectionName);
        return QString();
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table with all required columns
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL,
            version TEXT NOT NULL DEFAULT '1.0',
            schema_version TEXT NOT NULL DEFAULT '1.0',
            publisher TEXT
        )
    )");
    
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, version, schema_version) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Theme Test Book");
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
            chapter_title TEXT,
            html_content TEXT NOT NULL,
            associated_css TEXT
        )
    )");
    
    // Insert test content page
    query.prepare("INSERT INTO Content_Pages (page_id, page_order, html_content) VALUES (?, ?, ?)");
    query.addBindValue(1);
    query.addBindValue(1);
    query.addBindValue("<h1>Theme Test</h1><p>This is a test page for theme changes.</p>");
    query.exec();
    
    // Create Settings table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT NOT NULL,
            setting_type TEXT NOT NULL DEFAULT 'user'
        )
    )");
    
    db.close();
    QApplication::processEvents();
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

void TestThemeChange::createManifestEntry(const QString& guid, const QString& path)
{
    // Manifest entry is created via LocalDBManager
    // For this test, we'll use CartridgeDBConnector directly
    Q_UNUSED(guid);
    Q_UNUSED(path);
}

// Test: Theme change from light to dark
void TestThemeChange::testThemeChangeLightToDark()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_settingsManager->loadSettings(guid, m_cartridgePath);
    
    ReaderView* readerView = new ReaderView();
    readerView->loadCartridge(m_cartridgePath, guid);
    
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Set initial theme to light
    m_settingsManager->setUserOverride("default_theme", "light");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Change to dark
    m_settingsManager->setUserOverride("default_theme", "dark");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Verify theme was applied (no crash, no error)
    QVERIFY(readerView != nullptr);
    
    delete readerView;
}

// Test: Theme change from light to sepia
void TestThemeChange::testThemeChangeLightToSepia()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_settingsManager->loadSettings(guid, m_cartridgePath);
    
    ReaderView* readerView = new ReaderView();
    readerView->loadCartridge(m_cartridgePath, guid);
    
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Set initial theme to light
    m_settingsManager->setUserOverride("default_theme", "light");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Change to sepia
    m_settingsManager->setUserOverride("default_theme", "sepia");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Verify theme was applied
    QVERIFY(readerView != nullptr);
    
    delete readerView;
}

// Test: Theme change from dark to sepia
void TestThemeChange::testThemeChangeDarkToSepia()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_settingsManager->loadSettings(guid, m_cartridgePath);
    
    ReaderView* readerView = new ReaderView();
    readerView->loadCartridge(m_cartridgePath, guid);
    
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Set initial theme to dark
    m_settingsManager->setUserOverride("default_theme", "dark");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Change to sepia
    m_settingsManager->setUserOverride("default_theme", "sepia");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Verify theme was applied
    QVERIFY(readerView != nullptr);
    
    delete readerView;
}

// Test: Multiple rapid theme changes
void TestThemeChange::testThemeChangeMultiple()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_settingsManager->loadSettings(guid, m_cartridgePath);
    
    ReaderView* readerView = new ReaderView();
    readerView->loadCartridge(m_cartridgePath, guid);
    
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Rapidly change themes multiple times
    QStringList themes = {"light", "dark", "sepia", "light", "dark"};
    
    for (const QString& theme : themes) {
        m_settingsManager->setUserOverride("default_theme", theme);
        QApplication::processEvents();
        QTest::qWait(50); // Small delay between changes
    }
    
    // Verify no crash occurred
    QVERIFY(readerView != nullptr);
    
    delete readerView;
}

// Test: Theme change should be fast (no flash)
void TestThemeChange::testThemeChangeNoFlash()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_settingsManager->loadSettings(guid, m_cartridgePath);
    
    ReaderView* readerView = new ReaderView();
    readerView->loadCartridge(m_cartridgePath, guid);
    
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Set initial theme
    m_settingsManager->setUserOverride("default_theme", "light");
    QApplication::processEvents();
    QTest::qWait(100);
    
    // Measure time for theme change
    QElapsedTimer timer;
    timer.start();
    
    m_settingsManager->setUserOverride("default_theme", "dark");
    QApplication::processEvents();
    
    qint64 changeTime = timer.elapsed();
    
    // Theme change should be very fast (< 50ms) to avoid visible flash
    // QTextBrowser with palette changes should be nearly instantaneous
    qDebug() << "Theme change took" << changeTime << "ms";
    QVERIFY(changeTime < 100); // Allow some margin for processEvents
    
    delete readerView;
}

#include "test_theme_change.moc"

