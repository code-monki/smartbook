#ifndef SMARTBOOK_CREATOR_RESOURCEMANAGER_H
#define SMARTBOOK_CREATOR_RESOURCEMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QByteArray>

namespace smartbook {
namespace common {
namespace database {
    class CartridgeDBConnector;
}
}

namespace creator {

/**
 * @brief Resource information structure
 */
struct ResourceInfo {
    QString resourceId;
    QString resourcePath;
    QString resourceType;
    QByteArray resourceData;
    QString mimeType;
    
    bool isValid() const { return !resourceId.isEmpty(); }
};

/**
 * @brief Resource manager for Resources table operations
 * 
 * Handles resource CRUD operations and storage.
 * Implements FR-CT-3.24, FR-CT-3.25
 */
class ResourceManager : public QObject {
    Q_OBJECT

public:
    explicit ResourceManager(QObject* parent = nullptr);
    
    /**
     * @brief Open cartridge for resource management
     * @param cartridgePath Path to cartridge SQLite file
     * @return true if opened successfully
     */
    bool openCartridge(const QString& cartridgePath);
    
    /**
     * @brief Close cartridge
     */
    void closeCartridge();
    
    /**
     * @brief Get all resources
     * @return List of resource information
     */
    QList<ResourceInfo> getResources() const;
    
    /**
     * @brief Get resource by ID
     * @param resourceId Resource identifier
     * @return Resource information, or invalid ResourceInfo if not found
     */
    ResourceInfo getResource(const QString& resourceId) const;
    
    /**
     * @brief Import resource from file
     * @param filePath Path to resource file
     * @param resourceId Optional resource ID (auto-generated if empty)
     * @return Resource ID if successful, empty string otherwise
     */
    QString importResource(const QString& filePath, const QString& resourceId = QString());
    
    /**
     * @brief Import resource from data
     * @param data Resource data
     * @param resourceId Resource identifier
     * @param resourceType Resource type (e.g., "image", "font", "media")
     * @param mimeType MIME type
     * @return true if imported successfully
     */
    bool importResourceData(const QByteArray& data, const QString& resourceId,
                           const QString& resourceType, const QString& mimeType);
    
    /**
     * @brief Delete resource
     * @param resourceId Resource identifier
     * @return true if deleted successfully
     */
    bool deleteResource(const QString& resourceId);
    
    /**
     * @brief Check if resource exists
     * @param resourceId Resource identifier
     * @return true if resource exists
     */
    bool resourceExists(const QString& resourceId) const;
    
    /**
     * @brief Get resource data
     * @param resourceId Resource identifier
     * @return Resource data, or empty QByteArray if not found
     */
    QByteArray getResourceData(const QString& resourceId) const;
    
    /**
     * @brief Get current cartridge path
     * @return Cartridge path if open, empty string otherwise
     */
    QString getCartridgePath() const { return m_cartridgePath; }

signals:
    void resourceListChanged();

private:
    QString generateResourceId(const QString& filePath) const;
    QString detectMimeType(const QString& filePath) const;
    QString detectResourceType(const QString& mimeType) const;
    
    QString m_cartridgePath;
    common::database::CartridgeDBConnector* m_dbConnector;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_RESOURCEMANAGER_H
