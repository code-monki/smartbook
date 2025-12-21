#include "smartbook/creator/ResourceManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>

namespace smartbook {
namespace creator {

ResourceManager::ResourceManager(QObject* parent)
    : QObject(parent)
    , m_dbConnector(nullptr)
{
}

bool ResourceManager::openCartridge(const QString& cartridgePath)
{
    if (m_dbConnector) {
        closeCartridge();
    }
    
    m_cartridgePath = cartridgePath;
    m_dbConnector = new common::database::CartridgeDBConnector(this);
    
    if (!m_dbConnector->openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge for resource management:" << cartridgePath;
        delete m_dbConnector;
        m_dbConnector = nullptr;
        return false;
    }
    
    return true;
}

void ResourceManager::closeCartridge()
{
    if (m_dbConnector) {
        m_dbConnector->closeCartridge();
        delete m_dbConnector;
        m_dbConnector = nullptr;
    }
    m_cartridgePath.clear();
}

QList<ResourceInfo> ResourceManager::getResources() const
{
    QList<ResourceInfo> resources;
    
    if (!m_dbConnector) {
        return resources;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT resource_id, resource_path, resource_type, resource_data, mime_type FROM Resources ORDER BY resource_id");
    
    if (!query.exec()) {
        qWarning() << "Failed to get resources:" << query.lastError().text();
        return resources;
    }
    
    while (query.next()) {
        ResourceInfo info;
        info.resourceId = query.value(0).toString();
        info.resourcePath = query.value(1).toString();
        info.resourceType = query.value(2).toString();
        info.resourceData = query.value(3).toByteArray();
        info.mimeType = query.value(4).toString();
        resources.append(info);
    }
    
    return resources;
}

ResourceInfo ResourceManager::getResource(const QString& resourceId) const
{
    ResourceInfo info;
    
    if (!m_dbConnector || resourceId.isEmpty()) {
        return info;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT resource_id, resource_path, resource_type, resource_data, mime_type FROM Resources WHERE resource_id = ?");
    query.addBindValue(resourceId);
    
    if (!query.exec() || !query.next()) {
        return info;
    }
    
    info.resourceId = query.value(0).toString();
    info.resourcePath = query.value(1).toString();
    info.resourceType = query.value(2).toString();
    info.resourceData = query.value(3).toByteArray();
    info.mimeType = query.value(4).toString();
    
    return info;
}

QString ResourceManager::importResource(const QString& filePath, const QString& resourceId)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for resource import";
        return QString();
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open resource file:" << filePath;
        return QString();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QString finalResourceId = resourceId.isEmpty() ? generateResourceId(filePath) : resourceId;
    QString mimeType = detectMimeType(filePath);
    QString resourceType = detectResourceType(mimeType);
    
    QFileInfo fileInfo(filePath);
    QString resourcePath = fileInfo.fileName();
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare(R"(
        INSERT OR REPLACE INTO Resources (resource_id, resource_path, resource_type, resource_data, mime_type)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(finalResourceId);
    query.addBindValue(resourcePath);
    query.addBindValue(resourceType);
    query.addBindValue(data);
    query.addBindValue(mimeType);
    
    if (!query.exec()) {
        qWarning() << "Failed to import resource:" << query.lastError().text();
        return QString();
    }
    
    emit resourceListChanged();
    return finalResourceId;
}

bool ResourceManager::importResourceData(const QByteArray& data, const QString& resourceId,
                                        const QString& resourceType, const QString& mimeType)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for resource import";
        return false;
    }
    
    if (resourceId.isEmpty()) {
        qWarning() << "Resource ID is required";
        return false;
    }
    
    QString resourcePath = resourceId; // Use resource ID as path if not specified
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare(R"(
        INSERT OR REPLACE INTO Resources (resource_id, resource_path, resource_type, resource_data, mime_type)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(resourceId);
    query.addBindValue(resourcePath);
    query.addBindValue(resourceType);
    query.addBindValue(data);
    query.addBindValue(mimeType);
    
    if (!query.exec()) {
        qWarning() << "Failed to import resource data:" << query.lastError().text();
        return false;
    }
    
    emit resourceListChanged();
    return true;
}

bool ResourceManager::deleteResource(const QString& resourceId)
{
    if (!m_dbConnector) {
        qWarning() << "No cartridge open for resource deletion";
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("DELETE FROM Resources WHERE resource_id = ?");
    query.addBindValue(resourceId);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete resource:" << query.lastError().text();
        return false;
    }
    
    emit resourceListChanged();
    return true;
}

bool ResourceManager::resourceExists(const QString& resourceId) const
{
    if (!m_dbConnector) {
        return false;
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT COUNT(*) FROM Resources WHERE resource_id = ?");
    query.addBindValue(resourceId);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    return query.value(0).toInt() > 0;
}

QByteArray ResourceManager::getResourceData(const QString& resourceId) const
{
    if (!m_dbConnector) {
        return QByteArray();
    }
    
    QSqlQuery query(m_dbConnector->getDatabase());
    query.prepare("SELECT resource_data FROM Resources WHERE resource_id = ?");
    query.addBindValue(resourceId);
    
    if (!query.exec() || !query.next()) {
        return QByteArray();
    }
    
    return query.value(0).toByteArray();
}

QString ResourceManager::generateResourceId(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName();
    QString extension = fileInfo.suffix();
    
    // Generate unique ID: basename_timestamp.extension
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    return QString("%1_%2.%3").arg(baseName, timestamp, extension);
}

QString ResourceManager::detectMimeType(const QString& filePath) const
{
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);
    return mimeType.name();
}

QString ResourceManager::detectResourceType(const QString& mimeType) const
{
    if (mimeType.startsWith("image/")) {
        return "image";
    } else if (mimeType.startsWith("font/") || mimeType.contains("font")) {
        return "font";
    } else if (mimeType.startsWith("audio/")) {
        return "audio";
    } else if (mimeType.startsWith("video/")) {
        return "video";
    } else {
        return "other";
    }
}

} // namespace creator
} // namespace smartbook
