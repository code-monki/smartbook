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
 * FormValidator validates form data (as JSON) against a JSON Schema definition.
 * It performs comprehensive validation including required fields, types, ranges,
 * and patterns, providing detailed error messages for each validation failure.
 * 
 * @section Validation Rules
 * 
 * **Required Fields:**
 * - Checks that all fields listed in schema "required" array are present
 * - Validates that required fields are not null, undefined, or empty strings
 * - Provides error message: "Field is required"
 * 
 * **Type Validation:**
 * - Validates that field values match expected types (string, integer, number, boolean)
 * - For integers: checks that value is a whole number
 * - For numbers: checks that value is numeric
 * - Provides error message: "Expected {type} type"
 * 
 * **Range Validation:**
 * - Validates numeric values against minimum/maximum constraints
 * - Checks: `value >= minimum` and `value <= maximum`
 * - Provides error message: "Value must be at least/most {value}"
 * 
 * **Pattern Validation:**
 * - Validates string values against regex patterns
 * - Uses QRegularExpression for pattern matching
 * - Provides error message: "Value does not match required pattern"
 * 
 * @section Usage
 * 
 * @code
 * FormValidator validator;
 * bool isValid = validator.validate(dataJson, schemaJson);
 * if (!isValid) {
 *     QStringList errors = validator.errors();
 *     // Display errors to user
 *     for (const QString& error : errors) {
 *         qWarning() << error;
 *     }
 * }
 * @endcode
 * 
 * @section Error Collection
 * 
 * **Error Storage:**
 * - All validation errors are collected in `errors()` list
 * - Each error is a formatted string: "fieldName: error message"
 * - `lastError()` returns the most recent error
 * - Errors are cleared when `validate()` is called again
 * 
 * **Error Format:**
 * - Field-specific errors: "fieldName: error message"
 * - Schema-level errors: "error message" (no field name)
 * 
 * @section Validation Process
 * 
 * 1. Parse schema JSON (validate JSON format)
 * 2. Parse data JSON (validate JSON format)
 * 3. Build required fields set from schema "required" array
 * 4. For each property in schema:
 *    - Check if required (if yes, validate presence)
 *    - Validate type (if specified)
 *    - Validate range (if min/max specified)
 *    - Validate pattern (if pattern specified)
 * 5. Return true if all validations pass, false otherwise
 * 
 * @note This class is stateless (except for error storage). Multiple instances
 * can be used concurrently without issues.
 * 
 * @see FormSchemaParser
 * @see FormDataSerializer
 * @see FormEmbeddedWidget
 * @see qt-widgets-forms-guide.adoc
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

