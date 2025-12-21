#ifndef SMARTBOOK_CREATOR_FORMMANAGER_H
#define SMARTBOOK_CREATOR_FORMMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>

namespace smartbook {
namespace common {
namespace database {
    class CartridgeDBConnector;
}
}

namespace creator {

/**
 * @brief Form manager for Form_Definitions table operations
 * 
 * Handles form definition CRUD operations and storage.
 * Implements FR-CT-3.16, FR-CT-3.19
 */
class FormManager : public QObject {
    Q_OBJECT

public:
    explicit FormManager(QObject* parent = nullptr);
    
    /**
     * @brief Open cartridge for form management
     * @param cartridgePath Path to cartridge SQLite file
     * @return true if opened successfully
     */
    bool openCartridge(const QString& cartridgePath);
    
    /**
     * @brief Close cartridge
     */
    void closeCartridge();
    
    /**
     * @brief Get all form IDs
     * @return List of form IDs
     */
    QList<QString> getFormIds() const;
    
    /**
     * @brief Get form definition
     * @param formId Form identifier
     * @return JSON schema string, or empty if not found
     */
    QString getFormDefinition(const QString& formId) const;
    
    /**
     * @brief Save form definition
     * @param formId Form identifier
     * @param schemaJson JSON schema string
     * @param formVersion Form version (defaults to 1)
     * @return true if saved successfully
     */
    bool saveFormDefinition(const QString& formId, const QString& schemaJson, int formVersion = 1);
    
    /**
     * @brief Delete form definition
     * @param formId Form identifier
     * @return true if deleted successfully
     */
    bool deleteFormDefinition(const QString& formId);
    
    /**
     * @brief Check if form exists
     * @param formId Form identifier
     * @return true if form exists
     */
    bool formExists(const QString& formId) const;

signals:
    void formListChanged();

private:
    QString m_cartridgePath;
    common::database::CartridgeDBConnector* m_dbConnector;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_FORMMANAGER_H
