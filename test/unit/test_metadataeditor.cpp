#include <QtTest>
#include "smartbook/creator/MetadataEditor.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "test_helpers.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSignalSpy>
#include <QRegularExpression>
#include <QApplication>
#include <QDebug>

using namespace smartbook::creator;
using namespace smartbook::common::database;

class TestMetadataEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    
    // T-CT-22: Document Metadata (FR-CT-3.20)
    void testMetadataSaveAndLoad();
    
    // T-CT-22: Document Metadata persistence
    void testMetadataPersistence();
    
    // T-CT-23: Cover Image Management (FR-CT-3.21)
    void testCoverImageManagement();
    
    // T-CT-24: Schema Version Management (FR-CT-3.23)
    void testSchemaVersionManagement();
    
    // T-CT-22: GUID Generation (FR-CT-3.22)
    void testGuidGeneration();

private:
    QTemporaryDir* m_tempDir;
    QString m_testCartridgePath;
    MetadataEditor* m_editor;
    
    QString createTestCartridge();
    void verifyMetadataInDatabase(const QString& path, const QString& title, const QString& author);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestMetadataEditor tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestMetadataEditor::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_testCartridgePath = m_tempDir->filePath("test_cartridge.sqlite");
    m_editor = new MetadataEditor();
    QVERIFY(m_editor != nullptr);
}

void TestMetadataEditor::cleanup()
{
    // Clean up ResourceManager connection after each test
    // MetadataEditor doesn't expose close, but we can work around by using unique paths
    // The ResourceManager will be closed when MetadataEditor is destroyed
}

void TestMetadataEditor::cleanupTestCase()
{
    delete m_editor;
    delete m_tempDir;
}

QString TestMetadataEditor::createTestCartridge()
{
    // Use TestHelpers to create a minimal valid cartridge
    QString path = m_tempDir->filePath("source_cartridge.sqlite");
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString connectionName = QString("TestCartridge_MetadataEditor_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, "Test Book", connectionName);
    if (!created) {
        return QString();
    }
    
    return path;
}

void TestMetadataEditor::verifyMetadataInDatabase(const QString& path, const QString& title, const QString& author)
{
    QString connectionName = QString("TestCartridge_Verify_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector;
    bool opened = connector.openCartridge(path);
    QVERIFY(opened);
    
    QSqlDatabase db = connector.getDatabase();
    QSqlQuery query(db);
    query.prepare("SELECT title, author FROM Metadata LIMIT 1");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), title);
    QCOMPARE(query.value(1).toString(), author);
    
    QString dbConnectionName = db.connectionName();
    connector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
}

// T-CT-22: Document Metadata (FR-CT-3.20)
// AC: All metadata fields are saved to Metadata table. Metadata displays correctly in the interface.
void TestMetadataEditor::testMetadataSaveAndLoad()
{
    // Create test cartridge
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    // Set metadata fields via editor (we'll need to access private members or use public setters)
    // For now, we'll test the save/load functionality
    QString guid = MetadataEditor::generateGuid();
    m_editor->setCartridgeGuid(guid);
    
    // Create cartridge with schema first
    CartridgeDBConnector connector;
    bool opened = connector.openCartridge(cartridgePath);
    QVERIFY(opened);
    
    // Update existing metadata (createTestCartridge already inserted a row)
    QSqlDatabase db = connector.getDatabase();
    QString dbConnectionName = db.connectionName();
    QSqlQuery query(db);
    query.prepare("UPDATE Metadata SET cartridge_guid = ?, title = ?, author = ?, publication_year = ?, schema_version = ?, version = ? WHERE cartridge_guid IS NOT NULL");
    query.addBindValue(guid);
    query.addBindValue("Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.addBindValue("1.0");
    QVERIFY(query.exec());
    
    connector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
    
    // Wait a bit to ensure previous connection is fully closed
    QTest::qWait(10);
    
    // Load metadata
    bool loaded = m_editor->loadMetadata(cartridgePath);
    QVERIFY(loaded);
    
    // Verify GUID was loaded
    QCOMPARE(m_editor->getCartridgeGuid(), guid);
    
    // Save metadata (this will update the database)
    QSignalSpy changedSpy(m_editor, &MetadataEditor::metadataChanged);
    bool saved = m_editor->saveMetadata(cartridgePath);
    QVERIFY(saved);
    
    // Wait a bit to ensure connections are closed
    QTest::qWait(10);
    
    // Verify metadata was saved
    verifyMetadataInDatabase(cartridgePath, "Test Book", "Test Author");
}

// T-CT-22: Document Metadata persistence
// AC: Metadata persists after save and reload.
void TestMetadataEditor::testMetadataPersistence()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QString guid = MetadataEditor::generateGuid();
    m_editor->setCartridgeGuid(guid);
    
    // Create initial cartridge with metadata
    CartridgeDBConnector connector;
    bool opened = connector.openCartridge(cartridgePath);
    QVERIFY(opened);
    
    QSqlDatabase db = connector.getDatabase();
    QString dbConnectionName = db.connectionName();
    QSqlQuery query(db);
    
    // Delete all existing rows to ensure clean state
    query.prepare("DELETE FROM Metadata");
    QVERIFY(query.exec());
    
    // Verify deletion
    query.prepare("SELECT COUNT(*) FROM Metadata");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
    
    // Insert new metadata with our GUID
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, schema_version, version) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Persistent Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.addBindValue("1.0");
    QVERIFY(query.exec());
    
    // Verify insertion
    query.prepare("SELECT cartridge_guid FROM Metadata WHERE cartridge_guid = ?");
    query.addBindValue(guid);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), guid);
    
    connector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
    
    // Wait a bit to ensure connection is fully closed
    QTest::qWait(10);
    
    // Load, modify, save, reload
    bool loaded = m_editor->loadMetadata(cartridgePath);
    QVERIFY(loaded);
    
    // Get the GUID that was actually loaded (should match what we inserted)
    QString loadedGuid = m_editor->getCartridgeGuid();
    QCOMPARE(loadedGuid, guid);
    
    // Save (should persist current state)
    bool saved = m_editor->saveMetadata(cartridgePath);
    QVERIFY(saved);
    
    // Wait a bit to ensure connections are closed
    QTest::qWait(10);
    
    // Reload and verify persistence
    MetadataEditor* editor2 = new MetadataEditor();
    bool loaded2 = editor2->loadMetadata(cartridgePath);
    QVERIFY(loaded2);
    // Compare against the GUID that was loaded (which should be the same as what we inserted)
    QCOMPARE(editor2->getCartridgeGuid(), loadedGuid);
    
    delete editor2;
    
    // Wait a bit after cleanup
    QTest::qWait(10);
}

