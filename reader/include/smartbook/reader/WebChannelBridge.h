#ifndef SMARTBOOK_READER_WEBCHANNELBRIDGE_H
#define SMARTBOOK_READER_WEBCHANNELBRIDGE_H

#include <QObject>
#include <QWebChannel>
#include <QString>

namespace smartbook {
namespace reader {

/**
 * @brief WebChannel Bridge - IPC layer between embedded JS and native C++ code
 * 
 * Exposes restricted API (saveFormData, requestAppConsent, sandbox file operations)
 * to JavaScript embedded applications.
 */
class WebChannelBridge : public QObject {
    Q_OBJECT

public:
    explicit WebChannelBridge(QObject* parent = nullptr);

    /**
     * @brief Setup WebChannel for a QWebEngineView
     * @param webChannel QWebChannel instance
     */
    void setupWebChannel(QWebChannel* webChannel);

public slots:
    /**
     * @brief Save form data to cartridge
     * @param formId Form identifier
     * @param dataJson JSON string of form data
     * @param callback JavaScript callback function name
     */
    void saveFormData(const QString& formId, const QString& dataJson, const QString& callback);

    /**
     * @brief Load form data from cartridge
     * @param formId Form identifier
     * @param callback JavaScript callback function name
     */
    void loadFormData(const QString& formId, const QString& callback);

    /**
     * @brief Request user consent for embedded application
     * @param appId Application identifier
     * @param callback JavaScript callback function name
     */
    void requestAppConsent(const QString& appId, const QString& callback);

    /**
     * @brief Save file to sandbox
     * @param filename Filename within sandbox
     * @param data File data
     * @param callback JavaScript callback function name
     */
    void saveSandboxFile(const QString& filename, const QByteArray& data, const QString& callback);

    /**
     * @brief Load file from sandbox
     * @param filename Filename within sandbox
     * @param callback JavaScript callback function name
     */
    void loadSandboxFile(const QString& filename, const QString& callback);

    /**
     * @brief List files in sandbox
     * @param callback JavaScript callback function name
     */
    void listSandboxFiles(const QString& callback);

    /**
     * @brief Delete file from sandbox
     * @param filename Filename within sandbox
     * @param callback JavaScript callback function name
     */
    void deleteSandboxFile(const QString& filename, const QString& callback);

    /**
     * @brief Log message from JavaScript
     * @param level Log level (debug, info, warn, error)
     * @param message Log message
     */
    void logMessage(const QString& level, const QString& message);

signals:
    void formDataSaved(const QString& formId, bool success, const QString& error);
    void formDataLoaded(const QString& formId, const QString& dataJson, const QString& error);
    void consentGranted(const QString& appId, bool granted);
    void sandboxFileSaved(const QString& filename, bool success, const QString& error);
    void sandboxFileLoaded(const QString& filename, const QByteArray& data, const QString& error);
    void sandboxFilesListed(const QStringList& files, const QString& error);
    void sandboxFileDeleted(const QString& filename, bool success, const QString& error);

private:
    QString m_cartridgeGuid;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_WEBCHANNELBRIDGE_H
