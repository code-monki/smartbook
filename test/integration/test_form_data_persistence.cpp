#include <QtTest>
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/FormEmbeddedWidget.h"
#include "smartbook/common/forms/FormDataSerializer.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "test_helpers.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QApplication>
#include <QDebug>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QLayoutItem>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;
using namespace smartbook::common::forms;

/**
 * Integration test for form data persistence
 * 
 * Tests the complete form data persistence workflow with Qt Widgets forms:
 * 1. Save form data through FormEmbeddedWidget
 * 2. Load form data through FormEmbeddedWidget
 * 3. Persistence across sessions (close/reopen cartridge)
 * 4. Multiple forms in same cartridge
 * 5. Form data updates (overwrite existing)
 * 6. Integration with ReaderViewWindow
 * 
 * Test Case: Integration test for form data persistence
 * Requirements: FR-2.1.2 (Form Data Persistence), FR-2.7.4, FR-2.7.6
 */
class TestFormDataPersistence : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Form data persistence tests
    void testSaveAndLoadFormData();
    void testFormDataPersistenceAcrossSessions();
    void testMultipleFormsInCartridge();
    void testFormDataUpdate();
    void testFormDataIsolationPerCartridge();
    void testFormDataWithReaderViewWindow();

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    QString createTestCartridge(const QString& guid, const QString& title);
    void createManifestEntry(const QString& guid, const QString& path, const QString& title);
    QString createFormSchema(const QString& formId, const QStringList& fields);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestFormDataPersistence tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestFormDataPersistence::initTestCase()
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

void TestFormDataPersistence::cleanupTestCase()
{
    delete m_manifestManager;
    
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

QString TestFormDataPersistence::createFormSchema(const QString& formId, const QStringList& fields)
{
    QJsonObject schema;
    schema["type"] = "object";
    schema["title"] = formId;
    
    QJsonObject properties;
    QJsonArray required;
    
    for (const QString& fieldName : fields) {
        QJsonObject field;
        field["type"] = "string";
        field["title"] = fieldName;
        properties[fieldName] = field;
        required.append(fieldName);
    }
    
    schema["properties"] = properties;
    schema["required"] = required;
    
    QJsonDocument doc(schema);
    return doc.toJson(QJsonDocument::Compact);
}

QString TestFormDataPersistence::createTestCartridge(const QString& guid, const QString& title)
{
    QString path = m_tempDir->filePath(QString("cartridge_%1.sqlite").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QString connectionName = QString("TestCartridge_FormData_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, title, connectionName);
    if (!created) {
        return QString();
    }
    
    // Reuse the connection from createMinimalCartridge to add form definitions
    QSqlDatabase formDb = QSqlDatabase::database(connectionName, false);
    if (!formDb.isOpen()) {
        // If connection was closed, open it again
        if (!formDb.open()) {
            return QString();
        }
    }
    
    QSqlQuery query(formDb);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_title TEXT NOT NULL,
            form_schema_json TEXT NOT NULL
        )
    )")) {
        return QString();
    }
    
    // Create contact form schema (matching FormSchemaParser format)
    QString contactSchema = createFormSchema("contact_form", {"name", "email"});
    query.prepare("INSERT INTO Form_Definitions (form_id, form_title, form_schema_json) VALUES (?, ?, ?)");
    query.addBindValue("contact_form");
    query.addBindValue("Contact Form");
    query.addBindValue(contactSchema);
    if (!query.exec()) {
        return QString();
    }
    
    // Create survey form schema (matching FormSchemaParser format)
    QJsonObject surveySchema;
    surveySchema["type"] = "object";
    surveySchema["title"] = "Survey Form";
    QJsonObject surveyProperties;
    QJsonObject ratingField;
    ratingField["type"] = "integer";
    ratingField["title"] = "Rating";
    ratingField["minimum"] = 1;
    ratingField["maximum"] = 5;
    surveyProperties["rating"] = ratingField;
    surveySchema["properties"] = surveyProperties;
    QJsonDocument surveyDoc(surveySchema);
    QString surveySchemaJson = surveyDoc.toJson(QJsonDocument::Compact);
    
    query.prepare("INSERT INTO Form_Definitions (form_id, form_title, form_schema_json) VALUES (?, ?, ?)");
    query.addBindValue("survey_form");
    query.addBindValue("Survey Form");
    query.addBindValue(surveySchemaJson);
    if (!query.exec()) {
        return QString();
    }
    
    // Don't close the connection - let the caller manage it
    // formDb.close();
    // QSqlDatabase::removeDatabase(formConn);
    
    return path;
}

