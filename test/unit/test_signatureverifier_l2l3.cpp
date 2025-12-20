#include <QtTest>
#include "smartbook/common/security/SignatureVerifier.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>

using namespace smartbook::common::security;
using namespace smartbook::common::database;

class TestSignatureVerifierL2L3 : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testL2InitialConsent();  // T-SEC-02: L2/L3 Initial Consent (FR-2.3.2/2.3.3)
    void testL3InitialConsent();  // T-SEC-02: L3 Initial Consent

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    
    QString createL2Cartridge(const QString& guid);
    QString createL3Cartridge(const QString& guid);
};

void TestSignatureVerifierL2L3::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
}

void TestSignatureVerifierL2L3::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestSignatureVerifierL2L3::createL2Cartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("l2_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "L2CartridgeTest");
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
    
    // L2 has self-signed certificate (no CA_SIGNED marker)
    QByteArray certData = "SELF_SIGNED_CERTIFICATE_PLACEHOLDER";
    QByteArray hashData = QByteArray::fromHex("b2c3d4e5f6a1");
    
    query.prepare("INSERT INTO Cartridge_Security (hash_digest, certificate_data) VALUES (?, ?)");
    query.addBindValue(hashData);
    query.addBindValue(certData);
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase("L2CartridgeTest");
    
    return path;
}

QString TestSignatureVerifierL2L3::createL3Cartridge(const QString& guid)
{
    QString path = m_tempDir->filePath("l3_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "L3CartridgeTest");
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
    QSqlDatabase::removeDatabase("L3CartridgeTest");
    
    return path;
}

// T-SEC-02: Initial Consent (L2/L3)
// Requirement: FR-2.3.2/2.3.3
// Test Plan: test-plan.adoc lines 33-43
// AC: The native security dialog is displayed, clearly showing the App Name and the three consent options
void TestSignatureVerifierL2L3::testL2InitialConsent()
{
    SignatureVerifier verifier(this);
    
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createL2Cartridge(guid);
    QVERIFY(!cartridgePath.isEmpty());
    QVERIFY(QFile::exists(cartridgePath));
    
    // Verify cartridge - should detect L2 level and require consent
    VerificationResult result = verifier.verifyCartridge(cartridgePath, guid);
    
    // For L2 cartridges, the effective policy should be CONSENT_REQUIRED
    QCOMPARE(result.securityLevel, SecurityLevel::LEVEL_2);
    QCOMPARE(result.effectivePolicy, TrustPolicy::CONSENT_REQUIRED);
    QVERIFY(!result.isTampered);
}

void TestSignatureVerifierL2L3::testL3InitialConsent()
{
    SignatureVerifier verifier(this);
    
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createL3Cartridge(guid);
    QVERIFY(!cartridgePath.isEmpty());
    QVERIFY(QFile::exists(cartridgePath));
    
    // Verify cartridge - should detect L3 level and require consent
    VerificationResult result = verifier.verifyCartridge(cartridgePath, guid);
    
    // For L3 cartridges, the effective policy should be CONSENT_REQUIRED
    QCOMPARE(result.securityLevel, SecurityLevel::LEVEL_3);
    QCOMPARE(result.effectivePolicy, TrustPolicy::CONSENT_REQUIRED);
    QVERIFY(!result.isTampered);
}

QTEST_MAIN(TestSignatureVerifierL2L3)
#include "test_signatureverifier_l2l3.moc"
