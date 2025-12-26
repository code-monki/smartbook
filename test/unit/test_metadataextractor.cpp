#include <QtTest>
#include "smartbook/common/metadata/MetadataExtractor.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

using namespace smartbook::common::metadata;
using namespace smartbook::common::security;

class TestMetadataExtractor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testExtractMetadata();      // T-PERS-01: Metadata extraction
    void testCalculateContentHash(); // T-PERS-01: Content hash calculation
    void testHashConsistency();      // Hash should match SignatureVerifier

private:
    QTemporaryDir* m_tempDir;
    QString createTestCartridge(const QString& guid, const QString& title);
};

void TestMetadataExtractor::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
}

void TestMetadataExtractor::cleanupTestCase()
{
    delete m_tempDir;
}

QString TestMetadataExtractor::createTestCartridge(const QString& guid, const QString& title)
{
    QString cartridgePath = m_tempDir->filePath(QString("test_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)));
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "TestCartridge");
    db.setDatabaseName(cartridgePath);
    if (!db.open()) {
        return QString(); // Return empty string on failure
    }
    
    QSqlQuery query(db);
    
    // Create required tables
    query.exec(R"(
        CREATE TABLE Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT,
            publisher TEXT,
            version TEXT,
            publication_year TEXT NOT NULL,
            schema_version TEXT NOT NULL DEFAULT '1.0'
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Content_Pages (
            page_id TEXT PRIMARY KEY,
            page_order INTEGER NOT NULL,
            page_title TEXT,
            html_content TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Content_Themes (
            theme_id TEXT PRIMARY KEY,
            theme_name TEXT,
            theme_config_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT,
            manifest_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_name TEXT,
            form_schema_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT
        )
    )");
    
    // Insert test data
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publisher, publication_year, schema_version) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(title);
    query.addBindValue("Test Author");
    query.addBindValue("Test Publisher");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    if (!query.exec()) {
        db.close();
        QSqlDatabase::removeDatabase("TestCartridge");
        return QString(); // Return empty on failure
    }
    
    query.exec("INSERT INTO Content_Pages (page_id, page_order, page_title, html_content) VALUES ('page1', 1, 'Page 1', '<p>Content</p>')");
    query.exec("INSERT INTO Settings (setting_key, setting_value) VALUES ('test_key', 'test_value')");
    
    db.close();
    QSqlDatabase::removeDatabase("TestCartridge");
    
    return cartridgePath;
}

// T-PERS-01: Metadata extraction
// Requirement: FR-2.5.2 (Mandatory Metadata Storage)
// AC: Metadata is correctly extracted from cartridge (title, author, publisher, publication_year, etc.)
void TestMetadataExtractor::testExtractMetadata()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Test Book Title");
    
    CartridgeMetadata metadata = MetadataExtractor::extractMetadata(cartridgePath);
    
    QCOMPARE(metadata.cartridgeGuid, guid);
    QCOMPARE(metadata.title, "Test Book Title");
    QCOMPARE(metadata.author, "Test Author");
    QCOMPARE(metadata.publisher, "Test Publisher");
    QCOMPARE(metadata.publicationYear, "2025");
    QCOMPARE(metadata.schemaVersion, "1.0");
}

// T-PERS-01: Content hash calculation
// Requirement: FR-2.5.1 (Manifest Creation)
// AC: Content hash (H2) is correctly calculated for manifest entry
void TestMetadataExtractor::testCalculateContentHash()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Hash Test Book");
    
    QByteArray hash1 = MetadataExtractor::calculateContentHash(cartridgePath);
    QVERIFY(!hash1.isEmpty());
    QCOMPARE(hash1.size(), 32); // SHA-256 produces 32 bytes
    
    // Hash should be consistent
    QByteArray hash2 = MetadataExtractor::calculateContentHash(cartridgePath);
    QCOMPARE(hash1, hash2);
}

// Hash consistency: MetadataExtractor and SignatureVerifier should produce same hash
void TestMetadataExtractor::testHashConsistency()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Consistency Test Book");
    
    QByteArray hash1 = MetadataExtractor::calculateContentHash(cartridgePath);
    SignatureVerifier verifier;
    QByteArray hash2 = verifier.calculateContentHash(cartridgePath);
    
    QCOMPARE(hash1, hash2);
}

QTEST_MAIN(TestMetadataExtractor)
#include "test_metadataextractor.moc"

