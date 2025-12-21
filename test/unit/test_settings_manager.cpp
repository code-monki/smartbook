#include <QtTest>
#include "smartbook/common/settings/SettingsManager.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QApplication>

using namespace smartbook::common::settings;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestSettingsManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSettingsPriority();  // Test User > Author > App default priority
    void testUserOverride();
    void testResetToAuthorDefaults();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    QString createTestCartridge();
};

void TestSettingsManager::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    m_manifestManager = new ManifestManager(this);
    
    // Create test cartridge with Settings table
    m_cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge();
    QVERIFY(QFile::exists(m_cartridgePath));
    
    // Create manifest entry
    ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = m_cartridgeGuid;
    entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
    entry.localPath = m_cartridgePath;
    entry.title = "Test Book";
    entry.author = "Test Author";
    entry.publicationYear = "2025";
    entry.version = "1.0";
    m_manifestManager->createManifestEntry(entry);
}

void TestSettingsManager::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestSettingsManager::createTestCartridge()
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_Settings_" + m_cartridgeGuid;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
        QSqlDatabase::removeDatabase(connectionName);
        return QString();
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL
        )
    )");
    
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year) VALUES (?, ?, ?, ?)");
    query.addBindValue(m_cartridgeGuid);
    query.addBindValue("Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    // Create Settings table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT NOT NULL,
            setting_type TEXT NOT NULL,
            description TEXT
        )
    )");
    
    // Insert author-defined settings
    query.prepare(R"(
        INSERT INTO Settings (setting_key, setting_value, setting_type, description)
        VALUES (?, ?, ?, ?)
    )");
    
    query.addBindValue("default_font_size");
    query.addBindValue("14");
    query.addBindValue("integer");
    query.addBindValue("Default font size in points");
    query.exec();
    
    query.addBindValue("default_font_family");
    query.addBindValue("Georgia");
    query.addBindValue("string");
    query.addBindValue("Default font family");
    query.exec();
    
    query.addBindValue("line_spacing");
    query.addBindValue("1.5");
    query.addBindValue("float");
    query.addBindValue("Line spacing multiplier");
    query.exec();
    
    db.close();
    
    // Remove database connection after closing
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

// Test settings priority: User override > Author default > App default
void TestSettingsManager::testSettingsPriority()
{
    SettingsManager manager(this);
    bool loaded = manager.loadSettings(m_cartridgeGuid, m_cartridgePath);
    QVERIFY(loaded);
    
    // Test 1: Author default (no user override)
    QString fontSize = manager.getSetting("default_font_size", "12");
    QCOMPARE(fontSize, "14"); // Author default, not app default
    
    QString fontFamily = manager.getSetting("default_font_family", "serif");
    QCOMPARE(fontFamily, "Georgia"); // Author default
    
    // Test 2: App default (no author setting, no user override)
    QString pageWidth = manager.getSetting("page_width", "800px");
    QCOMPARE(pageWidth, "800px"); // App default
    
    // Test 3: User override (after setting one)
    manager.setUserOverride("default_font_size", "16");
    fontSize = manager.getSetting("default_font_size", "12");
    QCOMPARE(fontSize, "16"); // User override, not author default
}

void TestSettingsManager::testUserOverride()
{
    SettingsManager manager(this);
    bool loaded = manager.loadSettings(m_cartridgeGuid, m_cartridgePath);
    QVERIFY(loaded);
    
    // Set user override
    bool saved = manager.setUserOverride("default_font_size", "18");
    QVERIFY(saved);
    
    // Verify override is applied
    QString fontSize = manager.getSetting("default_font_size", "12");
    QCOMPARE(fontSize, "18");
    
    // Verify override persists after reload
    SettingsManager manager2(this);
    loaded = manager2.loadSettings(m_cartridgeGuid, m_cartridgePath);
    QVERIFY(loaded);
    
    fontSize = manager2.getSetting("default_font_size", "12");
    QCOMPARE(fontSize, "18"); // User override persists
}

void TestSettingsManager::testResetToAuthorDefaults()
{
    SettingsManager manager(this);
    bool loaded = manager.loadSettings(m_cartridgeGuid, m_cartridgePath);
    QVERIFY(loaded);
    
    // Set user override
    manager.setUserOverride("default_font_size", "20");
    QString fontSize = manager.getSetting("default_font_size", "12");
    QCOMPARE(fontSize, "20");
    
    // Reset to author defaults
    bool reset = manager.resetToAuthorDefaults();
    QVERIFY(reset);
    
    // Verify author default is restored
    fontSize = manager.getSetting("default_font_size", "12");
    QCOMPARE(fontSize, "14"); // Author default restored
}

// Use custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestSettingsManager tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

#include "test_settings_manager.moc"