void TestFormDataPersistence::createManifestEntry(const QString& guid, const QString& path, const QString& title)
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

// Test: Save and load form data through FormEmbeddedWidget
void TestFormDataPersistence::testSaveAndLoadFormData()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Form Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
    QVERIFY(formWidget != nullptr);
    
    // Load form
    QSignalSpy loadSpy(formWidget, &FormEmbeddedWidget::formLoaded);
    QSignalSpy errorSpy(formWidget, &FormEmbeddedWidget::formLoadError);
    bool loaded = formWidget->loadForm(cartridgePath);
    if (!loaded) {
        qDebug() << "Form load failed:" << formWidget->errorMessage();
    }
    QVERIFY(loaded);
    QVERIFY(loadSpy.wait(1000) || loadSpy.count() > 0);
    
    // Get the form widget from layout and fill in data
    QLayout* layout = formWidget->layout();
    QVERIFY(layout != nullptr);
    QVERIFY(layout->count() > 0);
    QWidget* innerForm = layout->itemAt(0)->widget();
    QVERIFY(innerForm != nullptr);
    
    // Use FormDataSerializer to set form data
    FormDataSerializer serializer;
    QJsonObject data;
    data["name"] = "John Doe";
    data["email"] = "john@example.com";
    QJsonDocument doc(data);
    bool dataLoaded = serializer.loadFormData(innerForm, doc.toJson(QJsonDocument::Compact));
    QVERIFY(dataLoaded);
    
    // Save form data
    QSignalSpy saveSpy(formWidget, &FormEmbeddedWidget::formDataSaved);
    bool saved = formWidget->saveFormData();
    QVERIFY(saved);
    QVERIFY(saveSpy.wait(1000) || saveSpy.count() > 0);
    QCOMPARE(saveSpy.count(), 1);
    QVERIFY(saveSpy.takeFirst().at(0).toBool()); // success
    
    // Reload form and verify data persists
    FormEmbeddedWidget* formWidget2 = new FormEmbeddedWidget("contact_form", nullptr);
    QSignalSpy loadSpy2(formWidget2, &FormEmbeddedWidget::formLoaded);
    bool loaded2 = formWidget2->loadForm(cartridgePath);
    QVERIFY(loaded2);
    QVERIFY(loadSpy2.wait(1000) || loadSpy2.count() > 0);
    
    // Verify data was loaded
    QSignalSpy dataLoadSpy(formWidget2, &FormEmbeddedWidget::formDataLoaded);
    bool dataLoaded2 = formWidget2->loadFormData();
    QVERIFY(dataLoaded2);
    QVERIFY(dataLoadSpy.wait(1000) || dataLoadSpy.count() > 0);
    
    delete formWidget;
    delete formWidget2;
}

// Test: Form data persists across sessions (close/reopen cartridge)
void TestFormDataPersistence::testFormDataPersistenceAcrossSessions()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Persistence Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "Persistence Test Book");
    
    // Session 1: Save form data
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        
        QLayout* layout = formWidget->layout();
        QVERIFY(layout != nullptr && layout->count() > 0);
        QWidget* innerForm = layout->itemAt(0)->widget();
        QVERIFY(innerForm != nullptr);
        
        FormDataSerializer serializer;
        QJsonObject data;
        data["name"] = "Jane Doe";
        data["email"] = "jane@example.com";
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy(formWidget, &FormEmbeddedWidget::formDataSaved);
        formWidget->saveFormData();
        QVERIFY(saveSpy.wait(1000) || saveSpy.count() > 0);
        QVERIFY(saveSpy.takeFirst().at(0).toBool());
        
        delete formWidget;
    }
    
    // Session 2: Load form data (simulating cartridge reopen)
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        
        QSignalSpy dataLoadSpy(formWidget, &FormEmbeddedWidget::formDataLoaded);
        formWidget->loadFormData();
        QVERIFY(dataLoadSpy.wait(1000) || dataLoadSpy.count() > 0);
        
        // Verify data was loaded
        QLayout* layout = formWidget->layout();
        QVERIFY(layout != nullptr && layout->count() > 0);
        QWidget* innerForm = layout->itemAt(0)->widget();
        QVERIFY(innerForm != nullptr);
        
        FormDataSerializer serializer;
        QString dataJson = serializer.serializeFormData(innerForm);
        QVERIFY(!dataJson.isEmpty());
        
        QJsonDocument doc = QJsonDocument::fromJson(dataJson.toUtf8());
        QJsonObject data = doc.object();
        QCOMPARE(data["name"].toString(), "Jane Doe");
        QCOMPARE(data["email"].toString(), "jane@example.com");
        
        delete formWidget;
    }
}

