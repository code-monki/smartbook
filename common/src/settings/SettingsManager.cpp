#include "smartbook/common/settings/SettingsManager.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSet>
#include <QDebug>

namespace smartbook {
namespace common {
namespace settings {

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
{
}

bool SettingsManager::loadSettings(const QString& cartridgeGuid, const QString& cartridgePath)
{
    m_cartridgeGuid = cartridgeGuid;
    m_authorSettings.clear();
    m_userOverrides.clear();
    
    // Load author-defined settings from cartridge
    loadAuthorSettings(cartridgePath);
    
    // Load user overrides from local database
    loadUserOverrides(cartridgeGuid);
    
    return true;
}

void SettingsManager::loadAuthorSettings(const QString& cartridgePath)
{
    // Open cartridge database
    database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge for settings:" << cartridgePath;
        return;
    }
    
    // Query Settings table from cartridge
    QSqlQuery query = connector.executeQuery(
        "SELECT setting_key, setting_value, setting_type FROM Settings"
    );
    
    while (query.next()) {
        SettingValue setting;
        QString key = query.value(0).toString();
        setting.value = query.value(1).toString();
        setting.type = query.value(2).toString();
        
        if (setting.isValid()) {
            m_authorSettings[key] = setting;
        }
    }
    
    connector.closeCartridge();
}

void SettingsManager::loadUserOverrides(const QString& cartridgeGuid)
{
    database::LocalDBManager& dbManager = database::LocalDBManager::getInstance();
    if (!dbManager.isOpen()) {
        qWarning() << "Local database not open for user settings";
        return;
    }
    
    // Query Local_User_Settings table
    QSqlQuery query(dbManager.getDatabase());
    query.prepare(R"(
        SELECT setting_key, setting_value
        FROM Local_User_Settings
        WHERE cartridge_guid = ?
    )");
    query.addBindValue(cartridgeGuid);
    
    if (!query.exec()) {
        qWarning() << "Failed to load user settings:" << query.lastError().text();
        return;
    }
    
    while (query.next()) {
        QString key = query.value(0).toString();
        QString value = query.value(1).toString();
        m_userOverrides[key] = value;
    }
}

QString SettingsManager::getSetting(const QString& settingKey, const QString& defaultValue) const
{
    return resolveSetting(settingKey, defaultValue);
}

SettingsManager::SettingValue SettingsManager::getSettingWithType(const QString& settingKey) const
{
    SettingValue result;
    
    // Check user override first
    if (m_userOverrides.contains(settingKey)) {
        result.value = m_userOverrides[settingKey];
        // Get type from author settings if available
        if (m_authorSettings.contains(settingKey)) {
            result.type = m_authorSettings[settingKey].type;
        } else {
            result.type = "string"; // Default type for user overrides
        }
        return result;
    }
    
    // Check author settings
    if (m_authorSettings.contains(settingKey)) {
        return m_authorSettings[settingKey];
    }
    
    // Return invalid (no setting found)
    return result;
}

QString SettingsManager::resolveSetting(const QString& settingKey, const QString& appDefault) const
{
    // Priority 1: User override
    if (m_userOverrides.contains(settingKey)) {
        return m_userOverrides[settingKey];
    }
    
    // Priority 2: Author default
    if (m_authorSettings.contains(settingKey)) {
        return m_authorSettings[settingKey].value;
    }
    
    // Priority 3: Application default
    return appDefault;
}

bool SettingsManager::setUserOverride(const QString& settingKey, const QString& value)
{
    if (m_cartridgeGuid.isEmpty()) {
        qWarning() << "No cartridge loaded for setting override";
        return false;
    }
    
    database::LocalDBManager& dbManager = database::LocalDBManager::getInstance();
    if (!dbManager.isOpen()) {
        qWarning() << "Local database not open for setting override";
        return false;
    }
    
    QSqlQuery query(dbManager.getDatabase());
    
    // Use INSERT OR REPLACE to update existing override
    query.prepare(R"(
        INSERT OR REPLACE INTO Local_User_Settings
        (cartridge_guid, setting_key, setting_value, timestamp)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(m_cartridgeGuid);
    query.addBindValue(settingKey);
    query.addBindValue(value);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    
    if (!query.exec()) {
        qWarning() << "Failed to save user setting override:" << query.lastError().text();
        return false;
    }
    
    // Update in-memory cache
    m_userOverrides[settingKey] = value;
    
    return true;
}

bool SettingsManager::resetToAuthorDefaults()
{
    if (m_cartridgeGuid.isEmpty()) {
        qWarning() << "No cartridge loaded for settings reset";
        return false;
    }
    
    database::LocalDBManager& dbManager = database::LocalDBManager::getInstance();
    if (!dbManager.isOpen()) {
        qWarning() << "Local database not open for settings reset";
        return false;
    }
    
    QSqlQuery query(dbManager.getDatabase());
    query.prepare(R"(
        DELETE FROM Local_User_Settings
        WHERE cartridge_guid = ?
    )");
    query.addBindValue(m_cartridgeGuid);
    
    if (!query.exec()) {
        qWarning() << "Failed to reset user settings:" << query.lastError().text();
        return false;
    }
    
    // Clear in-memory cache
    m_userOverrides.clear();
    
    return true;
}

QMap<QString, QString> SettingsManager::getAllSettings() const
{
    QMap<QString, QString> result;
    
    // Collect all setting keys (from author settings and user overrides)
    QSet<QString> allKeys;
    for (auto it = m_authorSettings.begin(); it != m_authorSettings.end(); ++it) {
        allKeys.insert(it.key());
    }
    for (auto it = m_userOverrides.begin(); it != m_userOverrides.end(); ++it) {
        allKeys.insert(it.key());
    }
    
    // Resolve each setting with priority
    for (const QString& key : allKeys) {
        result[key] = resolveSetting(key, QString());
    }
    
    return result;
}

} // namespace settings
} // namespace common
} // namespace smartbook
