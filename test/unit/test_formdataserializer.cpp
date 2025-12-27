/**
 * @file test_formdataserializer.cpp
 * @brief Unit tests for FormDataSerializer
 * 
 * Tests for serializing/deserializing form data to/from JSON.
 * 
 * Test Cases:
 * T-FORM-12: Form data serialization
 * T-FORM-13: Form data loading
 */

#include <QtTest>
#include "smartbook/common/forms/FormDataSerializer.h"
#include <QApplication>
#include <QWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

using namespace smartbook::common::forms;

class TestFormDataSerializer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    
    // T-FORM-12: Form data serialization
    void testSerializeFormData();
    void testSerializeFormDataAllTypes();
    void testSerializeFormDataEmpty();
    
    // T-FORM-13: Form data loading
    void testLoadFormData();
    void testLoadFormDataAllTypes();
    void testLoadFormDataPartial();
};

void TestFormDataSerializer::initTestCase()
{
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
}

// T-FORM-12: Form data serialization
void TestFormDataSerializer::testSerializeFormData()
{
    // Create a simple form widget
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    
    QLineEdit* nameEdit = new QLineEdit("Test Name");
    layout->addRow("Name", nameEdit);
    nameEdit->setObjectName("name");
    
    QSpinBox* ageSpin = new QSpinBox();
    ageSpin->setValue(25);
    layout->addRow("Age", ageSpin);
    ageSpin->setObjectName("age");
    
    FormDataSerializer serializer;
    QString jsonData = serializer.serializeFormData(formWidget);
    
    QVERIFY(!jsonData.isEmpty());
    
    // Parse and verify JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
    QVERIFY(parseError.error == QJsonParseError::NoError);
    QVERIFY(doc.isObject());
    
    QJsonObject data = doc.object();
    QCOMPARE(data["name"].toString(), "Test Name");
    QCOMPARE(data["age"].toInt(), 25);
    
    delete formWidget;
}

void TestFormDataSerializer::testSerializeFormDataAllTypes()
{
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    
    QLineEdit* textEdit = new QLineEdit("Text");
    textEdit->setObjectName("text");
    layout->addRow("Text", textEdit);
    
    QTextEdit* textareaEdit = new QTextEdit("Textarea content");
    textareaEdit->setObjectName("textarea");
    layout->addRow("Textarea", textareaEdit);
    
    QComboBox* comboBox = new QComboBox();
    comboBox->addItems({"Option 1", "Option 2", "Option 3"});
    comboBox->setCurrentIndex(1);
    comboBox->setObjectName("select");
    layout->addRow("Select", comboBox);
    
    QCheckBox* checkBox = new QCheckBox();
    checkBox->setChecked(true);
    checkBox->setObjectName("checkbox");
    layout->addRow("Checkbox", checkBox);
    
    QSpinBox* spinBox = new QSpinBox();
    spinBox->setValue(42);
    spinBox->setObjectName("integer");
    layout->addRow("Integer", spinBox);
    
    QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox();
    doubleSpinBox->setValue(3.14);
    doubleSpinBox->setObjectName("number");
    layout->addRow("Number", doubleSpinBox);
    
    QDateEdit* dateEdit = new QDateEdit();
    dateEdit->setDate(QDate(2024, 1, 15));
    dateEdit->setObjectName("date");
    layout->addRow("Date", dateEdit);
    
    FormDataSerializer serializer;
    QString jsonData = serializer.serializeFormData(formWidget);
    
    QVERIFY(!jsonData.isEmpty());
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
    QJsonObject data = doc.object();
    
    QCOMPARE(data["text"].toString(), "Text");
    QCOMPARE(data["textarea"].toString(), "Textarea content");
    QCOMPARE(data["select"].toString(), "Option 2");
    QCOMPARE(data["checkbox"].toBool(), true);
    QCOMPARE(data["integer"].toInt(), 42);
    QCOMPARE(data["number"].toDouble(), 3.14);
    QCOMPARE(data["date"].toString(), "2024-01-15");
    
    delete formWidget;
}

