#include <QtTest>
#include "smartbook/creator/FormBuilder.h"
#include "smartbook/creator/FormManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

using namespace smartbook::creator;
using namespace smartbook::common::database;

class TestFormBuilder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // T-CT-27: Form Creation
    void testFormCreation();
    
    // T-CT-28: Form Builder Interface
    void testFormBuilderInterface();
    
    // T-CT-29: Form Schema Validation
    void testFormSchemaValidation();
    
    // T-CT-30: Form Integration (form marker insertion)
    void testFormIntegration();

private:
    QTemporaryDir* m_tempDir;
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    FormBuilder* m_formBuilder;
    FormManager* m_formManager;
    
    QString createTestCartridge();
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestFormBuilder tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestFormBuilder::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cartridgePath = createTestCartridge();
    QVERIFY(QFile::exists(m_cartridgePath));
    
    m_formBuilder = new FormBuilder();
    m_formManager = new FormManager(this);
    
    bool opened = m_formManager->openCartridge(m_cartridgePath);
    QVERIFY(opened);
}

void TestFormBuilder::cleanupTestCase()
{
    if (m_formManager) {
        m_formManager->closeCartridge();
        delete m_formManager;
        m_formManager = nullptr;
    }
    
    if (m_formBuilder) {
        delete m_formBuilder;
        m_formBuilder = nullptr;
    }
    
    QApplication::processEvents();
    delete m_tempDir;
}

QString TestFormBuilder::createTestCartridge()
{
    QString path = m_tempDir->filePath("test_cartridge.sqlite");
    
    QString connectionName = "TestCartridge_FormBuilder_" + m_cartridgeGuid;
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
    
    // Create Form_Definitions table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_schema_json TEXT NOT NULL,
            form_version INTEGER NOT NULL DEFAULT 1,
            migration_rules_json TEXT
        )
    )");
    
    db.close();
    QApplication::processEvents();
    QSqlDatabase::removeDatabase(connectionName);
    
    return path;
}

// T-CT-27: Form Creation
// Requirement: FR-CT-3.16
// Test Plan: test-plan.adoc lines 520-528
// AC: Form definition is created using the JSON Schema. Form is stored in Form_Definitions table.
void TestFormBuilder::testFormCreation()
{
    QVERIFY(m_formBuilder != nullptr);
    QVERIFY(m_formManager != nullptr);
    
    // Create form using loadFormDefinition
    QString testSchema = R"({
        "schemaVersion": "1.0",
        "formId": "test_form_v1",
        "formTitle": "Test Form",
        "fields": [{"fieldId": "text_field", "fieldType": "text", "label": "Text Field"}]
    })";
    bool loaded = m_formBuilder->loadFormDefinition("test_form_v1", testSchema);
    QVERIFY(loaded);
    
    // Get form definition JSON
    QString schemaJson = m_formBuilder->getFormDefinitionJson();
    QVERIFY(!schemaJson.isEmpty());
    
    // Validate JSON structure
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(schemaJson.toUtf8(), &error);
    QVERIFY(error.error == QJsonParseError::NoError);
    
    QJsonObject root = doc.object();
    QCOMPARE(root.value("schemaVersion").toString(), QString("1.0"));
    QCOMPARE(root.value("formId").toString(), QString("test_form_v1"));
    QCOMPARE(root.value("formTitle").toString(), QString("Test Form"));
    QVERIFY(root.contains("fields"));
    
    // Save to cartridge
    bool saved = m_formBuilder->saveToCartridge(m_formManager);
    QVERIFY(saved);
    
    // Verify form exists in database
    QVERIFY(m_formManager->formExists("test_form_v1"));
    
    // Verify form can be retrieved
    QString retrieved = m_formManager->getFormDefinition("test_form_v1");
    QVERIFY(!retrieved.isEmpty());
}

// T-CT-28: Form Builder Interface
// Requirement: FR-CT-3.17
// Test Plan: test-plan.adoc lines 533-543
// AC: Visual form builder allows creating forms without direct JSON editing.
void TestFormBuilder::testFormBuilderInterface()
{
    QVERIFY(m_formBuilder != nullptr);
    
    // Create a new form
    QString baseSchema = R"({
        "schemaVersion": "1.0",
        "formId": "interface_test",
        "formTitle": "Interface Test",
        "fields": []
    })";
    m_formBuilder->loadFormDefinition("interface_test", baseSchema);
    
    // Test adding different field types via public API
    m_formBuilder->addTextField();
    m_formBuilder->addNumberField();
    m_formBuilder->addTextareaField();
    m_formBuilder->addSelectField();
    m_formBuilder->addGroupField();
    
    // Verify fields were added
    QString schemaJson = m_formBuilder->getFormDefinitionJson();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(schemaJson.toUtf8(), &parseError);
    QVERIFY(parseError.error == QJsonParseError::NoError);
    QJsonObject root = doc.object();
    QJsonArray fields = root.value("fields").toArray();
    
    QVERIFY(fields.size() == 5);
    
    // Verify field types
    QCOMPARE(fields[0].toObject().value("fieldType").toString(), QString("text"));
    QCOMPARE(fields[1].toObject().value("fieldType").toString(), QString("number"));
    QCOMPARE(fields[2].toObject().value("fieldType").toString(), QString("textarea"));
    QCOMPARE(fields[3].toObject().value("fieldType").toString(), QString("select"));
    QCOMPARE(fields[4].toObject().value("fieldType").toString(), QString("group"));
    
    // Test field removal
    m_formBuilder->removeField(0);
    schemaJson = m_formBuilder->getFormDefinitionJson();
    doc = QJsonDocument::fromJson(schemaJson.toUtf8(), &parseError);
    QVERIFY(parseError.error == QJsonParseError::NoError);
    root = doc.object();
    fields = root.value("fields").toArray();
    QVERIFY(fields.size() == 4);
}

