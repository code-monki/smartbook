#include <QtTest>
#include "smartbook/common/settings/SettingsManager.h"
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
#include <QSqlError>
#include <QApplication>
#include <QDebug>

using namespace smartbook::common::settings;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;
using namespace smartbook::reader;

/**
 * Integration test for settings application
 * 
 * Tests the complete settings application workflow:
 * 1. Load settings from cartridge
 * 2. User override settings
 * 3. Settings priority resolution (User > Author > App default)
 * 4. Settings persistence across sessions
 * 5. Settings isolation per cartridge
 * 6. Settings application in ReaderViewWindow
 * 
 * Test Case: Integration test for settings application
 * Requirements: FR-2.2.3, FR-2.6.1 through FR-2.6.5
 */
class TestSettingsApplication : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Settings application tests
    void testLoadAuthorSettings();
    void testUserOverridePriority();
    void testSettingsPriorityResolution();
    void testSettingsPersistenceAcrossSessions();
    void testSettingsIsolationPerCartridge();
    void testResetToAuthorDefaults();
    void testSettingsWithReaderViewWindow();

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
    TestSettingsApplication tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestSettingsApplication::initTestCase()
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

void TestSettingsApplication::cleanupTestCase()
{
    delete m_manifestManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestSettingsApplication::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_Settings_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    // Add Settings table with author-defined settings
    // Note: createMinimalCartridge may have already created Settings table without setting_type
    QString settingsConn = QString("TestCartridge_SettingsTable_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase settingsDb = QSqlDatabase::addDatabase("QSQLITE", settingsConn);
    settingsDb.setDatabaseName(path);
    if (settingsDb.open()) {
        QSqlQuery query(settingsDb);
        
        // Drop existing Settings table if it exists (may not have setting_type column)
        query.exec("DROP TABLE IF EXISTS Settings");
        
        // Create Settings table with setting_type column
        query.exec(R"(
            CREATE TABLE Settings (
                setting_key TEXT PRIMARY KEY,
                setting_value TEXT NOT NULL,
                setting_type TEXT NOT NULL
            )
        )");
        
        // Add author-defined settings
        query.prepare("INSERT INTO Settings (setting_key, setting_value, setting_type) VALUES (?, ?, ?)");
        query.addBindValue("font_size");
        query.addBindValue("14");
        query.addBindValue("integer");
        if (!query.exec()) {
            qWarning() << "Failed to insert font_size setting:" << query.lastError().text();
        }
        
        query.prepare("INSERT INTO Settings (setting_key, setting_value, setting_type) VALUES (?, ?, ?)");
        query.addBindValue("font_family");
        query.addBindValue("serif");
        query.addBindValue("string");
        if (!query.exec()) {
            qWarning() << "Failed to insert font_family setting:" << query.lastError().text();
        }
        
        query.prepare("INSERT INTO Settings (setting_key, setting_value, setting_type) VALUES (?, ?, ?)");
        query.addBindValue("line_height");
        query.addBindValue("1.5");
        query.addBindValue("float");
        if (!query.exec()) {
            qWarning() << "Failed to insert line_height setting:" << query.lastError().text();
        }
        
        settingsDb.close();
    }
    QSqlDatabase::removeDatabase(settingsConn);
    
    return path;
}

void TestSettingsApplication::createManifestEntry(const QString& guid, const QString& path, const QString& title)
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

// Test: Load author-defined settings from cartridge
void TestSettingsApplication::testLoadAuthorSettings()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Settings Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    SettingsManager manager(this);
    bool loaded = manager.loadSettings(guid, cartridgePath);
    QVERIFY(loaded);
    
    // Verify cartridge has settings (direct database check)
    QString verifyConn = QString("TestConn_Verify_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector verifyConnector(this);
    QVERIFY(verifyConnector.openCartridge(cartridgePath));
    QSqlQuery verifyQuery = verifyConnector.executeQuery("SELECT COUNT(*) FROM Settings");
    if (verifyQuery.next()) {
        int count = verifyQuery.value(0).toInt();
        QVERIFY(count > 0); // Settings exist in cartridge
    }
    verifyConnector.closeCartridge();
    QSqlDatabase::removeDatabase(verifyConn);
    
    // Verify author settings are loaded
    QString fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "14"); // Author default
    
    QString fontFamily = manager.getSetting("font_family", "sans-serif");
    QCOMPARE(fontFamily, "serif"); // Author default
    
    QString lineHeight = manager.getSetting("line_height", "1.2");
    QCOMPARE(lineHeight, "1.5"); // Author default
}

