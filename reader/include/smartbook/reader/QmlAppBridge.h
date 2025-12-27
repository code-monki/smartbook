#ifndef SMARTBOOK_READER_QMLAPPBRIDGE_H
#define SMARTBOOK_READER_QMLAPPBRIDGE_H

#include <QObject>
#include <QString>

namespace smartbook {
namespace reader {

/**
 * @brief Bridge object for C++/QML communication
 * 
 * Exposes restricted API (saveFormData, requestAppConsent, sandbox file operations)
 * to QML embedded applications. Similar to WebChannelBridge but for QML instead of JavaScript.
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