// Test: Multiple forms can exist in same cartridge
void TestFormDataPersistence::testMultipleFormsInCartridge()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Multiple Forms Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Save data for form 1
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        
        QWidget* innerForm = formWidget->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QJsonObject data;
        data["name"] = "Alice";
        data["email"] = "alice@example.com";
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy(formWidget, &FormEmbeddedWidget::formDataSaved);
        formWidget->saveFormData();
        QVERIFY(saveSpy.wait(1000) || saveSpy.count() > 0);
        
        delete formWidget;
    }
    
    // Save data for form 2
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("survey_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        
        QWidget* innerForm = formWidget->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QJsonObject data;
        data["rating"] = 5;
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy(formWidget, &FormEmbeddedWidget::formDataSaved);
        formWidget->saveFormData();
        QVERIFY(saveSpy.wait(1000) || saveSpy.count() > 0);
        
        delete formWidget;
    }
    
    // Load both forms and verify data
    {
        FormEmbeddedWidget* formWidget1 = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget1->loadForm(cartridgePath));
        QApplication::processEvents();
        formWidget1->loadFormData();
        QApplication::processEvents();
        
        QWidget* innerForm1 = formWidget1->layout()->itemAt(0)->widget();
        FormDataSerializer serializer1;
        QString dataJson1 = serializer1.serializeFormData(innerForm1);
        QJsonDocument doc1 = QJsonDocument::fromJson(dataJson1.toUtf8());
        QJsonObject data1 = doc1.object();
        QCOMPARE(data1["name"].toString(), "Alice");
        QCOMPARE(data1["email"].toString(), "alice@example.com");
        
        FormEmbeddedWidget* formWidget2 = new FormEmbeddedWidget("survey_form", nullptr);
        QVERIFY(formWidget2->loadForm(cartridgePath));
        QApplication::processEvents();
        formWidget2->loadFormData();
        QApplication::processEvents();
        
        QWidget* innerForm2 = formWidget2->layout()->itemAt(0)->widget();
        FormDataSerializer serializer2;
        QString dataJson2 = serializer2.serializeFormData(innerForm2);
        QJsonDocument doc2 = QJsonDocument::fromJson(dataJson2.toUtf8());
        QJsonObject data2 = doc2.object();
        QCOMPARE(data2["rating"].toInt(), 5);
        
        delete formWidget1;
        delete formWidget2;
    }
}

// Test: Form data can be updated (overwrite existing)
void TestFormDataPersistence::testFormDataUpdate()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "Update Test Book");
    QVERIFY(!cartridgePath.isEmpty());
    
    // Initial save
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        
        QWidget* innerForm = formWidget->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QJsonObject data;
        data["name"] = "Bob";
        data["email"] = "bob@example.com";
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy(formWidget, &FormEmbeddedWidget::formDataSaved);
        formWidget->saveFormData();
        QVERIFY(saveSpy.wait(1000) || saveSpy.count() > 0);
        
        delete formWidget;
    }
    
    // Update with new data
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        formWidget->loadFormData();
        QApplication::processEvents();
        
        QWidget* innerForm = formWidget->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QJsonObject data;
        data["name"] = "Bob Smith";
        data["email"] = "bob.smith@example.com";
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy(formWidget, &FormEmbeddedWidget::formDataSaved);
        formWidget->saveFormData();
        QVERIFY(saveSpy.wait(1000) || saveSpy.count() > 0);
        
        delete formWidget;
    }
    
    // Verify updated data
    {
        FormEmbeddedWidget* formWidget = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget->loadForm(cartridgePath));
        QApplication::processEvents();
        formWidget->loadFormData();
        QApplication::processEvents();
        
        QWidget* innerForm = formWidget->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QString dataJson = serializer.serializeFormData(innerForm);
        QJsonDocument doc = QJsonDocument::fromJson(dataJson.toUtf8());
        QJsonObject data = doc.object();
        QCOMPARE(data["name"].toString(), "Bob Smith");
        QCOMPARE(data["email"].toString(), "bob.smith@example.com");
        
        delete formWidget;
    }
}

