#include "smartbook/reader/WebChannelBridge.h"
#include <QWebChannel>
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

void WebChannelBridge::saveFormData(const QString& formId, const QString& /* dataJson */, const QString& /* callback */) {
    qDebug() << "saveFormData called:" << formId;
    // TODO: Implement form data saving
    emit formDataSaved(formId, true, QString());
}

void WebChannelBridge::loadFormData(const QString& formId, const QString& callback) {
    qDebug() << "loadFormData called:" << formId;
    // TODO: Implement form data loading
    emit formDataLoaded(formId, QString(), QString());
}

void WebChannelBridge::requestAppConsent(const QString& appId, const QString& /* callback */) {
    qDebug() << "requestAppConsent called:" << appId;
    // TODO: Implement consent dialog
    emit consentGranted(appId, false);
}

void WebChannelBridge::saveSandboxFile(const QString& filename, const QByteArray& /* data */, const QString& /* callback */) {
    qDebug() << "saveSandboxFile called:" << filename;
    // TODO: Implement sandbox file saving
    emit sandboxFileSaved(filename, true, QString());
}

void WebChannelBridge::loadSandboxFile(const QString& filename, const QString& /* callback */) {
    qDebug() << "loadSandboxFile called:" << filename;
    // TODO: Implement sandbox file loading
    emit sandboxFileLoaded(filename, QByteArray(), QString());
}

void WebChannelBridge::listSandboxFiles(const QString& /* callback */) {
    qDebug() << "listSandboxFiles called";
    // TODO: Implement sandbox file listing
    emit sandboxFilesListed(QStringList(), QString());
}

void WebChannelBridge::deleteSandboxFile(const QString& filename, const QString& /* callback */) {
    qDebug() << "deleteSandboxFile called:" << filename;
    // TODO: Implement sandbox file deletion
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

} // namespace reader
} // namespace smartbook
