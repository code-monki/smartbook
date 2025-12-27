#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/reader/ui/ConsentDialog.h"
#include "smartbook/common/security/TrustRegistry.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include <QWebChannel>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace smartbook {
namespace reader {

WebChannelBridge::WebChannelBridge(QObject* parent)
    : QObject(parent)
{
}

void WebChannelBridge::setupWebChannel(QWebChannel* webChannel) {
    if (webChannel) {
        webChannel->registerObject("SmartbookBridge", this);
    }
}

void WebChannelBridge::setCartridgeInfo(const QString& cartridgePath, const QString& cartridgeGuid) {
    m_cartridgePath = cartridgePath;
    m_cartridgeGuid = cartridgeGuid;
}

void WebChannelBridge::saveFormData(const QString& formId, const QString& dataJson, const QString& /* callback */) {
    // DDD Section: Form Data Persistence
    // Save form data to User_Data table in cartridge
    
    if (m_cartridgePath.isEmpty()) {
        qWarning() << "Cannot save form data: no cartridge path set";
        emit formDataSaved(formId, false, "No cartridge loaded");
        return;
    }
    
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        QString error = "Failed to open cartridge: " + m_cartridgePath;
        qWarning() << error;
        emit formDataSaved(formId, false, error);
        return;
    }
    
    bool success = connector.saveFormData(formId, dataJson);
    connector.closeCartridge();
    
    if (success) {
        qDebug() << "Form data saved:" << formId;
    emit formDataSaved(formId, true, QString());
    } else {
        QString error = "Failed to save form data to cartridge";
        qWarning() << error;
        emit formDataSaved(formId, false, error);
    }
}

void WebChannelBridge::loadFormData(const QString& formId, const QString& /* callback */) {
    // DDD Section: Form Data Persistence
    // Load form data from User_Data table in cartridge
    
    if (m_cartridgePath.isEmpty()) {
        qWarning() << "Cannot load form data: no cartridge path set";
        emit formDataLoaded(formId, QString(), "No cartridge loaded");
        return;
    }
    
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        QString error = "Failed to open cartridge: " + m_cartridgePath;
        qWarning() << error;
        emit formDataLoaded(formId, QString(), error);
        return;
    }
    
    QString dataJson = connector.loadFormData(formId);
    connector.closeCartridge();
    
    if (!dataJson.isEmpty()) {
        qDebug() << "Form data loaded:" << formId;
        emit formDataLoaded(formId, dataJson, QString());
    } else {
        qDebug() << "No form data found for:" << formId;
        emit formDataLoaded(formId, QString(), QString()); // Not an error - just no data
    }
}

void WebChannelBridge::requestAppConsent(const QString& appId, const QString& /* callback */) {
    // DDD Section: Embedded Application Consent
    // Show consent dialog for embedded application
    
    if (m_cartridgeGuid.isEmpty()) {
        qWarning() << "Cannot request app consent: no cartridge GUID set";
    emit consentGranted(appId, false);
        return;
    }
    
    // Check trust registry for existing trust decision
    common::security::TrustRegistry trustRegistry(this);
    common::security::TrustRegistry::TrustPolicy policy = trustRegistry.getTrustDecision(m_cartridgeGuid);
    
    // If already has persistent trust, grant immediately
    if (policy == common::security::TrustRegistry::TrustPolicy::PERSISTENT) {
        qDebug() << "App consent granted (persistent trust):" << appId;
        emit consentGranted(appId, true);
        return;
    }
    
    // If trust is revoked, deny immediately
    if (policy == common::security::TrustRegistry::TrustPolicy::REVOKED) {
        qDebug() << "App consent denied (trust revoked):" << appId;
        emit consentGranted(appId, false);
        return;
    }
    
    // Get cartridge metadata for consent dialog
    QString cartridgeTitle;
    QString authorName;
    
    if (!m_cartridgePath.isEmpty()) {
        common::database::CartridgeDBConnector connector(this);
        if (connector.openCartridge(m_cartridgePath)) {
            QSqlQuery query(connector.getDatabase());
            if (query.exec("SELECT title, author FROM Metadata LIMIT 1") && query.next()) {
                cartridgeTitle = query.value(0).toString();
                authorName = query.value(1).toString();
            }
            connector.closeCartridge();
        }
    }
    
    // Show consent dialog (using L2 security level for embedded apps)
    // Note: This is a simplified consent - full implementation would query Embedded_Apps table
    // for app metadata (name, description) to show in dialog
    reader::ui::ConsentDialog consentDialog(
        common::security::SecurityLevel::LEVEL_2,
        cartridgeTitle.isEmpty() ? "Unknown Cartridge" : cartridgeTitle,
        authorName.isEmpty() ? "Unknown Author" : authorName,
        nullptr // No parent widget in WebChannel context
    );
    
    int result = consentDialog.exec();
    reader::ui::ConsentDialog::ConsentResult userChoice = consentDialog.getResult();
    
    bool granted = (result == QDialog::Accepted && 
                    userChoice != ui::ConsentDialog::Cancel);
    
    if (granted) {
        // Store trust decision based on user choice
        if (userChoice == reader::ui::ConsentDialog::LoadAndAlwaysTrust) {
            trustRegistry.storeTrustDecision(m_cartridgeGuid, 
                common::security::TrustRegistry::TrustPolicy::PERSISTENT);
        } else if (userChoice == reader::ui::ConsentDialog::LoadForSessionOnly) {
            trustRegistry.storeTrustDecision(m_cartridgeGuid,
                common::security::TrustRegistry::TrustPolicy::SESSION);
        }
    }
    
    qDebug() << "App consent result:" << appId << "granted:" << granted;
    emit consentGranted(appId, granted);
}

