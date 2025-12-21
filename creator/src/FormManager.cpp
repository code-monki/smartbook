#include "smartbook/creator/FormManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

namespace smartbook {
namespace creator {

FormManager::FormManager(QObject* parent)
    : QObject(parent)
    , m_dbConnector(nullptr)
{
}

bool FormManager::openCartridge(const QString& cartridgePath)
{
    if (m_dbConnector) {
        closeCartridge();
    }
    
    m_cartridgePath = cartridgePath;
    m_dbConnector = new common::database::CartridgeDBConnector(this);
    
    if (!m_dbConnector->openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge for form management:" << cartridgePath;
        delete m_dbConnector;
        m_dbConnector = nullptr;
        return false;
    }
    
    return true;
}

void FormManager::closeCartridge()
{
    if (m_dbConnector) {
        m_dbConnector->closeCartridge();
        delete m_dbConnector;
        m_dbConnector = nullptr;
    }
    m_cartridgePath.clear();
}

QList<QString> FormManager::getFormIds() const
{
    QList<QString> formIds;
    
    if (!m_dbConnector) {
        return formIds;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT form_id FROM Form_Definitions ORDER BY form_id");
    
    if (!query.exec()) {
        qWarning() << "Failed to get form IDs:" << query.lastError().text();
        return formIds;
    }
    
    while (query.next()) {
        formIds.append(query.value(0).toString());
    }
    
    return formIds;
}

QString FormManager::getFormDefinition(const QString& formId) const
{
    if (!m_dbConnector) {
        return QString();
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT form_schema_json FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(formId);
    
    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to get form definition:" << query.lastError().text();
        return QString();
    }
    
    return query.value(0).toString();
}

bool FormManager::saveFormDefinition(const QString& formId, const QString& schemaJson, int formVersion)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for form save";
        return false;
    }
    
    // Check if form exists
    bool exists = formExists(formId);
    
    QSqlQuery query(m_dbConnector->getDatabase());
    
    if (exists) {
        // Update existing form
        query.prepare("UPDATE Form_Definitions SET form_schema_json = ?, form_version = ? WHERE form_id = ?");
        query.addBindValue(schemaJson);
        query.addBindValue(formVersion);
        query.addBindValue(formId);
    } else {
        // Insert new form
        query.prepare("INSERT INTO Form_Definitions (form_id, form_schema_json, form_version) VALUES (?, ?, ?)");
        query.addBindValue(formId);
        query.addBindValue(schemaJson);
        query.addBindValue(formVersion);
    }
    
    if (!query.exec()) {
        qWarning() << "Failed to save form definition:" << query.lastError().text();
        return false;
    }
    
    emit formListChanged();
    return true;
}

bool FormManager::deleteFormDefinition(const QString& formId)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for form deletion";
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("DELETE FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(formId);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete form definition:" << query.lastError().text();
        return false;
    }
    
    emit formListChanged();
    return true;
}

bool FormManager::formExists(const QString& formId) const
{
    if (!m_dbConnector) {
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT COUNT(*) FROM Form_Definitions WHERE form_id = ?");
    query.addBindValue(formId);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    return query.value(0).toInt() > 0;
}

} // namespace creator
} // namespace smartbook
