#include <QtTest>
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>

using namespace smartbook::common::database;

class TestCartridgeDBConnectorErrors : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testOpenNonExistentCartridge();
    void testOpenInvalidCartridge();
    void testFormDataOnClosedConnector();
    void testMultipleOpenClose();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    
    QString createValidCartridge(const QString& guid);
};

void TestCartridgeDBConnectorErrors::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
}

void TestCartridgeDBConnectorErrors::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestCartridgeDBConnectorErrors::createValidCartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("valid_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ValidCartridge");
    db.setDatabaseName(path);
    
    if (!db.open()) {
        return QString();
    }
    
    QSqlQuery query(db);
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL
        )
    )");
    
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year) VALUES (?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase("ValidCartridge");
    
    return path;
}

void TestCartridgeDBConnectorErrors::testOpenNonExistentCartridge()
{
    CartridgeDBConnector connector(this);
    
    QString nonExistentPath = "/nonexistent/path/cartridge.sqlite";
    bool opened = connector.openCartridge(nonExistentPath);
    QVERIFY(!opened);
    QVERIFY(!connector.isOpen());
}

void TestCartridgeDBConnectorErrors::testOpenInvalidCartridge()
{
    CartridgeDBConnector connector(this);
    
    // Create an invalid SQLite file (just text)
    QString invalidPath = m_tempDir->filePath("invalid.sqlite");
    QFile invalidFile(invalidPath);
    if (invalidFile.open(QIODevice::WriteOnly)) {
        invalidFile.write("This is not a valid SQLite database");
        invalidFile.close();
    }
    
    bool opened = connector.openCartridge(invalidPath);
    // May or may not open depending on SQLite's tolerance, but should handle gracefully
    // If it opens, operations should fail
    if (opened) {
        QVERIFY(connector.isOpen());
        // Try to get GUID - should be empty for invalid cartridge
        QString guid = connector.getCartridgeGuid();
        QVERIFY(guid.isEmpty());
    } else {
        QVERIFY(!connector.isOpen());
    }
}

void TestCartridgeDBConnectorErrors::testFormDataOnClosedConnector()
{
    CartridgeDBConnector connector(this);
    
    // Try to save form data without opening cartridge
    QString formId = "TestForm";
    QString data = R"({"test": "data"})";
    
    bool saved = connector.saveFormData(formId, data);
    QVERIFY(!saved);
    
    // Try to load form data without opening cartridge
    QString loaded = connector.loadFormData(formId);
    QVERIFY(loaded.isEmpty());
}

void TestCartridgeDBConnectorErrors::testMultipleOpenClose()
{
    CartridgeDBConnector connector(this);
    
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString path1 = createValidCartridge(guid1);
    QVERIFY(!path1.isEmpty());
    
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString path2 = createValidCartridge(guid2);
    QVERIFY(!path2.isEmpty());
    
    // Open first cartridge
    bool opened1 = connector.openCartridge(path1);
    QVERIFY(opened1);
    QVERIFY(connector.isOpen());
    QCOMPARE(connector.getCartridgeGuid(), guid1);
    
    // Save data to first cartridge
    QString formId = "TestForm";
    QString data1 = R"({"cartridge": "1"})";
    bool saved1 = connector.saveFormData(formId, data1);
    QVERIFY(saved1);
    
    // Close and open second cartridge
    connector.closeCartridge();
    QVERIFY(!connector.isOpen());
    
    bool opened2 = connector.openCartridge(path2);
    QVERIFY(opened2);
    QVERIFY(connector.isOpen());
    QCOMPARE(connector.getCartridgeGuid(), guid2);
    
    // Save different data to second cartridge
    QString data2 = R"({"cartridge": "2"})";
    bool saved2 = connector.saveFormData(formId, data2);
    QVERIFY(saved2);
    
    // Verify isolation - second cartridge should have its own data
    QString loaded2 = connector.loadFormData(formId);
    QCOMPARE(loaded2, data2);
    
    // Reopen first cartridge and verify its data is preserved
    connector.closeCartridge();
    bool reopened1 = connector.openCartridge(path1);
    QVERIFY(reopened1);
    QString loaded1 = connector.loadFormData(formId);
    QCOMPARE(loaded1, data1); // Should still have original data
}

QTEST_MAIN(TestCartridgeDBConnectorErrors)
#include "test_cartridgedbconnector_errors.moc"
