#include "smartbook/reader/ui/FormEmbeddedWidget.h"
#include "smartbook/common/forms/FormWidgetGenerator.h"
#include "smartbook/common/forms/FormDataSerializer.h"
#include "smartbook/common/forms/FormValidator.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace smartbook {
namespace reader {

FormEmbeddedWidget::FormEmbeddedWidget(const QString& formId, QWidget* parent)
    : QWidget(parent)
    , m_formId(formId)
    , m_isLoaded(false)
    , m_formGenerator(new common::forms::FormWidgetGenerator())
    , m_dataSerializer(new common::forms::FormDataSerializer())
    , m_validator(new common::forms::FormValidator())
    , m_formWidget(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
}

FormEmbeddedWidget::~FormEmbeddedWidget()
{
    delete m_formWidget;
    delete m_formGenerator;
    delete m_dataSerializer;
    delete m_validator;
}

bool FormEmbeddedWidget::loadForm(const QString& cartridgePath)
{
    m_cartridgePath = cartridgePath;
    m_isLoaded = false;
    m_errorMessage.clear();
    
    // Open cartridge database
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(cartridgePath)) {
        m_errorMessage = "Failed to open cartridge database";
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    // Query Form_Definitions table for form schema
    QSqlQuery query(connector.getDatabase());
    query.prepare("SELECT form_schema_json FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(m_formId);
    
    if (!query.exec()) {
        m_errorMessage = "Failed to query form schema: " + query.lastError().text();
        connector.closeCartridge();
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    if (!query.next()) {
        m_errorMessage = "Form not found: " + m_formId;
        connector.closeCartridge();
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    QString schemaJson = query.value(0).toString();
    connector.closeCartridge();
    
    if (schemaJson.isEmpty()) {
        m_errorMessage = "Form schema is empty for: " + m_formId;
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    // Generate form widget from schema
    m_formWidget = m_formGenerator->generateForm(schemaJson);
    if (!m_formWidget) {
        m_errorMessage = "Failed to generate form widget: " + m_formGenerator->lastError();
        emit formLoadError(m_errorMessage);
        return false;
    }
    
    // Add form widget to layout
    layout()->addWidget(m_formWidget);
    
    m_isLoaded = true;
    emit formLoaded();
    
    // Load existing form data if available
    loadFormData();
    
    return true;
}

bool FormEmbeddedWidget::saveFormData()
{
    if (!m_isLoaded || !m_formWidget) {
        return false;
    }
    
    if (m_cartridgePath.isEmpty()) {
        return false;
    }
    
    // Serialize form data
    QString dataJson = m_dataSerializer->serializeFormData(m_formWidget);
    if (dataJson.isEmpty()) {
        emit formDataSaved(false);
        return false;
    }
    
    // Validate form data against schema
    // Note: We need to get the schema again for validation
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        emit formDataSaved(false);
        return false;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare("SELECT form_schema_json FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(m_formId);
    
    if (!query.exec() || !query.next()) {
        connector.closeCartridge();
        emit formDataSaved(false);
        return false;
    }
    
    QString schemaJson = query.value(0).toString();
    connector.closeCartridge();
    
    // Validate
    if (!m_validator->validate(dataJson, schemaJson)) {
        qWarning() << "Form validation failed:" << m_validator->errors();
        emit formDataSaved(false);
        return false;
    }
    
    // Save to User_Data table
    if (!connector.openCartridge(m_cartridgePath)) {
        emit formDataSaved(false);
        return false;
    }
    
    QSqlQuery saveQuery(connector.getDatabase());
    saveQuery.prepare(R"(
        INSERT OR REPLACE INTO User_Data (form_key, serialized_data, timestamp)
        VALUES (?, ?, datetime('now'))
    )");
    saveQuery.addBindValue(m_formId);
    saveQuery.addBindValue(dataJson);
    
    bool success = saveQuery.exec();
    connector.closeCartridge();
    
    emit formDataSaved(success);
    return success;
}

bool FormEmbeddedWidget::loadFormData()
{
    if (!m_isLoaded || !m_formWidget) {
        return false;
    }
    
    if (m_cartridgePath.isEmpty()) {
        return false;
    }
    
    // Load from User_Data table
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        emit formDataLoaded(false);
        return false;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare(R"(
        SELECT serialized_data FROM User_Data
        WHERE form_key = ?
        ORDER BY timestamp DESC
        LIMIT 1
    )");
    query.addBindValue(m_formId);
    
    if (!query.exec()) {
        connector.closeCartridge();
        emit formDataLoaded(false);
        return false;
    }
    
    if (query.next()) {
        QString dataJson = query.value(0).toString();
        connector.closeCartridge();
        
        // Load data into form widget
        bool success = m_dataSerializer->loadFormData(m_formWidget, dataJson);
        emit formDataLoaded(success);
        return success;
    }
    
    connector.closeCartridge();
    emit formDataLoaded(true); // No data is not an error
    return true;
}

} // namespace reader
} // namespace smartbook

