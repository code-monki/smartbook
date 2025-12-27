#ifndef SMARTBOOK_COMMON_FORMS_FORMDATASERIALIZER_H
#define SMARTBOOK_COMMON_FORMS_FORMDATASERIALIZER_H

#include <QString>
#include <QWidget>
#include <QJsonValue>

namespace smartbook {
namespace common {
namespace forms {

/**
 * @brief Serializer for form data to/from JSON
 * 
 * Extracts form data from Qt Widgets to JSON and loads form data from JSON to Qt Widgets.
 * Handles all field types: text, textarea, select, checkbox, radio, number, date.
 */
class FormDataSerializer {
public:
    FormDataSerializer();
    ~FormDataSerializer();
    
    /**
     * @brief Serialize form data from widgets to JSON
     * @param formWidget Form widget containing form fields
     * @return JSON string of form data
     */
    QString serializeFormData(QWidget* formWidget);
    
    /**
     * @brief Load form data from JSON to widgets
     * @param formWidget Form widget containing form fields
     * @param jsonData JSON string of form data
     * @return true if successful, false otherwise
     */
    bool loadFormData(QWidget* formWidget, const QString& jsonData);
    
    /**
     * @brief Get last error message
     * @return Error message string, empty if no error
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
    
    /**
     * @brief Extract value from a widget
     * @param widget Widget to extract value from
     * @return JSON value
     */
    QJsonValue extractWidgetValue(QWidget* widget);
    
    /**
     * @brief Set value on a widget
     * @param widget Widget to set value on
     * @param value JSON value to set
     * @return true if successful, false otherwise
     */
    bool setWidgetValue(QWidget* widget, const QJsonValue& value);
};

} // namespace forms
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_FORMS_FORMDATASERIALIZER_H