// T-CT-23: Cover Image Management (FR-CT-3.21)
// AC: Cover image is stored correctly. cover_image_path in Metadata table is updated.
void TestMetadataEditor::testCoverImageManagement()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QString guid = MetadataEditor::generateGuid();
    m_editor->setCartridgeGuid(guid);
    
    // Create cartridge with metadata
    CartridgeDBConnector connector;
    bool opened = connector.openCartridge(cartridgePath);
    QVERIFY(opened);
    
    QSqlDatabase db = connector.getDatabase();
    QString dbConnectionName = db.connectionName();
    QSqlQuery query(db);
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, schema_version, version, cover_image_path) VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Cover Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.addBindValue("1.0");
    query.addBindValue("cover_image_123"); // Resource ID
    QVERIFY(query.exec());
    
    connector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
    
    // Wait a bit to ensure connection is fully closed
    QTest::qWait(10);
    
    // Load metadata (should load cover image path)
    bool loaded = m_editor->loadMetadata(cartridgePath);
    QVERIFY(loaded);
    
    // Save metadata (should preserve cover image path)
    bool saved = m_editor->saveMetadata(cartridgePath);
    QVERIFY(saved);
    
    // Wait a bit to ensure connections are closed
    QTest::qWait(10);
    
    // Verify cover_image_path was preserved
    QString verifyConnectionName = QString("TestCartridge_Verify_Cover_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector verifyConnector;
    opened = verifyConnector.openCartridge(cartridgePath);
    QVERIFY(opened);
    
    db = verifyConnector.getDatabase();
    dbConnectionName = db.connectionName();
    query = QSqlQuery(db);
    query.prepare("SELECT cover_image_path FROM Metadata WHERE cartridge_guid = ?");
    query.addBindValue(guid);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QString coverPath = query.value(0).toString();
    QVERIFY(!coverPath.isEmpty());
    
    verifyConnector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
}

// T-CT-24: Schema Version Management (FR-CT-3.23)
// AC: schema_version field is set to current Smartbook Format Specification version.
void TestMetadataEditor::testSchemaVersionManagement()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QString guid = MetadataEditor::generateGuid();
    m_editor->setCartridgeGuid(guid);
    
    // Create cartridge
    CartridgeDBConnector connector;
    bool opened = connector.openCartridge(cartridgePath);
    QVERIFY(opened);
    
    QSqlDatabase db = connector.getDatabase();
    QString dbConnectionName = db.connectionName();
    QSqlQuery query(db);
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, schema_version, version) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Schema Test Book");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.addBindValue("1.0");
    QVERIFY(query.exec());
    
    connector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
    
    // Wait a bit to ensure connection is fully closed
    QTest::qWait(10);
    
    // Load and save metadata
    bool loaded = m_editor->loadMetadata(cartridgePath);
    QVERIFY(loaded);
    
    bool saved = m_editor->saveMetadata(cartridgePath);
    QVERIFY(saved);
    
    // Wait a bit to ensure connections are closed
    QTest::qWait(10);
    
    // Verify schema_version is set correctly
    QString verifyConnectionName = QString("TestCartridge_Verify_Schema_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector verifyConnector;
    opened = verifyConnector.openCartridge(cartridgePath);
    QVERIFY(opened);
    
    db = verifyConnector.getDatabase();
    dbConnectionName = db.connectionName();
    query = QSqlQuery(db);
    query.prepare("SELECT schema_version FROM Metadata WHERE cartridge_guid = ?");
    query.addBindValue(guid);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QString schemaVersion = query.value(0).toString();
    QCOMPARE(schemaVersion, QString("1.0"));
    
    verifyConnector.closeCartridge();
    
    // Explicitly remove the connection from the pool
    if (QSqlDatabase::contains(dbConnectionName)) {
        QSqlDatabase::removeDatabase(dbConnectionName);
    }
}

// T-CT-22: GUID Generation (FR-CT-3.22)
// AC: A UUID Version 4 cartridge_guid is automatically generated.
void TestMetadataEditor::testGuidGeneration()
{
    // Test static GUID generation
    QString guid1 = MetadataEditor::generateGuid();
    QString guid2 = MetadataEditor::generateGuid();
    
    QVERIFY(!guid1.isEmpty());
    QVERIFY(!guid2.isEmpty());
    QVERIFY(guid1 != guid2); // Should be unique
    
    // Verify UUID v4 format
    QRegularExpression uuidRegex("^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$", 
                                 QRegularExpression::CaseInsensitiveOption);
    QVERIFY(uuidRegex.match(guid1).hasMatch());
    QVERIFY(uuidRegex.match(guid2).hasMatch());
    
    // Test setting GUID
    m_editor->setCartridgeGuid(guid1);
    QCOMPARE(m_editor->getCartridgeGuid(), guid1);
}

#include "test_metadataeditor.moc"