// Test: User override takes priority over author default
void TestSettingsApplication::testUserOverridePriority()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Override Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Create manifest entry (required for foreign key constraint)
    createManifestEntry(guid, cartridgePath, "Override Test Book");
    
    SettingsManager manager(this);
    bool loaded = manager.loadSettings(guid, cartridgePath);
    QVERIFY(loaded);
    
    // Verify author default
    QString fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "14");
    
    // Verify database is open
    QVERIFY(m_dbManager->isOpen());
    
    // Set user override
    bool overridden = manager.setUserOverride("font_size", "18");
    if (!overridden) {
        qWarning() << "setUserOverride failed - checking database state";
        QVERIFY(m_dbManager->isOpen());
    }
    QVERIFY(overridden);
    
    // Verify user override takes priority
    fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "18"); // User override
}

// Test: Settings priority resolution (User > Author > App default)
void TestSettingsApplication::testSettingsPriorityResolution()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Priority Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Create manifest entry (required for foreign key constraint)
    createManifestEntry(guid, cartridgePath, "Priority Test Book");
    
    SettingsManager manager(this);
    manager.loadSettings(guid, cartridgePath);
    
    // Test 1: Author default (no user override, no app default)
    QString fontFamily = manager.getSetting("font_family");
    QCOMPARE(fontFamily, "serif"); // Author default
    
    // Test 2: App default (no author setting, no user override)
    QString unknownSetting = manager.getSetting("unknown_setting", "app_default");
    QCOMPARE(unknownSetting, "app_default"); // App default
    
    // Test 3: User override (takes priority over author default)
    manager.setUserOverride("font_family", "monospace");
    fontFamily = manager.getSetting("font_family", "sans-serif");
    QCOMPARE(fontFamily, "monospace"); // User override
    
    // Test 4: User override (takes priority over app default)
    manager.setUserOverride("unknown_setting", "user_override");
    unknownSetting = manager.getSetting("unknown_setting", "app_default");
    QCOMPARE(unknownSetting, "user_override"); // User override
}

// Test: Settings persist across sessions
void TestSettingsApplication::testSettingsPersistenceAcrossSessions()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Persistence Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Persistence Test Book");
    
    // Session 1: Set user override
    {
        SettingsManager manager1(this);
        manager1.loadSettings(guid, cartridgePath);
        bool overridden = manager1.setUserOverride("font_size", "20");
        QVERIFY(overridden);
    }
    
    // Session 2: Load settings (simulating cartridge reopen)
    {
        SettingsManager manager2(this);
        manager2.loadSettings(guid, cartridgePath);
        
        // Verify user override persisted
        QString fontSize = manager2.getSetting("font_size", "12");
        QCOMPARE(fontSize, "20"); // User override from previous session
    }
}

// Test: Settings are isolated per cartridge
void TestSettingsApplication::testSettingsIsolationPerCartridge()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    // Create manifest entries (required for foreign key constraint)
    createManifestEntry(guid1, path1, "Book 1");
    createManifestEntry(guid2, path2, "Book 2");
    
    // Set different overrides for each cartridge
    SettingsManager manager1(this);
    manager1.loadSettings(guid1, path1);
    bool overridden1 = manager1.setUserOverride("font_size", "16");
    QVERIFY(overridden1);
    
    SettingsManager manager2(this);
    manager2.loadSettings(guid2, path2);
    bool overridden2 = manager2.setUserOverride("font_size", "24");
    QVERIFY(overridden2);
    
    // Verify settings are isolated
    QString fontSize1 = manager1.getSetting("font_size", "12");
    QString fontSize2 = manager2.getSetting("font_size", "12");
    
    QCOMPARE(fontSize1, "16");
    QCOMPARE(fontSize2, "24");
    QVERIFY(fontSize1 != fontSize2);
}

// Test: Reset to author defaults
void TestSettingsApplication::testResetToAuthorDefaults()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Reset Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Create manifest entry (required for foreign key constraint)
    createManifestEntry(guid, cartridgePath, "Reset Test Book");
    
    SettingsManager manager(this);
    manager.loadSettings(guid, cartridgePath);
    
    // Set user override
    bool overridden = manager.setUserOverride("font_size", "20");
    QVERIFY(overridden);
    QString fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "20");
    
    // Reset to author defaults
    bool reset = manager.resetToAuthorDefaults();
    QVERIFY(reset);
    
    // Verify back to author default
    fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "14"); // Author default
}

// Test: Settings work with ReaderViewWindow integration
void TestSettingsApplication::testSettingsWithReaderViewWindow()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "ReaderViewWindow Settings Test");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "ReaderViewWindow Settings Test");
    
    // Create ReaderViewWindow (which uses SettingsManager internally)
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    // Note: ReaderViewWindow loads settings internally through ReaderView
    // For this integration test, we verify settings can be loaded and applied
    SettingsManager manager(this);
    bool loaded = manager.loadSettings(guid, cartridgePath);
    QVERIFY(loaded);
    
    // Verify settings are accessible
    QString fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "14");
    
    // Set user override
    bool overridden = manager.setUserOverride("font_size", "18");
    QVERIFY(overridden);
    
    // Verify override is applied
    fontSize = manager.getSetting("font_size", "12");
    QCOMPARE(fontSize, "18");
    
    delete window;
}

#include "test_settings_application.moc"

