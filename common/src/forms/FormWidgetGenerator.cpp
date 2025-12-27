#include "smartbook/common/forms/FormWidgetGenerator.h"
#include "smartbook/common/forms/FormSchemaParser.h"
#include <QDebug>

namespace smartbook {
namespace common {
namespace forms {

FormWidgetGenerator::FormWidgetGenerator()
    : m_parser(new FormSchemaParser())
{
}

FormWidgetGenerator::~FormWidgetGenerator()
{
    delete m_parser;
}

QWidget* FormWidgetGenerator::generateForm(const QString& jsonSchema, const QString& stylesheet)
{
    m_lastError.clear();
    
    // Parse schema and create form widget
    QWidget* formWidget = m_parser->parseSchema(jsonSchema);
    if (!formWidget) {
        m_lastError = m_parser->lastError();
        return nullptr;
    }
    
    // Apply stylesheet if provided
    if (!stylesheet.isEmpty()) {
        formWidget->setStyleSheet(stylesheet);
    }
    
    return formWidget;
}

} // namespace forms
} // namespace common
} // namespace smartbook

