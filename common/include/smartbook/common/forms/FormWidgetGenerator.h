#ifndef SMARTBOOK_COMMON_FORMS_FORMWIDGETGENERATOR_H
#define SMARTBOOK_COMMON_FORMS_FORMWIDGETGENERATOR_H

#include <QWidget>
#include <QString>

namespace smartbook {
namespace common {
namespace forms {

class FormSchemaParser;

/**
 * @brief Generator for form widgets from JSON schemas
 * 
 * Generates form widgets with styling and validation support.
 * Uses FormSchemaParser internally to parse schemas.
 */
class FormWidgetGenerator {
public:
    FormWidgetGenerator();
    ~FormWidgetGenerator();
    
    /**
     * @brief Generate form widget from JSON schema
     * @param jsonSchema JSON string containing form schema
     * @param stylesheet Optional CSS-like stylesheet for form widgets
     * @return QWidget* with form layout, or nullptr on error
     */
    QWidget* generateForm(const QString& jsonSchema, const QString& stylesheet = QString());
    
    /**
     * @brief Get last error message
     * @return Error message string, empty if no error
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
    FormSchemaParser* m_parser;
};

} // namespace forms
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_FORMS_FORMWIDGETGENERATOR_H

