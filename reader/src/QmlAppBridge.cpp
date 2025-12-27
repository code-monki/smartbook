#include "smartbook/reader/QmlAppBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/reader/ui/ConsentDialog.h"
#include "smartbook/common/security/TrustRegistry.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QDialog>
#include <QDebug>

namespace smartbook {
namespace reader {

QmlAppBridge::QmlAppBridge(QObject* parent)
    : QObject(parent)
{
}

void QmlAppBridge::setCartridgePath(const QString& path)
{
    if (m_cartridgePath != path) {
        m_cartridgePath = path;
        emit cartridgePathChanged();
    }
}

void QmlAppBridge::setCartridgeGuid(const QString& guid)
{
    if (m_cartridgeGuid != guid) {
        m_cartridgeGuid = guid;
        emit cartridgeGuidChanged();
    }
}

void QmlAppBridge::saveFormData(const QString& formId, const QString& dataJson)
{
    // DDD Section: Form Data Persistence
    // Save form data to cartridge database
    
    if (m_cartridgePath.isEmpty()) {
        qWarning() << "Cannot save form data: no cartridge path set";
        emit formDataSaved(formId, false, "No cartridge path set");
        return;
    }
    
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        QString error = "Failed to open cartridge: " + m_cartridgePath;
        qWarning() << error;
        emit formDataSaved(formId, false, error);
        return;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare(R"(
        INSERT OR REPLACE INTO User_Data (form_id, data_json, last_updated)
        VALUES (?, ?, datetime('now'))
    )");
    query.addBindValue(formId);
    query.addBindValue(dataJson);
    
    if (!query.exec()) {
        QString error = "Failed to save form data: " + query.lastError().text();
        qWarning() << error;
        connector.closeCartridge();
        emit formDataSaved(formId, false, error);
        return;
    }
    
    connector.closeCartridge();
    qDebug() << "Form data saved:" << formId;
    emit formDataSaved(formId, true, QString());
}

void QmlAppBridge::loadFormData(const QString& formId)
{
    // DDD Section: Form Data Persistence
    // Load form data from cartridge database
    
    if (m_cartridgePath.isEmpty()) {
        qWarning() << "Cannot load form data: no cartridge path set";
        emit formDataLoaded(formId, QString(), "No cartridge path set");
        return;
    }
    
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(m_cartridgePath)) {
        QString error = "Failed to open cartridge: " + m_cartridgePath;
        qWarning() << error;
        emit formDataLoaded(formId, QString(), error);
        return;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare(R"(
        SELECT data_json FROM User_Data
        WHERE form_id = ?
        ORDER BY last_updated DESC
        LIMIT 1
    )");
    query.addBindValue(formId);
    
    if (!query.exec()) {
        QString error = "Failed to load form data: " + query.lastError().text();
        qWarning() << error;
        connector.closeCartridge();
        emit formDataLoaded(formId, QString(), error);
        return;
    }
    
    if (query.next()) {
        QString dataJson = query.value(0).toString();
        connector.closeCartridge();
        emit formDataLoaded(formId, dataJson, QString());
    } else {
        connector.closeCartridge();
        emit formDataLoaded(formId, QString(), QString()); // Not an error - just no data
    }
}

void QmlAppBridge::requestAppConsent(const QString& appId)
{
    // DDD Section: Embedded Application Consent
    // Show consent dialog for embedded application
    
    if (m_cartridgeGuid.isEmpty()) {
        qWarning() << "Cannot request app consent: no cartridge GUID set";
        emit appConsentDenied(appId);
        return;
    }
    
    // Check trust registry for existing trust decision
    common::security::TrustRegistry trustRegistry(this);
    common::security::TrustRegistry::TrustPolicy policy = trustRegistry.getTrustDecision(m_cartridgeGuid);
    
    // If already has persistent trust, grant immediately
    if (policy == common::security::TrustRegistry::TrustPolicy::PERSISTENT) {
        qDebug() << "App consent granted (persistent trust):" << appId;
        emit appConsentGranted(appId);
        return;
    }
    
    // If trust is revoked, deny immediately
    if (policy == common::security::TrustRegistry::TrustPolicy::REVOKED) {
        qDebug() << "App consent denied (trust revoked):" << appId;
        emit appConsentDenied(appId);
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
    ui::ConsentDialog consentDialog(
        common::security::SecurityLevel::LEVEL_2,
        cartridgeTitle,
        authorName,
        nullptr // No parent widget in QML context
    );
    
    int dialogResult = consentDialog.exec();
    ui::ConsentDialog::ConsentResult userChoice = consentDialog.getResult();
    
    if (dialogResult == QDialog::Accepted && userChoice != ui::ConsentDialog::Cancel) {
        // User chose to load the app
        if (userChoice == ui::ConsentDialog::LoadAndAlwaysTrust) {
            // Store persistent trust
            trustRegistry.storeTrustDecision(
                m_cartridgeGuid,
                common::security::TrustRegistry::TrustPolicy::PERSISTENT
            );
        } else if (userChoice == ui::ConsentDialog::LoadForSessionOnly) {
            // Store session trust
            trustRegistry.storeTrustDecision(
                m_cartridgeGuid,
                common::security::TrustRegistry::TrustPolicy::SESSION
            );
        }
        emit appConsentGranted(appId);
    } else {
        // User cancelled or closed dialog
        emit appConsentDenied(appId);
    }
}

void QmlAppBridge::saveSandboxFile(const QString& filename, const QString& data)
{
    // DDD Section: Sandbox File System API
    // Save file to app's sandbox directory
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        qWarning() << "Cannot save sandbox file: cartridge GUID or app ID not set";
        emit sandboxFileSaved(filename, false, "Cartridge or app ID not set");
        return;
    }
    
    QString sandboxPath = getSandboxPath();
    if (sandboxPath.isEmpty()) {
        emit sandboxFileSaved(filename, false, "Sandbox directory not available");
        return;
    }
    
    // Validate filename (prevent directory traversal)
    if (filename.contains("..") || filename.contains("/") || filename.contains("\\")) {
        qWarning() << "Invalid sandbox filename:" << filename;
        emit sandboxFileSaved(filename, false, "Invalid filename");
        return;
    }
    
    QFileInfo fileInfo(filename);
    QString safeFilename = fileInfo.fileName();
    
    QString filePath = QDir(sandboxPath).filePath(safeFilename);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString error = "Failed to open file for writing: " + file.errorString();
        qWarning() << error;
        emit sandboxFileSaved(filename, false, error);
        return;
    }
    