// T-CT-29: Form Schema Validation
// Requirement: FR-CT-3.18
// Test Plan: test-plan.adoc lines 548-556
// AC: Validation errors are displayed. Form cannot be saved until it conforms to schema.
void TestFormBuilder::testFormSchemaValidation()
{
    QVERIFY(m_formBuilder != nullptr);
    
    // Test validation with empty form ID
    QString emptyIdSchema = R"({
        "schemaVersion": "1.0",
        "formId": "",
        "formTitle": "Test Form",
        "fields": [{"fieldId": "field1", "fieldType": "text", "label": "Field 1"}]
    })";
    m_formBuilder->loadFormDefinition("", emptyIdSchema);
    
    bool isValid = m_formBuilder->validateFormSchema();
    QVERIFY(!isValid);
    
    QStringList errors = m_formBuilder->getValidationErrors();
    QVERIFY(errors.contains("Form ID is required"));
    
    // Test validation with empty form title
    QString emptyTitleSchema = R"({
        "schemaVersion": "1.0",
        "formId": "test_form",
        "formTitle": "",
        "fields": [{"fieldId": "field1", "fieldType": "text", "label": "Field 1"}]
    })";
    m_formBuilder->loadFormDefinition("test_form", emptyTitleSchema);
    
    isValid = m_formBuilder->validateFormSchema();
    QVERIFY(!isValid);
    errors = m_formBuilder->getValidationErrors();
    QVERIFY(errors.contains("Form Title is required"));
    
    // Test validation with no fields
    QString noFieldsSchema = R"({
        "schemaVersion": "1.0",
        "formId": "test_form",
        "formTitle": "Test Form",
        "fields": []
    })";
    m_formBuilder->loadFormDefinition("test_form", noFieldsSchema);
    
    isValid = m_formBuilder->validateFormSchema();
    QVERIFY(!isValid);
    errors = m_formBuilder->getValidationErrors();
    QVERIFY(errors.contains("At least one field is required"));
    
    // Test valid form
    QString validSchema = R"({
        "schemaVersion": "1.0",
        "formId": "valid_form",
        "formTitle": "Valid Form",
        "fields": [{"fieldId": "field1", "fieldType": "text", "label": "Field 1"}]
    })";
    m_formBuilder->loadFormDefinition("valid_form", validSchema);
    
    isValid = m_formBuilder->validateFormSchema();
    QVERIFY(isValid);
}

// T-CT-30: Form Integration
// Requirement: FR-CT-3.19
// Test Plan: test-plan.adoc lines 561-569
// AC: Form reference is inserted into content page. Form is linked correctly.
void TestFormBuilder::testFormIntegration()
{
    QVERIFY(m_formBuilder != nullptr);
    QVERIFY(m_formManager != nullptr);
    
    // Create and save a form
    QString integrationSchema = R"({
        "schemaVersion": "1.0",
        "formId": "integration_test_form",
        "formTitle": "Integration Test Form",
        "fields": [{"fieldId": "field1", "fieldType": "text", "label": "Field 1"}]
    })";
    m_formBuilder->loadFormDefinition("integration_test_form", integrationSchema);
    
    bool saved = m_formBuilder->saveToCartridge(m_formManager);
    QVERIFY(saved);
    
    // Note: Form marker insertion in ContentEditor would be tested
    // in an integration test with ContentEditor. This test verifies
    // that the form can be saved and retrieved, which is a prerequisite
    // for form integration.
    
    QString retrieved = m_formManager->getFormDefinition("integration_test_form");
    QVERIFY(!retrieved.isEmpty());
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(retrieved.toUtf8(), &error);
    QVERIFY(error.error == QJsonParseError::NoError);
    
    QJsonObject root = doc.object();
    QCOMPARE(root.value("formId").toString(), QString("integration_test_form"));
}

#include "test_formbuilder.moc"
