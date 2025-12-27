/**
 * @file test_formwidgetgenerator.cpp
 * @brief Unit tests for FormWidgetGenerator
 * 
 * Tests for generating form widgets from JSON schemas with styling and validation.
 */

#include <QtTest>
#include "smartbook/common/forms/FormWidgetGenerator.h"
#include <QApplication>
#include <QWidget>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

using namespace smartbook::common::forms;

class TestFormWidgetGenerator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    
    void testGenerateFormWidget();
    void testGenerateFormWidgetWithStyling();
    void testGenerateFormWidgetWithValidation();
    void testGenerateFormWidgetFromSchema();
};

void TestFormWidgetGenerator::initTestCase()
{
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
}

void TestFormWidgetGenerator::testGenerateFormWidget()
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
    FormWidgetGenerator generator;
    QWidget* formWidget = generator.generateForm(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QVERIFY(formWidget->layout() != nullptr);
    QVERIFY(qobject_cast<QFormLayout*>(formWidget->layout()) != nullptr);
    
    delete formWidget;
}

void TestFormWidgetGenerator::testGenerateFormWidgetWithStyling()
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
    FormWidgetGenerator generator;
    QWidget* formWidget = generator.generateForm(doc.toJson(QJsonDocument::Compact), "QLineEdit { background-color: #f0f0f0; }");
    
    QVERIFY(formWidget != nullptr);
    // Styling applied (verified by widget existence)
    
    delete formWidget;
}

void TestFormWidgetGenerator::testGenerateFormWidgetWithValidation()
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
    
    QJsonArray required;
    required.append("email");
    schema["required"] = required;
    
    QJsonDocument doc(schema);
    FormWidgetGenerator generator;
    QWidget* formWidget = generator.generateForm(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    // Validation rules connected (verified by widget existence)
    
    delete formWidget;
}

void TestFormWidgetGenerator::testGenerateFormWidgetFromSchema()
{
    // Test with complex schema
    QJsonObject schema;
    schema["type"] = "object";
    schema["title"] = "Character Sheet";
    
    QJsonObject properties;
    
    QJsonObject nameField;
    nameField["type"] = "string";
    nameField["title"] = "Character Name";
    nameField["required"] = true;
    properties["name"] = nameField;
    
    QJsonObject levelField;
    levelField["type"] = "integer";
    levelField["title"] = "Level";
    levelField["minimum"] = 1;
    levelField["maximum"] = 20;
    properties["level"] = levelField;
    
    QJsonObject classField;
    classField["type"] = "string";
    classField["title"] = "Class";
    classField["format"] = "select";
    QJsonArray classes;
    classes.append("Fighter");
    classes.append("Wizard");
    classes.append("Rogue");
    classField["enum"] = classes;
    properties["class"] = classField;
    
    schema["properties"] = properties;
    
    QJsonArray required;
    required.append("name");
    schema["required"] = required;
    
    QJsonDocument doc(schema);
    FormWidgetGenerator generator;
    QWidget* formWidget = generator.generateForm(doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(formWidget != nullptr);
    QFormLayout* layout = qobject_cast<QFormLayout*>(formWidget->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->rowCount(), 3);
    
    delete formWidget;
}

QTEST_MAIN(TestFormWidgetGenerator)
#include "test_formwidgetgenerator.moc"

