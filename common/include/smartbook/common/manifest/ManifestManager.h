#ifndef SMARTBOOK_COMMON_MANIFEST_MANIFESTMANAGER_H
#define SMARTBOOK_COMMON_MANIFEST_MANIFESTMANAGER_H

#include "smartbook/common/database/LocalDBManager.h"
#include <QString>
#include <QByteArray>
#include <QObject>

namespace smartbook {
namespace common {
namespace manifest {

/**
 * @brief Manages manifest entries in Local_Library_Manifest table
 * 
 * Handles creation, retrieval, and management of manifest entries
 * for imported cartridges.
 * 
 * Implements FR-2.5.1 (Manifest Creation) and related requirements.
 */
class ManifestManager : public QObject {
    Q_OBJECT

public:
    struct ManifestEntry {
        QString cartridgeGuid;
        QByteArray cartridgeHash;
        QString localPath;
        QString title;
        QString author;
        QString publisher;
        QString version;
        QString publicationYear;
        QByteArray coverImageData;
        
        bool isValid() const { return !cartridgeGuid.isEmpty() && !title.isEmpty(); }
    };

    explicit ManifestManager(QObject* parent = nullptr);
    
    /**
     * @brief Create a new manifest entry
     * @param entry Manifest entry data
     * @return true if creation successful, false otherwise
     */
    bool createManifestEntry(const ManifestEntry& entry);
    
    /**
     * @brief Get manifest entry by cartridge GUID
     * @param cartridgeGuid Cartridge GUID to look up
     * @return ManifestEntry (invalid if not found)
     */
    ManifestEntry getManifestEntry(const QString& cartridgeGuid);
    
    /**
     * @brief Update existing manifest entry
     * @param entry Manifest entry data (must have valid cartridgeGuid)
     * @return true if update successful, false otherwise
     */
    bool updateManifestEntry(const ManifestEntry& entry);
    
    /**
     * @brief Check if manifest entry exists
     * @param cartridgeGuid Cartridge GUID to check
     * @return true if entry exists, false otherwise
     */
    bool manifestEntryExists(const QString& cartridgeGuid);
    
    /**
     * @brief Delete manifest entry by cartridge GUID
     * @param cartridgeGuid Cartridge GUID to delete
     * @return true if deletion successful, false otherwise
     */
    bool deleteManifestEntry(const QString& cartridgeGuid);

private:
    database::LocalDBManager* m_dbManager;
};

} // namespace manifest
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_MANIFEST_MANIFESTMANAGER_H
