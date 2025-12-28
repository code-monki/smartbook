/**
 * @file FormSchemaParser.cpp
 * @brief Implementation of FormSchemaParser for parsing JSON form schemas
 * 
 * This file implements the FormSchemaParser class, which parses JSON Schema format
 * (Draft 7 compatible) and creates Qt Widget form layouts dynamically.
 * 
 * @section Schema Parsing Process
 * 
 * 1. Parse JSON schema string
 * 2. Validate schema structure (must be type "object")
 * 3. Extract properties and required fields
 * 4. For each property:
 *    - Determine field type and format
 *    - Create appropriate Qt Widget
 *    - Set default values and placeholders
 *    - Set objectName to field name (critical for FormDataSerializer)
 *    - Add to QFormLayout
 * 
 * @section Widget Creation
 * 
 * Widgets are created based on field type and format:
 * - string → QLineEdit (or QTextEdit if format="textarea")
 * - string + format="select" → QComboBox
 * - string + format="radio" → QRadioButton group
 * - string + format="date" → QDateEdit
 * - boolean → QCheckBox
 * - integer → QSpinBox
 * - number → QDoubleSpinBox
 * 
 * @section Widget Naming
 * 
 * All generated widgets have their objectName set to the field name from the schema.
 * This is critical for FormDataSerializer to correctly identify and interact with widgets.
 * 
 * @see FormSchemaParser.h
 * @see FormWidgetGenerator
 * @see FormDataSerializer
 * @see qt-widgets-forms-guide.adoc
 */

#include "smartbook/common/forms/FormSchemaParser.h"
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace smartbook {
namespace common {
namespace forms {

// ============================================================================
// Constructor and Destructor
// ============================================================================

FormSchemaParser::FormSchemaParser()
{
}

FormSchemaParser::~FormSchemaParser()
{
}

// ============================================================================
// Public Methods
// ============================================================================

QWidget* FormSchemaParser::parseSchema(const QString& jsonSchema)
{
    /**
     * @brief Parse JSON schema and create form widget
     * 
     * Parses JSON Schema format and creates a QWidget with QFormLayout
     * containing form fields for each property in the schema.
     * 
     * @param jsonSchema JSON string containing form schema
     * @return QWidget* with QFormLayout, or nullptr on error
     */
    
    m_lastError.clear();
    
    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonSchema.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "JSON parse error: " + parseError.errorString();
        return nullptr;
    }
    
    if (!doc.isObject()) {
        m_lastError = "Schema must be a JSON object";
        return nullptr;
    }
    
    QJsonObject schema = doc.object();
    
    // Check schema type
    if (schema["type"].toString() != "object") {
        m_lastError = "Schema type must be 'object'";
        return nullptr;
    }
    
    // Create form widget with layout
    QWidget* formWidget = new QWidget();
    QFormLayout* layout = new QFormLayout(formWidget);
    formWidget->setLayout(layout);
    
    // Get properties
    QJsonObject properties = schema["properties"].toObject();
    
    // Get required fields
    QJsonArray required = schema["required"].toArray();
    QSet<QString> requiredFields;
    for (const QJsonValue& value : required) {
        requiredFields.insert(value.toString());
    }
    
    // Create widgets for each property
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        QString fieldName = it.key();
        QJsonObject fieldSchema = it.value().toObject();
        
        // Check if required
        bool isRequired = requiredFields.contains(fieldName) || 
                         fieldSchema["required"].toBool(false);
        
        // Get field title
        QString title = fieldSchema["title"].toString(fieldName);
        if (isRequired) {
            title += " *";
        }
        
        // Create field widget
        QWidget* fieldWidget = createFieldWidget(fieldSchema);
        if (!fieldWidget) {
            delete formWidget;
            return nullptr;
        }
        
        // Set objectName for FormDataSerializer
        fieldWidget->setObjectName(fieldName);
        
        // Add to layout
        layout->addRow(title, fieldWidget);
    }
    
    return formWidget;
}

// ============================================================================
// Private Methods - Widget Creation
// ============================================================================

QWidget* FormSchemaParser::createFieldWidget(const QJsonObject& fieldSchema)
{
    /**
     * @brief Create widget for a field based on its type
     * 
     * Determines field type and format, then creates appropriate Qt Widget.
     * 
     * @param fieldSchema JSON object for the field
     * @return QWidget* for the field, or nullptr on error
     */
    QString type = fieldSchema["type"].toString();
    QString format = fieldSchema["format"].toString();
    
    if (type == "string") {
        if (format == "textarea") {
            return createTextarea(fieldSchema);
        } else if (format == "select") {
            return createSelect(fieldSchema);
        } else if (format == "radio") {
            return createRadioButtons(fieldSchema);
        } else if (format == "date") {
            return createDateInput(fieldSchema);
        } else {
            return createTextInput(fieldSchema);
        }
    } else if (type == "boolean") {
        return createCheckbox(fieldSchema);
    } else if (type == "integer" || type == "number") {
        return createNumberInput(fieldSchema);
    }
    
    m_lastError = "Unknown field type: " + type;
    return nullptr;
}

QWidget* FormSchemaParser::createTextInput(const QJsonObject& fieldSchema)
{
    QLineEdit* lineEdit = new QLineEdit();
    
    // Set default value
    if (fieldSchema.contains("defaultValue")) {
        lineEdit->setText(fieldSchema["defaultValue"].toString());
    }
    
    // Set placeholder
    if (fieldSchema.contains("placeholder")) {
        lineEdit->setPlaceholderText(fieldSchema["placeholder"].toString());
    }
    
    return lineEdit;
}

QWidget* FormSchemaParser::createTextarea(const QJsonObject& fieldSchema)
{
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setMaximumHeight(150);
    
    // Set default value
    if (fieldSchema.contains("defaultValue")) {
        textEdit->setPlainText(fieldSchema["defaultValue"].toString());
    }
    
    // Set placeholder (QTextEdit doesn't support placeholder, but we can set initial text)
    
    return textEdit;
}

QWidget* FormSchemaParser::createSelect(const QJsonObject& fieldSchema)
{
    QComboBox* comboBox = new QComboBox();
    
    // Get enum values
    QJsonArray enumValues = fieldSchema["enum"].toArray();
    for (const QJsonValue& value : enumValues) {
        comboBox->addItem(value.toString());
    }
    
    // Set default value
    if (fieldSchema.contains("defaultValue")) {
        int index = comboBox->findText(fieldSchema["defaultValue"].toString());
        if (index >= 0) {
            comboBox->setCurrentIndex(index);
        }
    }
    
    return comboBox;
}

QWidget* FormSchemaParser::createCheckbox(const QJsonObject& fieldSchema)
{
    QCheckBox* checkBox = new QCheckBox();
    
    // Set default value
    if (fieldSchema.contains("defaultValue")) {
        checkBox->setChecked(fieldSchema["defaultValue"].toBool(false));
    }
    
    return checkBox;
}

QWidget* FormSchemaParser::createRadioButtons(const QJsonObject& fieldSchema)
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QButtonGroup* buttonGroup = new QButtonGroup(container);
    
    // Get enum values
    QJsonArray enumValues = fieldSchema["enum"].toArray();
    QString defaultValue = fieldSchema["defaultValue"].toString();
    
    for (const QJsonValue& value : enumValues) {
        QString optionText = value.toString();
        QRadioButton* radio = new QRadioButton(optionText, container);
        buttonGroup->addButton(radio);
        layout->addWidget(radio);
        
        if (optionText == defaultValue) {
            radio->setChecked(true);
        }
    }
    
    return container;
}

