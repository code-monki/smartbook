#include <QtTest>
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>

using namespace smartbook::common::database;

class TestCartridgeDBConnector : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testMultiWindowIsolation();  // T-PERS-02: Multi-Window Isolation (FR-2.1.1)
    void testFormDataPersistence();   // T-PERS-02: Form data isolation

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    QString m_cartridgeAPath;
    QString m_cartridgeBPath;
    
    QString createTestCartridge(const QString& name, const QString& guid);
};

void TestCartridgeDBConnector::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    // Initialize local database
    LocalDBManager& dbManager = LocalDBManager::getInstance();
    bool initialized = dbManager.initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(dbManager.isOpen());
    
    // Create test cartridge files
    QString guidA = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guidB = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    m_cartridgeAPath = createTestCartridge("CartridgeA", guidA);
    m_cartridgeBPath = createTestCartridge("CartridgeB", guidB);
    
    QVERIFY(QFile::exists(m_cartridgeAPath));
    QVERIFY(QFile::exists(m_cartridgeBPath));
}

void TestCartridgeDBConnector::cleanupTestCase()
{
    LocalDBManager& dbManager = LocalDBManager::getInstance();
    if (dbManager.isOpen()) {
        dbManager.closeConnection();
    }
    delete m_tempDir;
}

QString TestCartridgeDBConnector::createTestCartridge(const QString& name, const QString& guid)
{
    QString path = m_tempDir->filePath(name + ".sqlite");
    
    // Create a minimal SQLite database with required tables
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "TestCartridge_" + name);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
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
    
    // Insert metadata
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year) VALUES (?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(name);
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    // Create User_Data table (for form data isolation test)
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS User_Data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            form_id TEXT NOT NULL,
            data_json TEXT NOT NULL,
            saved_timestamp INTEGER NOT NULL
        )
    )");
    
    db.close();
    QSqlDatabase::removeDatabase("TestCartridge_" + name);
    
    return path;
}

// T-PERS-02: Multi-Window Isolation
// Requirement: FR-2.1.1
// Test Plan: test-plan.adoc lines 102-110
// AC: Both windows remain responsive. Data saved in B is isolated to B's User_Data table,
//     confirming the per-instance nature of CartridgeDBConnector
void TestCartridgeDBConnector::testMultiWindowIsolation()
{
    // Create two separate CartridgeDBConnector instances (simulating two windows)
    CartridgeDBConnector connectorA(this);
    CartridgeDBConnector connectorB(this);
    
    // Open different cartridges in each connector
    bool openedA = connectorA.openCartridge(m_cartridgeAPath);
    bool openedB = connectorB.openCartridge(m_cartridgeBPath);
    
    QVERIFY(openedA);
    QVERIFY(openedB);
    
    // Save form data to Cartridge A
    QString formId = "FormX";
    QString dataA = R"({"field1": "valueA", "field2": "dataA"})";
    bool savedA = connectorA.saveFormData(formId, dataA);
    QVERIFY(savedA);
    
    // Save different form data to Cartridge B (same form ID, different data)
    QString dataB = R"({"field1": "valueB", "field2": "dataB"})";
    bool savedB = connectorB.saveFormData(formId, dataB);
    QVERIFY(savedB);
    
    // Verify isolation: Load data from A should return dataA, not dataB
    QString loadedA = connectorA.loadFormData(formId);
    QCOMPARE(loadedA, dataA);
    
    // Verify isolation: Load data from B should return dataB, not dataA
    QString loadedB = connectorB.loadFormData(formId);
    QCOMPARE(loadedB, dataB);
    
    // Both connectors should remain functional
    QVERIFY(connectorA.isOpen());
    QVERIFY(connectorB.isOpen());
}

void TestCartridgeDBConnector::testFormDataPersistence()
{
    CartridgeDBConnector connector(this);
    bool opened = connector.openCartridge(m_cartridgeAPath);
    QVERIFY(opened);
    
    // Save form data
    QString formId = "TestForm";
    QString testData = R"({"test": "data", "value": 42})";
    bool saved = connector.saveFormData(formId, testData);
    QVERIFY(saved);
    
    // Close and reopen
    connector.closeCartridge();
    opened = connector.openCartridge(m_cartridgeAPath);
    QVERIFY(opened);
    
    // Data should persist
    QString loaded = connector.loadFormData(formId);
    QCOMPARE(loaded, testData);
}

QTEST_MAIN(TestCartridgeDBConnector)
#include "test_cartridgedbconnector.moc"