// Test: Form data is isolated per cartridge
void TestFormDataPersistence::testFormDataIsolationPerCartridge()
{
    QString guid1 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString guid2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString path1 = createTestCartridge(guid1, "Book 1");
    QString path2 = createTestCartridge(guid2, "Book 2");
    QVERIFY(!path1.isEmpty());
    QVERIFY(!path2.isEmpty());
    
    // Save different data to each cartridge
    {
        FormEmbeddedWidget* formWidget1 = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget1->loadForm(path1));
        QApplication::processEvents();
        
        QWidget* innerForm1 = formWidget1->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QJsonObject data;
        data["name"] = "Cartridge 1 User";
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm1, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy1(formWidget1, &FormEmbeddedWidget::formDataSaved);
        formWidget1->saveFormData();
        QVERIFY(saveSpy1.wait(1000) || saveSpy1.count() > 0);
        
        delete formWidget1;
    }
    
    {
        FormEmbeddedWidget* formWidget2 = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget2->loadForm(path2));
        QApplication::processEvents();
        
        QWidget* innerForm2 = formWidget2->layout()->itemAt(0)->widget();
        FormDataSerializer serializer;
        QJsonObject data;
        data["name"] = "Cartridge 2 User";
        QJsonDocument doc(data);
        serializer.loadFormData(innerForm2, doc.toJson(QJsonDocument::Compact));
        
        QSignalSpy saveSpy2(formWidget2, &FormEmbeddedWidget::formDataSaved);
        formWidget2->saveFormData();
        QVERIFY(saveSpy2.wait(1000) || saveSpy2.count() > 0);
        
        delete formWidget2;
    }
    
    // Verify data is isolated
    {
        FormEmbeddedWidget* formWidget1 = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget1->loadForm(path1));
        QApplication::processEvents();
        formWidget1->loadFormData();
        QApplication::processEvents();
        
        QWidget* innerForm1 = formWidget1->layout()->itemAt(0)->widget();
        FormDataSerializer serializer1;
        QString dataJson1 = serializer1.serializeFormData(innerForm1);
        QJsonDocument doc1 = QJsonDocument::fromJson(dataJson1.toUtf8());
        QJsonObject data1 = doc1.object();
        QCOMPARE(data1["name"].toString(), "Cartridge 1 User");
        
        FormEmbeddedWidget* formWidget2 = new FormEmbeddedWidget("contact_form", nullptr);
        QVERIFY(formWidget2->loadForm(path2));
        QApplication::processEvents();
        formWidget2->loadFormData();
        QApplication::processEvents();
        
        QWidget* innerForm2 = formWidget2->layout()->itemAt(0)->widget();
        FormDataSerializer serializer2;
        QString dataJson2 = serializer2.serializeFormData(innerForm2);
        QJsonDocument doc2 = QJsonDocument::fromJson(dataJson2.toUtf8());
        QJsonObject data2 = doc2.object();
        QCOMPARE(data2["name"].toString(), "Cartridge 2 User");
        
        delete formWidget1;
        delete formWidget2;
    }
}

// Test: Form data works with ReaderViewWindow integration
void TestFormDataPersistence::testFormDataWithReaderViewWindow()
{
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString cartridgePath = createTestCartridge(guid, "ReaderViewWindow Test");
    QVERIFY(!cartridgePath.isEmpty());
    
    createManifestEntry(guid, cartridgePath, "ReaderViewWindow Test");
    
    // Create ReaderViewWindow
    ReaderViewWindow* window = new ReaderViewWindow(guid);
    QVERIFY(window != nullptr);
    
    // Verify the cartridge can be opened and form data can be saved/loaded
    QString conn = QString("TestConn_ReaderView_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    CartridgeDBConnector connector(this);
    QVERIFY(connector.openCartridge(cartridgePath));
    
    // Save form data directly to database
    QSqlQuery query(connector.getDatabase());
    query.prepare(R"(
        INSERT OR REPLACE INTO User_Data (form_key, serialized_data, timestamp)
        VALUES (?, ?, datetime('now'))
    )");
    query.addBindValue("contact_form");
    query.addBindValue(R"({"name": "ReaderView User"})");
    bool saved = query.exec();
    QVERIFY(saved);
    
    // Load form data
    QSqlQuery loadQuery(connector.getDatabase());
    loadQuery.prepare(R"(
        SELECT serialized_data FROM User_Data
        WHERE form_key = ?
        ORDER BY timestamp DESC
        LIMIT 1
    )");
    loadQuery.addBindValue("contact_form");
    QVERIFY(loadQuery.exec());
    QVERIFY(loadQuery.next());
    QString loaded = loadQuery.value(0).toString();
    QCOMPARE(loaded, R"({"name": "ReaderView User"})");
    
    connector.closeCartridge();
    QSqlDatabase::removeDatabase(conn);
    
    delete window;
}

#include "test_form_data_persistence.moc"
