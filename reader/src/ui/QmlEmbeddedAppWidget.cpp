/**
 * @file QmlEmbeddedAppWidget.cpp
 * @brief Implementation of QmlEmbeddedAppWidget for embedding QML applications
 * 
 * This file implements the QmlEmbeddedAppWidget class, which hosts QML embedded
 * applications within cartridge content pages. It loads QML code from the cartridge
 * database and renders it using QQuickWidget.
 * 
 * @section QML Loading Process
 * 
 * 1. Load QML code from Embedded_Apps.qml_code column
 * 2. Write QML code to temporary file
 * 3. Load QML file using QQuickWidget::setSource()
 * 4. Expose QmlAppBridge to QML context as "SmartbookBridge"
 * 5. Handle QML compilation errors and emit signals
 * 
 * @section C++/QML Communication
 * 
 * QmlAppBridge is exposed to QML context as a context property:
 * - QML can access: `property var bridge: SmartbookBridge`
 * - Bridge provides: form data persistence, sandbox file operations, logging
 * 
 * @section Error Handling
 * 
 * If QML code fails to compile or load:
 * - hasError() returns true
 * - errorMessage() contains QML error details
 * - appLoadError() signal is emitted
 * - Widget remains empty (no crash)
 * 
 * @see QmlEmbeddedAppWidget.h
 * @see QmlAppBridge
 * @see ReaderView
 * @see qml-embedded-apps-guide.adoc
 */

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

// ============================================================================
// Constructor and Destructor
// ============================================================================

QmlEmbeddedAppWidget::QmlEmbeddedAppWidget(QWidget* parent)
    : QWidget(parent)
    , m_quickWidget(nullptr)
    , m_qmlEngine(nullptr)
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

// ============================================================================
// Private Methods - QML Engine Setup
// ============================================================================

void QmlEmbeddedAppWidget::setupQmlEngine()
{
    /**
     * @brief Setup QML engine and bridge
     * 
     * Creates QQmlEngine, QmlAppBridge, and QQuickWidget, and exposes
     * bridge to QML context as "SmartbookBridge".
     */
    
    // Create QML engine
    m_qmlEngine = new QQmlEngine(this);
    
    // Create bridge object for C++/QML communication
    m_bridge = new QmlAppBridge(this);
    
    // Create QQuickWidget for rendering QML
    // SizeRootObjectToView ensures QML root item resizes to match widget size
    m_quickWidget = new QQuickWidget(m_qmlEngine, this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // Add to layout
    layout()->addWidget(m_quickWidget);
    
    // Expose bridge to QML context as "SmartbookBridge"
    // QML can access via: property var bridge: SmartbookBridge
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
    
    // QML engine will be deleted by parent
    m_qmlEngine = nullptr;
    
    if (m_bridge) {
        delete m_bridge;
        m_bridge = nullptr;
    }
}

// ============================================================================
// Public Methods
// ============================================================================

bool QmlEmbeddedAppWidget::loadApp(const QString& cartridgePath, const QString& appId)
{
    /**
     * @brief Load QML application from cartridge database
     * 
     * Loads QML code from Embedded_Apps table and renders it using QQuickWidget.
     * 
     * Process:
     * 1. Unload any existing app
     * 2. Load QML code from database (Embedded_Apps.qml_code column)
     * 3. Write QML code to temporary file
     * 4. Load QML file using QQuickWidget::setSource()
     * 5. Check for QML compilation errors
     * 6. Emit appLoaded() signal on success
     * 
     * @param cartridgePath Path to cartridge file
     * @param appId Application identifier (must match Embedded_Apps.app_id)
     * @return true if loaded successfully, false otherwise
     * 
     * @note Emits appLoadError() signal if load fails
     */
    
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
    
    // Set cartridge info on bridge (use existing values if already set)
    if (m_bridge) {
        m_bridge->setAppId(appId);
        // Only update if not already set
        if (m_cartridgePath.isEmpty()) {
            setCartridgeInfo(cartridgePath, QString()); // GUID will be set separately if needed
        }
    } else {
        setCartridgeInfo(cartridgePath, QString());
    }
    
    // Create temporary QML file
    // QQuickWidget requires a file URL, so we write QML code to temp file
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
    
    // Load QML from file using setSource (simpler approach, matches PoC)
    QUrl qmlUrl = QUrl::fromLocalFile(tempFile.fileName());
    m_quickWidget->setSource(qmlUrl);
    
    // Check for QML compilation errors
    if (m_quickWidget->status() == QQuickWidget::Error) {
        QStringList errors;
        for (const QQmlError& error : m_quickWidget->errors()) {
            errors << error.toString();
        }
        m_hasError = true;
        m_errorMessage = "QML compilation errors:\n" + errors.join("\n");
        qWarning() << m_errorMessage;
        emit appLoadError(m_errorMessage);
        return false;
    }
    
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

// ============================================================================
// Private Methods - Database Operations
// ============================================================================

QString QmlEmbeddedAppWidget::loadQmlCodeFromDatabase(const QString& cartridgePath, const QString& appId)
{
    /**
     * @brief Load QML code from cartridge database
     * 
     * Queries Embedded_Apps table for QML code associated with appId.
     * 
     * @param cartridgePath Path to cartridge file
     * @param appId Application identifier
     * @return QML code string, or empty string on error
     */
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