void WebChannelBridge::saveSandboxFile(const QString& filename, const QByteArray& data, const QString& /* callback */) {
    // DDD Section: Sandbox File System API
    // Save file to app's sandbox directory: {cartridge_guid}/{app_id}/sandbox/
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        qWarning() << "Cannot save sandbox file: cartridge GUID or app ID not set";
        emit sandboxFileSaved(filename, false, "Cartridge or app ID not set");
        return;
    }
    
    QString sandboxPath = getSandboxPath();
    if (sandboxPath.isEmpty()) {
        emit sandboxFileSaved(filename, false, "Failed to create sandbox directory");
        return;
    }
    
    // Validate filename (prevent directory traversal)
    // Check original filename first before sanitizing
    if (filename.contains("..") || filename.contains("/") || filename.contains("\\")) {
        qWarning() << "Invalid sandbox filename:" << filename;
        emit sandboxFileSaved(filename, false, "Invalid filename");
        return;
    }
    
    QFileInfo fileInfo(filename);
    QString safeFilename = fileInfo.fileName(); // Remove any path components
    
    QString filePath = QDir(sandboxPath).filePath(safeFilename);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        QString error = "Failed to open file for writing: " + file.errorString();
        qWarning() << error;
        emit sandboxFileSaved(filename, false, error);
        return;
    }
    
    qint64 bytesWritten = file.write(data);
    file.close();
    
    if (bytesWritten == data.size()) {
        qDebug() << "Sandbox file saved:" << filePath;
    emit sandboxFileSaved(filename, true, QString());
    } else {
        QString error = "Failed to write all data";
        qWarning() << error;
        emit sandboxFileSaved(filename, false, error);
    }
}

void WebChannelBridge::loadSandboxFile(const QString& filename, const QString& /* callback */) {
    // DDD Section: Sandbox File System API
    // Load file from app's sandbox directory
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        qWarning() << "Cannot load sandbox file: cartridge GUID or app ID not set";
        emit sandboxFileLoaded(filename, QByteArray(), "Cartridge or app ID not set");
        return;
    }
    
    QString sandboxPath = getSandboxPath();
    if (sandboxPath.isEmpty()) {
        emit sandboxFileLoaded(filename, QByteArray(), "Sandbox directory not available");
        return;
    }
    
    // Validate filename (prevent directory traversal)
    // Check original filename first before sanitizing
    if (filename.contains("..") || filename.contains("/") || filename.contains("\\")) {
        qWarning() << "Invalid sandbox filename:" << filename;
        emit sandboxFileLoaded(filename, QByteArray(), "Invalid filename");
        return;
    }
    
    QFileInfo fileInfo(filename);
    QString safeFilename = fileInfo.fileName();
    
    QString filePath = QDir(sandboxPath).filePath(safeFilename);
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "Sandbox file not found:" << filePath;
        emit sandboxFileLoaded(filename, QByteArray(), "File not found");
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        QString error = "Failed to open file for reading: " + file.errorString();
        qWarning() << error;
        emit sandboxFileLoaded(filename, QByteArray(), error);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    qDebug() << "Sandbox file loaded:" << filePath << "size:" << data.size();
    emit sandboxFileLoaded(filename, data, QString());
}

