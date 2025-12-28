/**
 * @file FormValidator.cpp
 * @brief Implementation of FormValidator for validating form data
 * 
 * This file implements the FormValidator class, which validates form data (as JSON)
 * against a JSON Schema definition. It performs comprehensive validation including
 * required fields, types, ranges, and patterns.
 * 
 * @section Validation Process
 * 
 * 1. Parse data JSON (validate JSON format)
 * 2. Parse schema JSON (validate JSON format)
 * 3. Build required fields set from schema "required" array
 * 4. For each property in schema:
 *    - Check if required (if yes, validate presence)
 *    - Validate type (if specified)
 *    - Validate range (if min/max specified)
 *    - Validate pattern (if pattern specified)
 * 5. Return true if all validations pass, false otherwise
 * 
 * @section Validation Rules
 * 
 * **Required Fields:**
 * - Field must be present in data
 * - Field value must not be null, undefined, or empty string
 * 
 * **Type Validation:**
 * - string: Value must be a string
 * - integer: Value must be a whole number
 * - number: Value must be numeric
 * - boolean: Value must be a boolean
 * 
 * **Range Validation:**
 * - Checks minimum and maximum constraints for numeric values
 * - Validates: value >= minimum && value <= maximum
 * 
 * **Pattern Validation:**
 * - Uses QRegularExpression for regex pattern matching
 * - Only applies to string values
 * 
 * @section Error Collection
 * 
 * All validation errors are collected in errors() list. Each error is a
 * formatted string: "fieldName: error message". The lastError() method
 * returns the most recent error.
 * 
 * @see FormValidator.h
 * @see FormSchemaParser
 * @see FormDataSerializer
 * @see qt-widgets-forms-guide.adoc
 */

#include "smartbook/common/forms/FormValidator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QDebug>

namespace smartbook {
namespace common {
namespace forms {

// ============================================================================
// Constructor and Destructor
// ============================================================================

FormValidator::FormValidator()
{
}

FormValidator::~FormValidator()
{
}

// ============================================================================
// Public Methods
// ============================================================================

bool FormValidator::validate(const QString& dataJson, const QString& schemaJson)
{
    /**
     * @brief Validate form data against schema
     * 
     * Validates form data (as JSON) against a JSON Schema definition.
     * Performs all validation checks and collects errors.
     * 
     * @param dataJson JSON string of form data
     * @param schemaJson JSON string of form schema
     * @return true if valid, false otherwise
     */
    
    m_lastError.clear();
    m_errors.clear();
    
    // Parse data JSON
    QJsonParseError parseError;
    QJsonDocument dataDoc = QJsonDocument::fromJson(dataJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "Data JSON parse error: " + parseError.errorString();
        return false;
    }
    
    if (!dataDoc.isObject()) {
        m_lastError = "Data must be a JSON object";
        return false;
    }
    
    QJsonObject data = dataDoc.object();
    
    // Parse schema JSON
    QJsonDocument schemaDoc = QJsonDocument::fromJson(schemaJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = "Schema JSON parse error: " + parseError.errorString();
        return false;
    }
    
    if (!schemaDoc.isObject()) {
        m_lastError = "Schema must be a JSON object";
        return false;
    }
    
    QJsonObject schema = schemaDoc.object();
    
    // Get required fields
    QJsonArray required = schema["required"].toArray();
    QSet<QString> requiredFields;
    for (const QJsonValue& value : required) {
        requiredFields.insert(value.toString());
    }
    
    // Get properties
    QJsonObject properties = schema["properties"].toObject();
    
    // Validate each field in schema
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        QString fieldName = it.key();
        QJsonObject fieldSchema = it.value().toObject();
        
        bool isRequired = requiredFields.contains(fieldName) ||
                         fieldSchema["required"].toBool(false);
        
        QJsonValue fieldValue = data[fieldName];
        
        if (!validateField(fieldName, fieldValue, fieldSchema, isRequired)) {
            return false;
        }
    }
    
