#include "smartbook/reader/ui/QmlEmbeddedAppWidget.h"
#include "smartbook/reader/QmlAppBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QTemporaryFile>
#include <QDir>
#include <QUrl>
#include <QTextStream>
#include <QDebug>

namespace smartbook {
namespace reader {

QmlEmbeddedAppWidget::QmlEmbeddedAppWidget(QWidget* parent)
    : QWidget(parent)
    , m_quickWidget(nullptr)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_bridge(nullptr)
    , m_appLoaded(false)
    , m_hasError(false)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    setupQmlEngine();
}

QmlEmbeddedAppWidget::~QmlEmbeddedAppWidget()
{
    cleanupQmlEngine();
}

void QmlEmbeddedAppWidget::setupQmlEngine()
{
    // Create QML engine
    m_qmlEngine = new QQmlEngine(this);
    
    // Create bridge object
    m_bridge = new QmlAppBridge(this);
    
    // Create QQuickWidget for rendering
    m_quickWidget = new QQuickWidget(m_qmlEngine, this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // Add to layout
    layout()->addWidget(m_quickWidget);
    
    // Expose bridge to QML context
    m_qmlEngine->rootContext()->setContextProperty("SmartbookBridge", m_bridge);
}

void QmlEmbeddedAppWidget::cleanupQmlEngine()
{
    unloadApp();
    
    if (m_quickWidget) {
        layout()->removeWidget(m_quickWidget);
        delete m_quickWidget;
        m_quickWidget = nullptr;
    }
    
    if (m_qmlComponent) {
        delete m_qmlComponent;
        m_qmlComponent = nullptr;
    }
    
    // QML engine will be deleted by parent
    m_qmlEngine = nullptr;
    
    if (m_bridge) {
        delete m_bridge;
        m_bridge = nullptr;
    }
}

bool QmlEmbeddedAppWidget::loadApp(const QString& cartridgePath, const QString& appId)
{
    // Unload any existing app
    unloadApp();
    
    // Reset error state
    m_hasError = false;
    m_errorMessage.clear();
    
    // Load QML code from database
    QString qmlCode = loadQmlCodeFromDatabase(cartridgePath, appId);
    if (qmlCode.isEmpty()) {
        m_hasError = true;
        m_errorMessage = "Failed to load QML code from database";
        emit appLoadError(m_errorMessage);
        return false;
    }
    
    // Set cartridge info on bridge
    setCartridgeInfo(cartridgePath, QString()); // GUID will be set separately if needed
    if (m_bridge) {
        m_bridge->setAppId(appId);
    }
    
    // Create temporary QML file
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        m_hasError = true;
        m_errorMessage = "Failed to create temporary QML file";
        emit appLoadError(m_errorMessage);
        return false;
    }
    
    QTextStream out(&tempFile);
    out << qmlCode;
    tempFile.close();
    
    // Load QML component
    QUrl qmlUrl = QUrl::fromLocalFile(tempFile.fileName());
    m_qmlComponent = new QQmlComponent(m_qmlEngine, qmlUrl, this);
    
    // Check for errors
    if (m_qmlComponent->isError()) {
        QStringList errors;
        for (const QQmlError& error : m_qmlComponent->errors()) {
            errors << error.toString();
        }
        m_hasError = true;
        m_errorMessage = "QML compilation errors:\n" + errors.join("\n");
        qWarning() << m_errorMessage;
        emit appLoadError(m_errorMessage);
        delete m_qmlComponent;
        m_qmlComponent = nullptr;
        return false;
    }
    
    // Create root object
    QObject* rootObject = m_qmlComponent->create();
    if (!rootObject) {
        m_hasError = true;
        m_errorMessage = "Failed to create QML root object";
        qWarning() << m_errorMessage;
        emit appLoadError(m_errorMessage);
        delete m_qmlComponent;
        m_qmlComponent = nullptr;
        return false;
    }
    
    // Set root object in QQuickWidget
    m_quickWidget->setContent(qmlUrl, m_qmlComponent, rootObject);
    
    m_currentAppId = appId;
    m_appLoaded = true;
    
    emit appLoaded();
    return true;
}

void QmlEmbeddedAppWidget::unloadApp()
{
    if (!m_appLoaded) {
        return;
    }
    
    // Clear QML component
    if (m_quickWidget) {
        m_quickWidget->setSource(QUrl());
    }
    
    if (m_qmlComponent) {
        delete m_qmlComponent;
        m_qmlComponent = nullptr;
    }
    
    m_currentAppId.clear();
    m_appLoaded = false;
    m_hasError = false;
    m_errorMessage.clear();
}

void QmlEmbeddedAppWidget::setCartridgeInfo(const QString& cartridgePath, const QString& cartridgeGuid)
{
    m_cartridgePath = cartridgePath;
    m_cartridgeGuid = cartridgeGuid;
    
    if (m_bridge) {
        m_bridge->setCartridgePath(cartridgePath);
        m_bridge->setCartridgeGuid(cartridgeGuid);
    }
}

QString QmlEmbeddedAppWidget::loadQmlCodeFromDatabase(const QString& cartridgePath, const QString& appId)
{
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge:" << cartridgePath;
        return QString();
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare(R"(
        SELECT qml_code FROM Embedded_Apps
        WHERE app_id = ?
    )");
    query.addBindValue(appId);
    
    if (!query.exec()) {
        qWarning() << "Failed to query Embedded_Apps:" << query.lastError().text();
        connector.closeCartridge();
        return QString();
    }
    
    if (!query.next()) {
        qWarning() << "QML app not found:" << appId;
        connector.closeCartridge();
        return QString();
    }
    
    QString qmlCode = query.value(0).toString();
    connector.closeCartridge();
    
    return qmlCode;
}

} // namespace reader
} // namespace smartbook

