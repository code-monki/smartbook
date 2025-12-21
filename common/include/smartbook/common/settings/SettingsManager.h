#ifndef SMARTBOOK_COMMON_SETTINGS_SETTINGSMANAGER_H
#define SMARTBOOK_COMMON_SETTINGS_SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>

namespace smartbook {
namespace common {
namespace settings {

/**
 * @brief Manages rendering settings with priority resolution
 * 
 * Handles reading settings from cartridge Settings table,
 * user overrides from Local_User_Settings, and applying
 * priority (User > Author > App default).
 * 
 * Implements FR-2.2.3, FR-2.6.1 through FR-2.6.5
 */
class SettingsManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Setting value with type information
     */
    struct SettingValue {
        QString value;
        QString type;  // "string", "integer", "float", "boolean", "json"
        bool isValid() const { return !value.isEmpty() && !type.isEmpty(); }
    };

    explicit SettingsManager(QObject* parent = nullptr);
    
    /**
     * @brief Load settings for a cartridge
     * @param cartridgeGuid Cartridge GUID
     * @param cartridgePath Path to cartridge file
     * @return true if loaded successfully, false otherwise
     */
    bool loadSettings(const QString& cartridgeGuid, const QString& cartridgePath);
    
    /**
     * @brief Get setting value with priority resolution
     * @param settingKey Setting key (e.g., "default_font_size")
     * @param defaultValue Application default value
     * @return Resolved setting value (User override > Author default > App default)
     */
    QString getSetting(const QString& settingKey, const QString& defaultValue = QString()) const;
    
    /**
     * @brief Get setting value with type information
     * @param settingKey Setting key
     * @return SettingValue with resolved value and type
     */
    SettingValue getSettingWithType(const QString& settingKey) const;
    
    /**
     * @brief Set user override for a setting
     * @param settingKey Setting key
     * @param value Override value
     * @return true if saved successfully, false otherwise
     */
    bool setUserOverride(const QString& settingKey, const QString& value);
    
    /**
     * @brief Reset user overrides for cartridge (restore author defaults)
     * @return true if reset successfully, false otherwise
     */
    bool resetToAuthorDefaults();
    
    /**
     * @brief Get all resolved settings as a map
     * @return Map of setting_key -> resolved_value
     */
    QMap<QString, QString> getAllSettings() const;

private:
    void loadAuthorSettings(const QString& cartridgePath);
    void loadUserOverrides(const QString& cartridgeGuid);
    QString resolveSetting(const QString& settingKey, const QString& appDefault) const;
    
    QString m_cartridgeGuid;
    QMap<QString, SettingValue> m_authorSettings;  // From cartridge Settings table
    QMap<QString, QString> m_userOverrides;      // From Local_User_Settings
};

} // namespace settings
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_SETTINGS_SETTINGSMANAGER_H
