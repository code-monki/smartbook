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
 * Parses JSON form schemas and creates Qt Widget form layouts.
 * Supports standard form field types: text, textarea, select, checkbox, radio, number, date.
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

