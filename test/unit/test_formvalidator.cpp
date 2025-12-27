/**
 * @file test_formvalidator.cpp
 * @brief Unit tests for FormValidator
 * 
 * Tests for validating form data against schema rules.
 * 
 * Test Cases:
 * T-FORM-09: Required field validation
 * T-FORM-10: Range validation
 * T-FORM-11: Pattern validation
 */

#include <QtTest>
#include "smartbook/common/forms/FormValidator.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

using namespace smartbook::common::forms;

class TestFormValidator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    
    // T-FORM-09: Required field validation
    void testRequiredFieldValidation();
    void testRequiredFieldMissing();
    void testRequiredFieldEmpty();
    
    // T-FORM-10: Range validation
    void testRangeValidationInteger();
    void testRangeValidationNumber();
    void testRangeValidationOutOfRange();
    
    // T-FORM-11: Pattern validation
    void testPatternValidation();
    void testPatternValidationFailure();
    
    // Additional tests
    void testTypeValidation();
    void testMultipleValidations();
    void testValidationErrorMessages();
};

void TestFormValidator::initTestCase()
{
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
}

// T-FORM-09: Required field validation
void TestFormValidator::testRequiredFieldValidation()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    schema["required"] = required;
    
    QJsonObject data;
    data["name"] = "Test Name";
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(isValid);
    QVERIFY(validator.errors().isEmpty());
}

void TestFormValidator::testRequiredFieldMissing()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    schema["required"] = required;
    
    QJsonObject data; // Missing "name" field
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(!isValid);
    QVERIFY(!validator.errors().isEmpty());
    // Error format is "name: Field is required"
    bool foundNameError = false;
    for (const QString& error : validator.errors()) {
        if (error.contains("name") && error.contains("required")) {
            foundNameError = true;
            break;
        }
    }
    QVERIFY(foundNameError);
}

void TestFormValidator::testRequiredFieldEmpty()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    schema["required"] = required;
    
    QJsonObject data;
    data["name"] = ""; // Empty string
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(!isValid);
    QVERIFY(!validator.errors().isEmpty());
}

// T-FORM-10: Range validation
void TestFormValidator::testRangeValidationInteger()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject ageField;
    ageField["type"] = "integer";
    ageField["title"] = "Age";
    ageField["minimum"] = 0;
    ageField["maximum"] = 120;
    properties["age"] = ageField;
    schema["properties"] = properties;
    
    QJsonObject data;
    data["age"] = 25;
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(isValid);
}

void TestFormValidator::testRangeValidationNumber()
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
    
    QJsonObject data;
    data["score"] = 85.5;
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(isValid);
}

void TestFormValidator::testRangeValidationOutOfRange()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject ageField;
    ageField["type"] = "integer";
    ageField["title"] = "Age";
    ageField["minimum"] = 0;
    ageField["maximum"] = 120;
    properties["age"] = ageField;
    schema["properties"] = properties;
    
    QJsonObject data;
    data["age"] = 150; // Out of range
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(!isValid);
    QVERIFY(!validator.errors().isEmpty());
}

// T-FORM-11: Pattern validation
void TestFormValidator::testPatternValidation()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject emailField;
    emailField["type"] = "string";
    emailField["title"] = "Email";
    emailField["pattern"] = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    properties["email"] = emailField;
    schema["properties"] = properties;
    
    QJsonObject data;
    data["email"] = "test@example.com";
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(isValid);
}

void TestFormValidator::testPatternValidationFailure()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject emailField;
    emailField["type"] = "string";
    emailField["title"] = "Email";
    emailField["pattern"] = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    properties["email"] = emailField;
    schema["properties"] = properties;
    
    QJsonObject data;
    data["email"] = "invalid-email"; // Invalid format
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(!isValid);
    QVERIFY(!validator.errors().isEmpty());
}

// Additional tests
void TestFormValidator::testTypeValidation()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject ageField;
    ageField["type"] = "integer";
    ageField["title"] = "Age";
    properties["age"] = ageField;
    schema["properties"] = properties;
    
    QJsonObject data;
    data["age"] = "not a number"; // Wrong type
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(!isValid);
}

void TestFormValidator::testMultipleValidations()
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
    ageField["minimum"] = 0;
    ageField["maximum"] = 120;
    properties["age"] = ageField;
    
    QJsonObject emailField;
    emailField["type"] = "string";
    emailField["title"] = "Email";
    emailField["pattern"] = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    properties["email"] = emailField;
    
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    required.append("email");
    schema["required"] = required;
    
    QJsonObject data;
    data["name"] = "Test";
    data["age"] = 25;
    data["email"] = "test@example.com";
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(isValid);
}

void TestFormValidator::testValidationErrorMessages()
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Name";
    properties["name"] = nameField;
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    schema["required"] = required;
    
    QJsonObject data; // Missing name
    
    FormValidator validator;
    QJsonDocument schemaDoc(schema);
    QJsonDocument dataDoc(data);
    
    bool isValid = validator.validate(dataDoc.toJson(QJsonDocument::Compact),
                                      schemaDoc.toJson(QJsonDocument::Compact));
    
    QVERIFY(!isValid);
    QStringList errors = validator.errors();
    QVERIFY(!errors.isEmpty());
    
    // Error messages should be human-readable
    for (const QString& error : errors) {
        QVERIFY(!error.isEmpty());
    }
}

QTEST_MAIN(TestFormValidator)
#include "test_formvalidator.moc"