    QTextStream out(&file);
    out << data;
    file.close();
    
    qDebug() << "Sandbox file saved:" << filePath;
    emit sandboxFileSaved(filename, true, QString());
}

void QmlAppBridge::loadSandboxFile(const QString& filename)
{
    // DDD Section: Sandbox File System API
    // Load file from app's sandbox directory
    
    if (m_cartridgeGuid.isEmpty() || m_appId.isEmpty()) {
        qWarning() << "Cannot load sandbox file: cartridge GUID or app ID not set";
        emit sandboxFileLoaded(filename, QString(), "Cartridge or app ID not set");
        return;
    }
    
    QString sandboxPath = getSandboxPath();
    if (sandboxPath.isEmpty()) {
        emit sandboxFileLoaded(filename, QString(), "Sandbox directory not available");
        return;
    }
    
    // Validate filename (prevent directory traversal)
    if (filename.contains("..") || filename.contains("/") || filename.contains("\\")) {
        qWarning() << "Invalid sandbox filename:" << filename;
        emit sandboxFileLoaded(filename, QString(), "Invalid filename");
        return;
    }
    
    QFileInfo fileInfo(filename);
    QString safeFilename = fileInfo.fileName();
    
    QString filePath = QDir(sandboxPath).filePath(safeFilename);
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "Sandbox file not found:" << filePath;
        emit sandboxFileLoaded(filename, QString(), "File not found");
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString error = "Failed to open file for reading: " + file.errorString();
        qWarning() << error;
        emit sandboxFileLoaded(filename, QString(), error);
        return;
    }
    
    QTextStream in(&file);
    QString data = in.readAll();
    file.close();
    
    qDebug() << "Sandbox file loaded:" << filePath;
    emit sandboxFileLoaded(filename, data, QString());
}

void QmlAppBridge::listSandboxFiles()
{
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
        emit sandboxFilesListed(QStringList(), QString());
        return;
    }
    
    QStringList files = sandboxDir.entryList(QDir::Files, QDir::Name);
    qDebug() << "Sandbox files listed:" << files;
    emit sandboxFilesListed(files, QString());
}

void QmlAppBridge::deleteSandboxFile(const QString& filename)
{
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

void QmlAppBridge::logMessage(const QString& level, const QString& message)
{
    if (level == "error") {
        qCritical() << "[QML]" << message;
    } else if (level == "warn") {
        qWarning() << "[QML]" << message;
    } else {
        qDebug() << "[QML]" << message;
    }
}

QString QmlAppBridge::getSandboxPath() const
{
    // DDD Section: Sandbox File System API
    // Sandbox path: {AppData}/SmartBook/sandbox/{cartridge_guid}/{app_id}/
    
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        qWarning() << "Cannot determine AppData location for sandbox";
        return QString();
    }
    
    QDir appDataDir(appDataPath);
    QString sandboxBase = appDataDir.filePath("sandbox");
    QString cartridgeSandbox = QDir(sandboxBase).filePath(m_cartridgeGuid);
    QString appSandbox = QDir(cartridgeSandbox).filePath(m_appId);
    
    // Ensure directory exists
    QDir().mkpath(appSandbox);
    
    return appSandbox;
}

} // namespace reader
} // namespace smartbook