    return m_errors.isEmpty();
}

bool FormValidator::validateField(const QString& fieldName, const QJsonValue& fieldValue,
                                    const QJsonObject& fieldSchema, bool isRequired)
{
    // Check required
    if (isRequired && !validateRequired(fieldName, fieldValue)) {
        return false;
    }
    
    // If field is not required and not present, skip other validations
    if (fieldValue.isNull() || fieldValue.isUndefined()) {
        return true;
    }
    
    // Validate type
    QString expectedType = fieldSchema["type"].toString();
    if (!validateType(fieldName, fieldValue, expectedType)) {
        return false;
    }
    
    // Validate range (for numbers)
    if (expectedType == "integer" || expectedType == "number") {
        if (!validateRange(fieldName, fieldValue, fieldSchema)) {
            return false;
        }
    }
    
    // Validate pattern (for strings)
    if (expectedType == "string") {
        QString pattern = fieldSchema["pattern"].toString();
        if (!pattern.isEmpty()) {
            QString stringValue = fieldValue.toString();
            if (!validatePattern(fieldName, stringValue, pattern)) {
                return false;
            }
        }
    }
    
    return true;
}

bool FormValidator::validateRequired(const QString& fieldName, const QJsonValue& fieldValue)
{
    if (fieldValue.isNull() || fieldValue.isUndefined()) {
        addError(fieldName, "Field is required");
        return false;
    }
    
    // Check for empty string
    if (fieldValue.isString() && fieldValue.toString().isEmpty()) {
        addError(fieldName, "Field is required");
        return false;
    }
    
    return true;
}

bool FormValidator::validateType(const QString& fieldName, const QJsonValue& fieldValue,
                                  const QString& expectedType)
{
    if (expectedType == "string") {
        if (!fieldValue.isString()) {
            addError(fieldName, "Expected string type");
            return false;
        }
    } else if (expectedType == "integer") {
        if (!fieldValue.isDouble()) {
            addError(fieldName, "Expected integer type");
            return false;
        }
        // Check if it's actually an integer (no decimal part)
        double value = fieldValue.toDouble();
        if (value != static_cast<int>(value)) {
            addError(fieldName, "Expected integer type");
            return false;
        }
    } else if (expectedType == "number") {
        if (!fieldValue.isDouble()) {
            addError(fieldName, "Expected number type");
            return false;
        }
    } else if (expectedType == "boolean") {
        if (!fieldValue.isBool()) {
            addError(fieldName, "Expected boolean type");
            return false;
        }
    }
    
    return true;
}

bool FormValidator::validateRange(const QString& fieldName, const QJsonValue& fieldValue,
                                   const QJsonObject& fieldSchema)
{
    double value = fieldValue.toDouble();
    
    if (fieldSchema.contains("minimum")) {
        double minimum = fieldSchema["minimum"].toDouble();
        if (value < minimum) {
            addError(fieldName, QString("Value must be at least %1").arg(minimum));
            return false;
        }
    }
    
    if (fieldSchema.contains("maximum")) {
        double maximum = fieldSchema["maximum"].toDouble();
        if (value > maximum) {
            addError(fieldName, QString("Value must be at most %1").arg(maximum));
            return false;
        }
    }
    
    return true;
}

bool FormValidator::validatePattern(const QString& fieldName, const QString& fieldValue,
                                     const QString& pattern)
{
    QRegularExpression regex(pattern);
    if (!regex.isValid()) {
        addError(fieldName, "Invalid pattern in schema");
        return false;
    }
    
    QRegularExpressionMatch match = regex.match(fieldValue);
    if (!match.hasMatch()) {
        addError(fieldName, "Value does not match required pattern");
        return false;
    }
    
    return true;
}

void FormValidator::addError(const QString& fieldName, const QString& message)
{
    QString error = QString("%1: %2").arg(fieldName, message);
    m_errors.append(error);
}

} // namespace forms
} // namespace common
} // namespace smartbook