void TestFormDataSerializer::testSerializeFormDataEmpty()
{
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    
    QLineEdit* emptyEdit = new QLineEdit();
    emptyEdit->setObjectName("empty");
    layout->addRow("Empty", emptyEdit);
    
    FormDataSerializer serializer;
    QString jsonData = serializer.serializeFormData(formWidget);
    
    QVERIFY(!jsonData.isEmpty());
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
    QJsonObject data = doc.object();
    QCOMPARE(data["empty"].toString(), "");
    
    delete formWidget;
}

// T-FORM-13: Form data loading
void TestFormDataSerializer::testLoadFormData()
{
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    
    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setObjectName("name");
    layout->addRow("Name", nameEdit);
    
    QSpinBox* ageSpin = new QSpinBox();
    ageSpin->setObjectName("age");
    layout->addRow("Age", ageSpin);
    
    QJsonObject data;
    data["name"] = "Loaded Name";
    data["age"] = 30;
    
    QJsonDocument doc(data);
    FormDataSerializer serializer;
    bool success = serializer.loadFormData(formWidget, doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(success);
    QCOMPARE(nameEdit->text(), "Loaded Name");
    QCOMPARE(ageSpin->value(), 30);
    
    delete formWidget;
}

void TestFormDataSerializer::testLoadFormDataAllTypes()
{
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    
    QLineEdit* textEdit = new QLineEdit();
    textEdit->setObjectName("text");
    layout->addRow("Text", textEdit);
    
    QTextEdit* textareaEdit = new QTextEdit();
    textareaEdit->setObjectName("textarea");
    layout->addRow("Textarea", textareaEdit);
    
    QComboBox* comboBox = new QComboBox();
    comboBox->addItems({"Option 1", "Option 2", "Option 3"});
    comboBox->setObjectName("select");
    layout->addRow("Select", comboBox);
    
    QCheckBox* checkBox = new QCheckBox();
    checkBox->setObjectName("checkbox");
    layout->addRow("Checkbox", checkBox);
    
    QSpinBox* spinBox = new QSpinBox();
    spinBox->setRange(0, 1000);
    spinBox->setObjectName("integer");
    layout->addRow("Integer", spinBox);
    
    QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox();
    doubleSpinBox->setObjectName("number");
    layout->addRow("Number", doubleSpinBox);
    
    QDateEdit* dateEdit = new QDateEdit();
    dateEdit->setObjectName("date");
    layout->addRow("Date", dateEdit);
    
    QJsonObject data;
    data["text"] = "Loaded Text";
    data["textarea"] = "Loaded Textarea";
    data["select"] = "Option 3";
    data["checkbox"] = true;
    data["integer"] = 100;
    data["number"] = 2.71;
    data["date"] = "2024-12-25";
    
    QJsonDocument doc(data);
    FormDataSerializer serializer;
    bool success = serializer.loadFormData(formWidget, doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(success);
    QCOMPARE(textEdit->text(), "Loaded Text");
    QCOMPARE(textareaEdit->toPlainText(), "Loaded Textarea");
    QCOMPARE(comboBox->currentText(), "Option 3");
    QCOMPARE(checkBox->isChecked(), true);
    QCOMPARE(spinBox->value(), 100);
    QCOMPARE(doubleSpinBox->value(), 2.71);
    QCOMPARE(dateEdit->date(), QDate(2024, 12, 25));
    
    delete formWidget;
}

void TestFormDataSerializer::testLoadFormDataPartial()
{
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    
    QLineEdit* nameEdit = new QLineEdit("Original");
    nameEdit->setObjectName("name");
    layout->addRow("Name", nameEdit);
    
    QLineEdit* emailEdit = new QLineEdit("original@email.com");
    emailEdit->setObjectName("email");
    layout->addRow("Email", emailEdit);
    
    // Only load name, email should remain unchanged
    QJsonObject data;
    data["name"] = "Updated Name";
    
    QJsonDocument doc(data);
    FormDataSerializer serializer;
    bool success = serializer.loadFormData(formWidget, doc.toJson(QJsonDocument::Compact));
    
    QVERIFY(success);
    QCOMPARE(nameEdit->text(), "Updated Name");
    QCOMPARE(emailEdit->text(), "original@email.com"); // Unchanged
    
    delete formWidget;
}

QTEST_MAIN(TestFormDataSerializer)
#include "test_formdataserializer.moc"

