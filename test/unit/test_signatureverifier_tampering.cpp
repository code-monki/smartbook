#include <QtTest>
#include "smartbook/common/security/SignatureVerifier.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>

using namespace smartbook::common::security;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestSignatureVerifierTampering : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testTamperingDetection();  // T-SEC-04: Tampering Detection (FR-2.4.4)

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    QString createTestCartridge(const QString& guid, const QByteArray& h1Hash);
};

void TestSignatureVerifierTampering::initTestCase()
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

void TestSignatureVerifierTampering::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestSignatureVerifierTampering::createTestCartridge(const QString& guid, const QByteArray& h1Hash)
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "TamperTest");
    db.setDatabaseName(path);
    
    if (!db.open()) {
        return QString();
    }
    
    QSqlQuery query(db);
    
    // Create Metadata
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
    
    // Create content tables
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY,
            content_html TEXT
        )
    )");
    query.exec("INSERT INTO Content_Pages (page_id, content_html) VALUES (1, '<p>Original content</p>')");
    
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
    
    // Create security table with provided H1 hash
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            security_id INTEGER PRIMARY KEY,
            hash_digest BLOB,
            certificate_data BLOB,
            signature_data BLOB
        )
    )");
    
    QByteArray certData = "SELF_SIGNED_CERTIFICATE_PLACEHOLDER";
    
    query.prepare("INSERT INTO Cartridge_Security (hash_digest, certificate_data) VALUES (?, ?)");
    query.addBindValue(h1Hash);
    query.addBindValue(certData);
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase("TamperTest");
    
    return path;
}

// T-SEC-04: Tampering Detection
// Requirement: FR-2.4.4, NFR-3.3
// Test Plan: test-plan.adoc lines 59-69
// AC: The SignatureVerifier blocks the load. A "Rejected (Tampered)" error is displayed.
//     Content is not rendered and requestAppConsent() returns FALSE.
void TestSignatureVerifierTampering::testTamperingDetection()
{
    SignatureVerifier verifier(this);
    
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Create cartridge with initial H1 hash
    QByteArray originalH1 = QByteArray::fromHex("a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0");
    QString cartridgePath = createTestCartridge(guid, originalH1);
    QVERIFY(!cartridgePath.isEmpty());
    QVERIFY(QFile::exists(cartridgePath));
    
    // Calculate actual H2 from the cartridge
    QByteArray h2Hash = verifier.calculateContentHash(cartridgePath);
    QVERIFY(!h2Hash.isEmpty());
    
    // Update H1 to match H2 (non-tampered state)
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
    
    // Verify non-tampered cartridge first
    VerificationResult result1 = verifier.verifyCartridge(cartridgePath, guid);
    QVERIFY(!result1.isTampered);
    QCOMPARE(result1.effectivePolicy, TrustPolicy::CONSENT_REQUIRED); // L2 requires consent
    
    // Now tamper the cartridge by modifying content
    db = QSqlDatabase::addDatabase("QSQLITE", "TamperContent");
    db.setDatabaseName(cartridgePath);
    if (db.open()) {
        QSqlQuery tamperQuery(db);
        tamperQuery.exec("UPDATE Content_Pages SET content_html = '<p>TAMPERED content</p>' WHERE page_id = 1");
        db.close();
    }
    QSqlDatabase::removeDatabase("TamperContent");
    
    // Verify tampered cartridge - H2 will now differ from H1
    VerificationResult result2 = verifier.verifyCartridge(cartridgePath, guid);
    QVERIFY(result2.isTampered);
    QCOMPARE(result2.effectivePolicy, TrustPolicy::REJECTED);
}

QTEST_MAIN(TestSignatureVerifierTampering)
#include "test_signatureverifier_tampering.moc"
