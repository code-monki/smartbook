#include "smartbook/common/manifest/ManifestManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace smartbook {
namespace common {
namespace manifest {

ManifestManager::ManifestManager(QObject* parent)
    : QObject(parent)
    , m_dbManager(&database::LocalDBManager::getInstance())
{
}

bool ManifestManager::createManifestEntry(const ManifestEntry& entry)
{
    if (!m_dbManager->isOpen()) {
        qWarning() << "Database not open for manifest entry creation";
        return false;
    }
    
    // Validate required fields
    if (entry.cartridgeGuid.isEmpty()) {
        qWarning() << "Cannot create manifest entry: empty cartridge GUID";
        return false;
    }
    
    if (entry.title.isEmpty()) {
        qWarning() << "Cannot create manifest entry: empty title";
        return false;
    }
    
    if (entry.publicationYear.isEmpty()) {
        qWarning() << "Cannot create manifest entry: empty publication year";
        return false;
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare(R"(
        INSERT INTO Local_Library_Manifest 
        (cartridge_guid, cartridge_hash, local_path, title, author, publisher, version, publication_year, cover_image_data)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(entry.cartridgeGuid);
    query.addBindValue(entry.cartridgeHash);
    query.addBindValue(entry.localPath);
    query.addBindValue(entry.title);
    query.addBindValue(entry.author);
    query.addBindValue(entry.publisher);
    query.addBindValue(entry.version);
    query.addBindValue(entry.publicationYear);
    query.addBindValue(entry.coverImageData);
    
    if (!query.exec()) {
        qCritical() << "Failed to create manifest entry:" << query.lastError().text();
        return false;
    }
    
    return true;
}

ManifestManager::ManifestEntry ManifestManager::getManifestEntry(const QString& cartridgeGuid)
{
    ManifestEntry entry;
    
    if (!m_dbManager->isOpen()) {
        qWarning() << "Database not open for manifest entry retrieval";
        return entry;
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare(R"(
        SELECT cartridge_guid, cartridge_hash, local_path, title, author, publisher, 
               version, publication_year, cover_image_data
        FROM Local_Library_Manifest
        WHERE cartridge_guid = ?
    )");
    query.addBindValue(cartridgeGuid);
    
    if (!query.exec()) {
        qWarning() << "Failed to query manifest entry:" << query.lastError().text();
        return entry;
    }
    
    if (query.next()) {
        entry.cartridgeGuid = query.value(0).toString();
        entry.cartridgeHash = query.value(1).toByteArray();
        entry.localPath = query.value(2).toString();
        entry.title = query.value(3).toString();
        entry.author = query.value(4).toString();
        entry.publisher = query.value(5).toString();
        entry.version = query.value(6).toString();
        entry.publicationYear = query.value(7).toString();
        entry.coverImageData = query.value(8).toByteArray();
    }
    
    return entry;
}

bool ManifestManager::updateManifestEntry(const ManifestEntry& entry)
{
    if (!m_dbManager->isOpen()) {
        qWarning() << "Database not open for manifest entry update";
        return false;
    }
    
    if (entry.cartridgeGuid.isEmpty()) {
        qWarning() << "Cannot update manifest entry: empty cartridge GUID";
        return false;
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare(R"(
        UPDATE Local_Library_Manifest 
        SET cartridge_hash = ?, local_path = ?, title = ?, author = ?, 
            publisher = ?, version = ?, publication_year = ?, cover_image_data = ?
        WHERE cartridge_guid = ?
    )");
    
    query.addBindValue(entry.cartridgeHash);
    query.addBindValue(entry.localPath);
    query.addBindValue(entry.title);
    query.addBindValue(entry.author);
    query.addBindValue(entry.publisher);
    query.addBindValue(entry.version);
    query.addBindValue(entry.publicationYear);
    query.addBindValue(entry.coverImageData);
    query.addBindValue(entry.cartridgeGuid);
    
    if (!query.exec()) {
        qCritical() << "Failed to update manifest entry:" << query.lastError().text();
        return false;
    }
    
    // Check if any rows were affected
    if (query.numRowsAffected() == 0) {
        qWarning() << "No manifest entry found to update for GUID:" << entry.cartridgeGuid;
        return false;
    }
    
    return true;
}

bool ManifestManager::manifestEntryExists(const QString& cartridgeGuid)
{
    if (!m_dbManager->isOpen()) {
        return false;
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("SELECT COUNT(*) FROM Local_Library_Manifest WHERE cartridge_guid = ?");
    query.addBindValue(cartridgeGuid);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool ManifestManager::deleteManifestEntry(const QString& cartridgeGuid)
{
    if (!m_dbManager->isOpen()) {
        qWarning() << "Database not open for manifest entry deletion";
        return false;
    }
    
    if (cartridgeGuid.isEmpty()) {
        qWarning() << "Cannot delete manifest entry: empty cartridge GUID";
        return false;
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("DELETE FROM Local_Library_Manifest WHERE cartridge_guid = ?");
    query.addBindValue(cartridgeGuid);
    
    if (!query.exec()) {
        qCritical() << "Failed to delete manifest entry:" << query.lastError().text();
        return false;
    }
    
    // Check if any rows were affected
    if (query.numRowsAffected() == 0) {
        qWarning() << "No manifest entry found to delete for GUID:" << cartridgeGuid;
        return false;
    }
    
    return true;
}

} // namespace manifest
} // namespace common
} // namespace smartbook
