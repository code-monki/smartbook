#ifndef SMARTBOOK_COMMON_FORMS_FORMSCHEMAPARSER_H
#define SMARTBOOK_COMMON_FORMS_FORMSCHEMAPARSER_H

#include <QWidget>
#include <QString>

namespace smartbook {
namespace common {
namespace forms {

/**
 * @brief Parser for JSON form schemas
 * 
 * FormSchemaParser parses JSON Schema format (Draft 7 compatible) and creates
 * Qt Widget form layouts dynamically. It supports standard form field types and
 * generates appropriate Qt Widgets for each field type.
 * 
 * @section Schema Format
 * 
 * Forms use JSON Schema format:
 * @code
 * {
 *   "type": "object",
 *   "title": "Form Title",
 *   "properties": {
 *     "fieldName": {
 *       "type": "string",
 *       "title": "Field Label",
 *       "defaultValue": "default",
 *       "placeholder": "Enter value"
 *     }
 *   },
 *   "required": ["fieldName"]
 * }
 * @endcode
 * 
 * @section Supported Field Types
 * 
 * **Text Input (`type: "string"`):**
 * - Creates QLineEdit widget
 * - Supports: defaultValue, placeholder, minLength, maxLength, pattern
 * 
 * **Textarea (`type: "string", format: "textarea"`):**
 * - Creates QTextEdit widget
 * - Supports: defaultValue, placeholder, minLength, maxLength
 * 
 * **Select/Dropdown (`type: "string", format: "select"`):**
 * - Creates QComboBox widget
 * - Requires: enum (array of values), enumNames (array of labels)
 * 
 * **Checkbox (`type: "boolean"`):**
 * - Creates QCheckBox widget
 * - Supports: defaultValue (true/false)
 * 
 * **Radio Buttons (`type: "string", format: "radio"`):**
 * - Creates QRadioButton widgets with QButtonGroup
 * - Requires: enum (array of values), enumNames (array of labels)
 * 
 * **Number (`type: "number"` or `type: "integer"`):**
 * - Creates QDoubleSpinBox (number) or QSpinBox (integer)
 * - Supports: defaultValue, minimum, maximum
 * 
 * **Date (`type: "string", format: "date"`):**
 * - Creates QDateEdit widget
 * - Supports: defaultValue (ISO date string)
 * 
 * @section Widget Naming
 * 
 * All generated widgets have their `objectName` set to the field name from the schema.
 * This is crucial for FormDataSerializer to correctly identify and interact with widgets.
 * 
 * @section Usage
 * 
 * @code
 * FormSchemaParser parser;
 * QWidget* formWidget = parser.parseSchema(jsonSchema);
 * if (formWidget) {
 *     // Form widget is ready to use
 *     // Add to layout, show, etc.
 * } else {
 *     QString error = parser.lastError();
 *     // Handle error
 * }
 * @endcode
 * 
 * @section Error Handling
 * 
 * If schema parsing fails:
 * - `parseSchema()` returns nullptr
 * - `lastError()` contains error message
 * - Common errors: invalid JSON, missing required fields, unknown field types
 * 
 * @note This class is stateless (except for lastError). Multiple instances
 * can be used concurrently without issues.
 * 
 * @see FormWidgetGenerator
 * @see FormDataSerializer
 * @see FormValidator
 * @see qt-widgets-forms-guide.adoc
 */
class FormSchemaParser {
public:
    FormSchemaParser();
    ~FormSchemaParser();
    
    /**
     * @brief Parse JSON schema and create form widget
     * @param jsonSchema JSON string containing form schema
     * @return QWidget* with QFormLayout, or nullptr on error
     */
    QWidget* parseSchema(const QString& jsonSchema);
    
    /**
     * @brief Get last error message
     * @return Error message string, empty if no error
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
    
    /**
     * @brief Create widget for a field based on its type
     * @param fieldSchema JSON object for the field
     * @return QWidget* for the field, or nullptr on error
     */
    QWidget* createFieldWidget(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create text input widget
     */
    QWidget* createTextInput(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create textarea widget
     */
    QWidget* createTextarea(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create select/combobox widget
     */
    QWidget* createSelect(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create checkbox widget
     */
    QWidget* createCheckbox(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create radio button group widget
     */
    QWidget* createRadioButtons(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create number input widget
     */
    QWidget* createNumberInput(const QJsonObject& fieldSchema);
    
    /**
     * @brief Create date input widget
     */
    QWidget* createDateInput(const QJsonObject& fieldSchema);
};

} // namespace forms
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_FORMS_FORMSCHEMAPARSER_H