QWidget* FormSchemaParser::createNumberInput(const QJsonObject& fieldSchema)
{
    QString type = fieldSchema["type"].toString();
    
    if (type == "integer") {
        QSpinBox* spinBox = new QSpinBox();
        
        // Set range
        if (fieldSchema.contains("minimum")) {
            spinBox->setMinimum(fieldSchema["minimum"].toInt());
        }
        if (fieldSchema.contains("maximum")) {
            spinBox->setMaximum(fieldSchema["maximum"].toInt());
        }
        
        // Set default value
        if (fieldSchema.contains("defaultValue")) {
            spinBox->setValue(fieldSchema["defaultValue"].toInt());
        }
        
        return spinBox;
    } else {
        // number (double/float)
        QDoubleSpinBox* spinBox = new QDoubleSpinBox();
        
        // Set range
        if (fieldSchema.contains("minimum")) {
            spinBox->setMinimum(fieldSchema["minimum"].toDouble());
        }
        if (fieldSchema.contains("maximum")) {
            spinBox->setMaximum(fieldSchema["maximum"].toDouble());
        }
        
        // Set default value
        if (fieldSchema.contains("defaultValue")) {
            spinBox->setValue(fieldSchema["defaultValue"].toDouble());
        }
        
        return spinBox;
    }
}

QWidget* FormSchemaParser::createDateInput(const QJsonObject& fieldSchema)
{
    QDateEdit* dateEdit = new QDateEdit();
    dateEdit->setCalendarPopup(true);
    dateEdit->setDate(QDate::currentDate());
    
    // Set default value
    if (fieldSchema.contains("defaultValue")) {
        QString dateStr = fieldSchema["defaultValue"].toString();
        QDate date = QDate::fromString(dateStr, Qt::ISODate);
        if (date.isValid()) {
            dateEdit->setDate(date);
        }
    }
    
    return dateEdit;
}

} // namespace forms
} // namespace common
} // namespace smartbook

