/**
 * @file test_formschemaparser.cpp
 * @brief Unit tests for FormSchemaParser
 * 
 * Tests for parsing JSON form schemas and creating Qt Widget form layouts.
 * 
 * Test Cases:
 * T-FORM-01: Parse JSON schema
 * T-FORM-02: Generate text input
 * T-FORM-03: Generate textarea
 * T-FORM-04: Generate select
 * T-FORM-05: Generate checkbox
 * T-FORM-06: Generate radio buttons
 * T-FORM-07: Generate number input
 * T-FORM-08: Generate date input
 */

#include <QtTest>
#include "smartbook/common/forms/FormSchemaParser.h"
#include <QApplication>
#include <QWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

using namespace smartbook::common::forms;

class TestFormSchemaParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    
    // T-FORM-01: Parse JSON schema
    void testParseJsonSchema();
    void testParseInvalidJson();
    void testParseEmptySchema();
    
    // T-FORM-02: Generate text input
    void testGenerateTextInput();
    void testGenerateTextInputWithValidation();
    
    // T-FORM-03: Generate textarea
    void testGenerateTextarea();
    
    // T-FORM-04: Generate select
    void testGenerateSelect();
    
    // T-FORM-05: Generate checkbox
    void testGenerateCheckbox();
    
    // T-FORM-06: Generate radio buttons
    void testGenerateRadioButtons();
    
    // T-FORM-07: Generate number input
    void testGenerateNumberInput();
    void testGenerateNumberInputWithRange();
    
    // T-FORM-08: Generate date input
    void testGenerateDateInput();
    
    // Additional tests
    void testGenerateMultipleFields();
    void testRequiredFieldValidation();
};

void TestFormSchemaParser::initTestCase()
{
    // Ensure QApplication exists
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
}

// T-FORM-01: Parse JSON schema
void TestFormSchemaParser::testParseJsonSchema()
{
    QJsonObject schema;
    schema["type"] = "object";
    schema["title"] = "Test Form";
    
    QJsonObject properties;
    
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(jsonString);
    
    QVERIFY(formWidget != nullptr);
    QVERIFY(formWidget->layout() != nullptr);
    QVERIFY(qobject_cast<QFormLayout*>(formWidget->layout()) != nullptr);
    
    delete formWidget;
}

void TestFormSchemaParser::testParseInvalidJson()
{
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema("invalid json");
    
    QVERIFY(formWidget == nullptr);
}

void TestFormSchemaParser::testParseEmptySchema()
{
    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = QJsonObject();
    
    QJsonDocument doc(schema);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(jsonString);
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 0);
    
    delete formWidget;
}

// T-FORM-02: Generate text input
void TestFormSchemaParser::testGenerateTextInput()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    
    QVERIFY(fieldWidget != nullptr);
    QVERIFY(qobject_cast<QLineEdit*>(fieldWidget) != nullptr);
    
    delete formWidget;
}

void TestFormSchemaParser::testGenerateTextInputWithValidation()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject emailField;
    emailField["type"] = "string";
    emailField["title"] = "Email";
    emailField["required"] = true;
    emailField["pattern"] = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    properties["email"] = emailField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    delete formWidget;
}

// T-FORM-03: Generate textarea
void TestFormSchemaParser::testGenerateTextarea()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject descriptionField;
    descriptionField["type"] = "string";
    descriptionField["title"] = "Description";
    descriptionField["format"] = "textarea";
    properties["description"] = descriptionField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    QVERIFY(qobject_cast<QTextEdit*>(fieldWidget) != nullptr);
    
    delete formWidget;
}