void WebChannelBridge::listSandboxFiles(const QString& /* callback */) {
    // DDD Section: Sandbox File System API
    // List files in app's sandbox directory
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        qWarning() << "Cannot list sandbox files: cartridge GUID or app ID not set";
        emit sandboxFilesListed(QStringList(), "Cartridge or app ID not set");
        return;
    }
    
    QString sandboxPath = getSandboxPath();
    if (sandboxPath.isEmpty()) {
        emit sandboxFilesListed(QStringList(), "Sandbox directory not available");
        return;
    }
    
    QDir sandboxDir(sandboxPath);
    if (!sandboxDir.exists()) {
        qDebug() << "Sandbox directory does not exist:" << sandboxPath;
        emit sandboxFilesListed(QStringList(), QString()); // Empty list, not an error
        return;
    }
    
    QStringList files = sandboxDir.entryList(QDir::Files, QDir::Name);
    qDebug() << "Sandbox files listed:" << files.size() << "files";
    emit sandboxFilesListed(files, QString());
}

void WebChannelBridge::deleteSandboxFile(const QString& filename, const QString& /* callback */) {
    // DDD Section: Sandbox File System API
    // Delete file from app's sandbox directory
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        qWarning() << "Cannot delete sandbox file: cartridge GUID or app ID not set";
        emit sandboxFileDeleted(filename, false, "Cartridge or app ID not set");
        return;
    }
    
    QString sandboxPath = getSandboxPath();
    if (sandboxPath.isEmpty()) {
        emit sandboxFileDeleted(filename, false, "Sandbox directory not available");
        return;
    }
    
    // Validate filename (prevent directory traversal)
    // Check original filename first before sanitizing
    if (filename.contains("..") || filename.contains("/") || filename.contains("\\")) {
        qWarning() << "Invalid sandbox filename:" << filename;
        emit sandboxFileDeleted(filename, false, "Invalid filename");
        return;
    }
    
    QFileInfo fileInfo(filename);
    QString safeFilename = fileInfo.fileName();
    
    QString filePath = QDir(sandboxPath).filePath(safeFilename);
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "Sandbox file not found for deletion:" << filePath;
        emit sandboxFileDeleted(filename, false, "File not found");
        return;
    }
    
    if (!file.remove()) {
        QString error = "Failed to delete file: " + file.errorString();
        qWarning() << error;
        emit sandboxFileDeleted(filename, false, error);
        return;
    }
    
    qDebug() << "Sandbox file deleted:" << filePath;
    emit sandboxFileDeleted(filename, true, QString());
}

void WebChannelBridge::logMessage(const QString& level, const QString& message) {
    if (level == "error") {
        qCritical() << "[JS]" << message;
    } else if (level == "warn") {
        qWarning() << "[JS]" << message;
    } else {
        qDebug() << "[JS]" << message;
    }
}

QString WebChannelBridge::getSandboxPath() const {
    // DDD Section: Sandbox File System API
    // Sandbox path: {app_data_dir}/sandbox/{cartridge_guid}/{app_id}/sandbox/
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        return QString();
    }
    
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString sandboxBase = QDir(appDataDir).filePath("sandbox");
    QString cartridgeDir = QDir(sandboxBase).filePath(m_cartridgeGuid);
    QString appDir = QDir(cartridgeDir).filePath(m_appId);
    QString sandboxPath = QDir(appDir).filePath("sandbox");
    
    // Create directory structure if it doesn't exist
    QDir dir;
    if (!dir.mkpath(sandboxPath)) {
        qWarning() << "Failed to create sandbox directory:" << sandboxPath;
        return QString();
    }
    
    return sandboxPath;
}

} // namespace reader
} // namespace smartbook
