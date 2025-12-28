#ifndef SMARTBOOK_READER_UI_QMLEMBEDDEDAPPWIDGET_H
#define SMARTBOOK_READER_UI_QMLEMBEDDEDAPPWIDGET_H

#include <QWidget>
#include <QString>

class QQuickWidget;
class QQmlEngine;
class QQmlComponent;
class QQmlContext;

namespace smartbook {
namespace reader {

class QmlAppBridge;

/**
 * @brief Widget for embedding QML applications in content pages
 * 
 * QmlEmbeddedAppWidget hosts a QML embedded application within a cartridge content page.
 * It loads QML code from the cartridge database (Embedded_Apps table) and renders it
 * using QQuickWidget, providing a seamless integration with HTML content.
 * 
 * @section Architecture
 * 
 * **QML Loading:**
 * - QML code is stored in `Embedded_Apps.qml_code` column
 * - Code is written to a temporary file and loaded via `QQuickWidget::setSource()`
 * - Each app runs in its own isolated QQuickWidget instance
 * 
 * **C++/QML Communication:**
 * - QmlAppBridge is exposed to QML context as "SmartbookBridge"
 * - Bridge provides access to cartridge database and sandbox file system
 * - All communication is synchronous (no async JavaScript bridge)
 * 
 * **Lifecycle:**
 * - Apps are loaded when content page is displayed
 * - Apps are unloaded when page changes or widget is destroyed
 * - Each app instance is independent (no shared state)
 * 
 * @section Usage
 * 
 * @code
 * QmlEmbeddedAppWidget* widget = new QmlEmbeddedAppWidget(parent);
 * widget->setCartridgeInfo(cartridgePath, cartridgeGuid);
 * bool loaded = widget->loadApp(cartridgePath, "my_app_id");
 * if (loaded) {
 *     // App is ready to use
 * }
 * @endcode
 * 
 * @section Security
 * 
 * **Isolation:**
 * - Each app runs in its own QQuickWidget instance
 * - Apps cannot access each other's data or state
 * - Apps cannot access the host file system directly
 * 
 * **Network Restrictions:**
 * - QML apps have no network access
 * - All network requests are blocked
 * - Apps must use sandbox file system for data persistence
 * 
 * @section Error Handling
 * 
 * If QML code fails to load or compile:
 * - `hasError()` returns true
 * - `errorMessage()` contains error details
 * - `appLoadError()` signal is emitted
 * - Widget remains empty (no crash)
 * 
 * @note This widget is created automatically by ReaderView when it detects
 * QML app markers in HTML content. Manual instantiation is rarely needed.
 * 
 * @see ReaderView
 * @see QmlAppBridge
 * @see ContentParser
 * @see qml-embedded-apps-guide.adoc
 */
class QmlEmbeddedAppWidget : public QWidget {
    Q_OBJECT

public:
    explicit QmlEmbeddedAppWidget(QWidget* parent = nullptr);
    ~QmlEmbeddedAppWidget();

    /**
     * @brief Load QML application from cartridge database
     * @param cartridgePath Path to cartridge file
     * @param appId Application identifier
     * @return true if loaded successfully, false otherwise
     */
    bool loadApp(const QString& cartridgePath, const QString& appId);
    
    /**
     * @brief Unload current QML application
     */
    void unloadApp();
    
    /**
     * @brief Check if an app is currently loaded
     * @return true if app is loaded, false otherwise
     */
    bool isAppLoaded() const { return m_appLoaded; }
    
    /**
     * @brief Check if there was an error loading the app
     * @return true if error occurred, false otherwise
     */
    bool hasError() const { return m_hasError; }
    
    /**
     * @brief Get error message if load failed
     * @return Error message string
     */
    QString errorMessage() const { return m_errorMessage; }
    
    /**
     * @brief Set cartridge information for bridge
     * @param cartridgePath Path to cartridge file
     * @param cartridgeGuid Cartridge GUID
     */
    void setCartridgeInfo(const QString& cartridgePath, const QString& cartridgeGuid);
    
    /**
     * @brief Get cartridge path
     * @return Cartridge path
     */
    QString cartridgePath() const { return m_cartridgePath; }
    
    /**
     * @brief Get cartridge GUID
     * @return Cartridge GUID
     */
    QString cartridgeGuid() const { return m_cartridgeGuid; }

signals:
    /**
     * @brief Emitted when app is loaded successfully
     */
    void appLoaded();
    
    /**
     * @brief Emitted when app fails to load
     * @param errorMessage Error description
     */
    void appLoadError(const QString& errorMessage);

private:
    void setupQmlEngine();
    void cleanupQmlEngine();
    QString loadQmlCodeFromDatabase(const QString& cartridgePath, const QString& appId);
    
    QQuickWidget* m_quickWidget;
    QQmlEngine* m_qmlEngine;
    QmlAppBridge* m_bridge;
    
    QString m_cartridgePath;
    QString m_cartridgeGuid;
    QString m_currentAppId;
    bool m_appLoaded;
    bool m_hasError;
    QString m_errorMessage;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_QMLEMBEDDEDAPPWIDGET_H

