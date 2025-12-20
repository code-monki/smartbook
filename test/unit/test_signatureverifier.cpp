#include <QtTest>
#include "smartbook/common/security/SignatureVerifier.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>

using namespace smartbook::common::security;
using namespace smartbook::common::database;

class TestSignatureVerifier : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testL1CommercialTrust();  // T-SEC-01: L1 Commercial Trust (FR-2.3.1)

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    
    QString createL1Cartridge(const QString& guid);
    QString createL2Cartridge(const QString& guid);
    QString createL3Cartridge(const QString& guid);
};

void TestSignatureVerifier::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
}

void TestSignatureVerifier::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestSignatureVerifier::createL1Cartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("l1_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "L1Cartridge");
    db.setDatabaseName(path);
    
    if (!db.open()) {
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
    query.addBindValue(guid);
    query.addBindValue("L1 Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    // Create Cartridge_Security table with CA-signed certificate
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            security_id INTEGER PRIMARY KEY,
            hash_digest BLOB,
            certificate_data BLOB,
            signature_data BLOB
        )
    )");
    
    // For L1, we need a CA-signed certificate
    // In a real implementation, this would be a valid CA-signed certificate
    // For testing, we'll create a placeholder that indicates L1 level
    
    // Create minimal content tables so H2 hash can be calculated
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY,
            content_html TEXT
        )
    )");
    query.exec("INSERT INTO Content_Pages (page_id, content_html) VALUES (1, '<p>Test content</p>')");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Themes (
            theme_id TEXT PRIMARY KEY,
            theme_config_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            security_id INTEGER PRIMARY KEY,
            hash_digest BLOB,
            certificate_data BLOB,
            signature_data BLOB
        )
    )");
    
    // Calculate H2 hash from the content we just created
    // For testing, we'll calculate it after creating content
    // But for now, we'll use a placeholder and skip integrity check in test
    QByteArray certData = "CA_SIGNED_CERTIFICATE_PLACEHOLDER";
    // Use a placeholder hash - the test will need to handle this
    QByteArray hashData = QByteArray::fromHex("a1b2c3d4e5f6");
    
    query.prepare("INSERT INTO Cartridge_Security (hash_digest, certificate_data) VALUES (?, ?)");
    query.addBindValue(hashData);
    query.addBindValue(certData);
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase("L1Cartridge");
    
    return path;
}

QString TestSignatureVerifier::createL2Cartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("l2_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "L2Cartridge");
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
    query.addBindValue("L2 Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            security_id INTEGER PRIMARY KEY,
            hash_digest BLOB,
            certificate_data BLOB,
            signature_data BLOB
        )
    )");
    
    // L2 has self-signed certificate
    QByteArray certData = "SELF_SIGNED_CERTIFICATE_PLACEHOLDER";
    QByteArray hashData = QByteArray::fromHex("b2c3d4e5f6a1");
    
    query.prepare("INSERT INTO Cartridge_Security (hash_digest, certificate_data) VALUES (?, ?)");
    query.addBindValue(hashData);
    query.addBindValue(certData);
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase("L2Cartridge");
    
    return path;
}

QString TestSignatureVerifier::createL3Cartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("l3_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "L3Cartridge");
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
    query.addBindValue("L3 Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.exec();
    
    // L3 has no security table (unsigned)
    
    db.close();
    QSqlDatabase::removeDatabase("L3Cartridge");
    
    return path;
}

// T-SEC-01: L1 Commercial Trust
// Requirement: FR-2.3.1
// Test Plan: test-plan.adoc lines 22-30
// AC: The call returns TRUE immediately. No native modal dialog is displayed.
void TestSignatureVerifier::testL1CommercialTrust()
{
    SignatureVerifier verifier(this);
    
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createL1Cartridge(guid);
    QVERIFY(!cartridgePath.isEmpty());
    QVERIFY(QFile::exists(cartridgePath));
    
    // Calculate actual H2 hash from the cartridge we created
    QByteArray h2Hash = verifier.calculateContentHash(cartridgePath);
    QVERIFY(!h2Hash.isEmpty());
    
    // Update the cartridge with the correct H1 hash (should match H2 for non-tampered)
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "UpdateH1");
    db.setDatabaseName(cartridgePath);
    if (db.open()) {
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE Cartridge_Security SET hash_digest = ?");
        updateQuery.addBindValue(h2Hash);
        updateQuery.exec();
        db.close();
    }
    QSqlDatabase::removeDatabase("UpdateH1");
    
    // Verify cartridge - should detect L1 level
    VerificationResult result = verifier.verifyCartridge(cartridgePath, guid);
    
    // For L1 cartridges, the effective policy should be WHITELISTED
    // This means requestAppConsent() would return TRUE immediately
    QCOMPARE(result.securityLevel, SecurityLevel::LEVEL_1);
    QVERIFY(!result.isTampered);
    
    // Verify the policy is WHITELISTED (L1 should bypass consent)
    QCOMPARE(result.effectivePolicy, TrustPolicy::WHITELISTED);
}

QTEST_MAIN(TestSignatureVerifier)
#include "test_signatureverifier.moc"
