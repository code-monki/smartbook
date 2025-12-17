#include <QtTest>
#include "smartbook/common/database/LocalDBManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QTemporaryDir>

using namespace smartbook::common::database;

class TestLocalDBManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingleton();
    void testInitializeConnection();
    void testSchemaCreation();
    void testQueryExecution();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
};

void TestLocalDBManager::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
}

void TestLocalDBManager::cleanupTestCase()
{
    delete m_tempDir;
}

void TestLocalDBManager::testSingleton()
{
    LocalDBManager& instance1 = LocalDBManager::getInstance();
    LocalDBManager& instance2 = LocalDBManager::getInstance();
    
    QCOMPARE(&instance1, &instance2);
}

void TestLocalDBManager::testInitializeConnection()
{
    LocalDBManager& dbManager = LocalDBManager::getInstance();
    
    bool result = dbManager.initializeConnection(m_testDbPath);
    QVERIFY(result);
    QVERIFY(dbManager.isOpen());
}

void TestLocalDBManager::testSchemaCreation()
{
    LocalDBManager& dbManager = LocalDBManager::getInstance();
    
    if (!dbManager.isOpen()) {
        dbManager.initializeConnection(m_testDbPath);
    }
    
    // Verify tables exist
    QSqlQuery query = dbManager.executeQuery(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='Local_Library_Manifest'"
    );
    QVERIFY(query.next());
    
    query = dbManager.executeQuery(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='Local_Trust_Registry'"
    );
    QVERIFY(query.next());
}

void TestLocalDBManager::testQueryExecution()
{
    LocalDBManager& dbManager = LocalDBManager::getInstance();
    
    if (!dbManager.isOpen()) {
        dbManager.initializeConnection(m_testDbPath);
    }
    
    QSqlQuery query = dbManager.executeQuery("SELECT 1 as test");
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

QTEST_MAIN(TestLocalDBManager)
#include "test_localdbmanager.moc"
