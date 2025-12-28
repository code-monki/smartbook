#ifndef SMARTBOOK_READER_QMLAPPBRIDGE_H
#define SMARTBOOK_READER_QMLAPPBRIDGE_H

#include <QObject>
#include <QString>

namespace smartbook {
namespace reader {

/**
 * @brief Bridge object for C++/QML communication in embedded applications
 * 
 * QmlAppBridge provides a secure communication channel between QML embedded applications
 * and the C++ Reader application. It exposes a restricted API that allows QML apps to:
 * - Persist form data to the cartridge database
 * - Access sandboxed file system (isolated per app)
 * - Request user consent (for security-sensitive operations)
 * - Log messages to the application log
 * 
 * @section Registration
 * 
 * The bridge is registered as a QML type "SmartbookBridge" in module "SmartBook 1.0":
 * @code
 * import SmartBook 1.0
 * 
 * Item {
 *     property var bridge: SmartbookBridge
 *     // Use bridge methods...
 * }
 * @endcode
 * 
 * @section Security
 * 
 * **Sandbox Isolation:**
 * - Each app has its own sandbox directory: `{cartridge_guid}/{app_id}/sandbox/`
 * - Apps cannot access files outside their sandbox
 * - Apps cannot access the host file system
 * 
 * **Network Restrictions:**
 * - QML apps have no network access
 * - All network requests are blocked
 * - Apps must use sandbox file system for data persistence
 * 
 * **Form Data:**
 * - Form data is stored in cartridge database (User_Data table)
 * - Data is isolated per cartridge and form ID
 * - Validation is performed before saving
 * 
 * @section API Methods
 * 
 * **Form Data:**
 * - `saveFormData(formId, dataJson)` - Save form data to cartridge
 * - `loadFormData(formId)` - Load form data from cartridge
 * 
 * **Sandbox Files:**
 * - `saveSandboxFile(filename, data)` - Save file to sandbox
 * - `loadSandboxFile(filename)` - Load file from sandbox
 * - `listSandboxFiles()` - List files in sandbox
 * - `deleteSandboxFile(filename)` - Delete file from sandbox
 * 
 * **Other:**
 * - `requestAppConsent(appId)` - Request user consent
 * - `logMessage(level, message)` - Log message (debug, info, warn, error)
 * 
 * @section Signals
 * 
 * All operations emit signals to notify QML of completion:
 * - `formDataSaved(formId, success, error)` - Form save completed
 * - `formDataLoaded(formId, dataJson, error)` - Form load completed
 * - `sandboxFileSaved(filename, success, error)` - File save completed
 * - `sandboxFileLoaded(filename, data, error)` - File load completed
 * - And more...
 * 
 * @note This class replaces WebChannelBridge which was used with Qt WebEngine.
 * The API is similar but adapted for QML instead of JavaScript.
 * 
 * @see QmlEmbeddedAppWidget
 * @see qml-embedded-apps-guide.adoc
 */
class QmlAppBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString cartridgePath READ cartridgePath WRITE setCartridgePath NOTIFY cartridgePathChanged)
    Q_PROPERTY(QString cartridgeGuid READ cartridgeGuid WRITE setCartridgeGuid NOTIFY cartridgeGuidChanged)

public:
    explicit QmlAppBridge(QObject* parent = nullptr);

    QString cartridgePath() const { return m_cartridgePath; }
    void setCartridgePath(const QString& path);
    
    QString cartridgeGuid() const { return m_cartridgeGuid; }
    void setCartridgeGuid(const QString& guid);
    
    /**
     * @brief Set app ID for sandbox operations
     * @param appId Application identifier
     */
    void setAppId(const QString& appId) { m_appId = appId; }

public slots:
    /**
     * @brief Save form data to cartridge
     * @param formId Form identifier
     * @param dataJson JSON string of form data
     */
    Q_INVOKABLE void saveFormData(const QString& formId, const QString& dataJson);

    /**
     * @brief Load form data from cartridge
     * @param formId Form identifier
     */
    Q_INVOKABLE void loadFormData(const QString& formId);

    /**
     * @brief Request user consent for embedded application
     * @param appId Application identifier
     */
    Q_INVOKABLE void requestAppConsent(const QString& appId);

    /**
     * @brief Save file to sandbox
     * @param filename Filename within sandbox
     * @param data File data (as string)
     */
    Q_INVOKABLE void saveSandboxFile(const QString& filename, const QString& data);

    /**
     * @brief Load file from sandbox
     * @param filename Filename within sandbox
     */
    Q_INVOKABLE void loadSandboxFile(const QString& filename);

    /**
     * @brief List files in sandbox
     */
    Q_INVOKABLE void listSandboxFiles();

    /**
     * @brief Delete file from sandbox
     * @param filename Filename within sandbox
     */
    Q_INVOKABLE void deleteSandboxFile(const QString& filename);

    /**
     * @brief Log message from QML
     * @param level Log level (debug, info, warn, error)
     * @param message Log message
     */
    Q_INVOKABLE void logMessage(const QString& level, const QString& message);

signals:
    void cartridgePathChanged();
    void cartridgeGuidChanged();
    void formDataSaved(const QString& formId, bool success, const QString& error);
    void formDataLoaded(const QString& formId, const QString& dataJson, const QString& error);
    void appConsentGranted(const QString& appId);
    void appConsentDenied(const QString& appId);
    void sandboxFileSaved(const QString& filename, bool success, const QString& error);
    void sandboxFileLoaded(const QString& filename, const QString& data, const QString& error);
    void sandboxFilesListed(const QStringList& files, const QString& error);
    void sandboxFileDeleted(const QString& filename, bool success, const QString& error);

private:
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    QString m_appId; // Current app ID for sandbox operations
    
    QString getSandboxPath() const;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_QMLAPPBRIDGE_H

