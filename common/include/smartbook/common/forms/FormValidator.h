#ifndef SMARTBOOK_COMMON_FORMS_FORMVALIDATOR_H
#define SMARTBOOK_COMMON_FORMS_FORMVALIDATOR_H

#include <QString>
#include <QStringList>
#include <QJsonValue>
#include <QJsonObject>

namespace smartbook {
namespace common {
namespace forms {

/**
 * @brief Validator for form data against JSON schemas
 * 
 * Validates form data against schema rules including:
 * - Required field validation
 * - Type validation
 * - Range validation (min/max)
 * - Pattern validation (regex)
 */
class FormValidator {
public:
    FormValidator();
    ~FormValidator();
    
    /**
     * @brief Validate form data against schema
     * @param dataJson JSON string of form data
     * @param schemaJson JSON string of form schema
     * @return true if valid, false otherwise
     */
    bool validate(const QString& dataJson, const QString& schemaJson);
    
    /**
     * @brief Get validation errors
     * @return List of error messages
     */
    QStringList errors() const { return m_errors; }
    
    /**
     * @brief Clear validation errors
     */
    void clearErrors() { m_errors.clear(); }
    
    /**
     * @brief Get last error message
     * @return Error message string, empty if no error
     */
    QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
    QStringList m_errors;
    
    /**
     * @brief Validate a single field
     * @param fieldName Field name
     * @param fieldValue Field value (JSON value)
     * @param fieldSchema Field schema (JSON object)
     * @param isRequired Whether field is required
     * @return true if valid, false otherwise
     */
    bool validateField(const QString& fieldName, const QJsonValue& fieldValue,
                       const QJsonObject& fieldSchema, bool isRequired);
    
    /**
     * @brief Validate required field
     */
    bool validateRequired(const QString& fieldName, const QJsonValue& fieldValue);
    
    /**
     * @brief Validate field type
     */
    bool validateType(const QString& fieldName, const QJsonValue& fieldValue,
                      const QString& expectedType);
    
    /**
     * @brief Validate range (min/max)
     */
    bool validateRange(const QString& fieldName, const QJsonValue& fieldValue,
                      const QJsonObject& fieldSchema);
    
    /**
     * @brief Validate pattern (regex)
     */
    bool validatePattern(const QString& fieldName, const QString& fieldValue,
                         const QString& pattern);
    
    /**
     * @brief Add error message
     */
    void addError(const QString& fieldName, const QString& message);
};

} // namespace forms
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_FORMS_FORMVALIDATOR_H

