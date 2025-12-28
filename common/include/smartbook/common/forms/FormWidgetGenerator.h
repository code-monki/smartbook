#ifndef SMARTBOOK_COMMON_FORMS_FORMWIDGETGENERATOR_H
#define SMARTBOOK_COMMON_FORMS_FORMWIDGETGENERATOR_H

#include <QWidget>
#include <QString>

namespace smartbook {
namespace common {
namespace forms {

class FormSchemaParser;

/**
 * @brief Generator for complete form widgets from JSON schemas
 * 
 * FormWidgetGenerator creates complete, ready-to-use form widgets from JSON schemas.
 * It uses FormSchemaParser internally to parse schemas and create form layouts,
 * then wraps the layout in a container widget with basic styling.
 * 
 * @section Architecture
 * 
 * **Form Generation:**
 * 1. Uses FormSchemaParser to parse JSON schema and create QFormLayout
 * 2. Wraps layout in a QWidget container
 * 3. Applies basic styling (optional stylesheet)
 * 4. Returns complete form widget ready for use
 * 
 * **Styling:**
 * - Basic default styling applied (background, border, padding)
 * - Optional custom stylesheet can be provided
 * - Styling uses Qt Widgets stylesheets (not CSS)
 * 
 * @section Usage
 * 
 * @code
 * FormWidgetGenerator generator;
 * QWidget* formWidget = generator.generateForm(jsonSchema);
 * if (formWidget) {
 *     // Form widget is ready to use
 *     layout->addWidget(formWidget);
 * } else {
 *     QString error = generator.lastError();
 *     // Handle error
 * }
 * @endcode
 * 
 * @section Integration
 * 
 * FormWidgetGenerator is typically used by:
 * - FormEmbeddedWidget - Creates forms for embedding in content pages
 * - FormBuilder (Creator Tool) - Preview forms during authoring
 * 
 * @note This class is a convenience wrapper around FormSchemaParser.
 * For more control, use FormSchemaParser directly.
 * 
 * @see FormSchemaParser
 * @see FormEmbeddedWidget
 * @see qt-widgets-forms-guide.adoc
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