// T-FORM-04: Generate select
void TestFormSchemaParser::testGenerateSelect()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject colorField;
    colorField["type"] = "string";
    colorField["title"] = "Color";
    colorField["format"] = "select";
    
    QJsonArray enumValues;
    enumValues.append("red");
    enumValues.append("green");
    enumValues.append("blue");
    colorField["enum"] = enumValues;
    
    properties["color"] = colorField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    QComboBox* comboBox = qobject_cast<QComboBox*>(fieldWidget);
    QVERIFY(comboBox != nullptr);
    QCOMPARE(comboBox->count(), 3);
    QCOMPARE(comboBox->itemText(0), "red");
    QCOMPARE(comboBox->itemText(1), "green");
    QCOMPARE(comboBox->itemText(2), "blue");
    
    delete formWidget;
}

// T-FORM-05: Generate checkbox
void TestFormSchemaParser::testGenerateCheckbox()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject agreeField;
    agreeField["type"] = "boolean";
    agreeField["title"] = "I agree";
    properties["agree"] = agreeField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    QVERIFY(qobject_cast<QCheckBox*>(fieldWidget) != nullptr);
    
    delete formWidget;
}

// T-FORM-06: Generate radio buttons
void TestFormSchemaParser::testGenerateRadioButtons()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject sizeField;
    sizeField["type"] = "string";
    sizeField["title"] = "Size";
    sizeField["format"] = "radio";
    
    QJsonArray enumValues;
    enumValues.append("small");
    enumValues.append("medium");
    enumValues.append("large");
    sizeField["enum"] = enumValues;
    
    properties["size"] = sizeField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    // Radio buttons are in a container widget
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    
    // Find radio buttons in the container
    QList<QRadioButton*> radioButtons = fieldWidget->findChildren<QRadioButton*>();
    QCOMPARE(radioButtons.size(), 3);
    
    delete formWidget;
}

// T-FORM-07: Generate number input
void TestFormSchemaParser::testGenerateNumberInput()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject ageField;
    ageField["type"] = "integer";
    ageField["title"] = "Age";
    properties["age"] = ageField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    QVERIFY(qobject_cast<QSpinBox*>(fieldWidget) != nullptr);
    
    delete formWidget;
}

void TestFormSchemaParser::testGenerateNumberInputWithRange()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject scoreField;
    scoreField["type"] = "number";
    scoreField["title"] = "Score";
    scoreField["minimum"] = 0.0;
    scoreField["maximum"] = 100.0;
    properties["score"] = scoreField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(fieldWidget);
    QVERIFY(spinBox != nullptr);
    QCOMPARE(spinBox->minimum(), 0.0);
    QCOMPARE(spinBox->maximum(), 100.0);
    
    delete formWidget;
}

// T-FORM-08: Generate date input
void TestFormSchemaParser::testGenerateDateInput()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject birthDateField;
    birthDateField["type"] = "string";
    birthDateField["title"] = "Birth Date";
    birthDateField["format"] = "date";
    properties["birthDate"] = birthDateField;
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    QWidget* fieldWidget = layout->itemAt(0, QFormLayout::FieldRole)->widget();
    QVERIFY(fieldWidget != nullptr);
    QVERIFY(qobject_cast<QDateEdit*>(fieldWidget) != nullptr);
    
    delete formWidget;
}

// Additional tests
void TestFormSchemaParser::testGenerateMultipleFields()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    
    QJsonObject ageField;
    ageField["type"] = "integer";
    ageField["title"] = "Age";
    properties["age"] = ageField;
    
    QJsonObject emailField;
    emailField["type"] = "string";
    emailField["title"] = "Email";
    properties["email"] = emailField;
    
    schema["properties"] = properties;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 3);
    
    delete formWidget;
}

void TestFormSchemaParser::testRequiredFieldValidation()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    nameField["required"] = true;
    properties["name"] = nameField;
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    schema["required"] = required;
    
    QJsonDocument doc(schema);
    FormSchemaParser parser;
    QWidget* formWidget = parser.parseSchema(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 1);
    
    // Required field should be marked (e.g., with asterisk or different styling)
    // This will be verified in FormValidator tests
    
    delete formWidget;
}

QTEST_MAIN(TestFormSchemaParser)
#include "test_formschemaparser.moc"

