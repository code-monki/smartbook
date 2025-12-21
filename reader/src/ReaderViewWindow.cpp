#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QCloseEvent>
#include <QSqlQuery>
#include <QDebug>
#include <QApplication>

namespace smartbook {
namespace reader {

ReaderViewWindow::ReaderViewWindow(const QString& cartridgeGuid, QWidget* parent)
    : QMainWindow(parent)
    , m_cartridgeGuid(cartridgeGuid)
    , m_readerView(nullptr)
    , m_webChannelBridge(nullptr)
{
    setupUI();
    loadCartridge();
}

ReaderViewWindow::~ReaderViewWindow() {
    saveWindowState();
}

void ReaderViewWindow::setupUI() {
    setWindowTitle("SmartBook Reader");
    resize(1024, 768);

    m_readerView = new ReaderView(this);
    setCentralWidget(m_readerView);

    connect(m_readerView, &ReaderView::contentLoaded,
            this, &ReaderViewWindow::onContentLoaded);
    connect(m_readerView, &ReaderView::errorOccurred,
            this, &ReaderViewWindow::onError);
}

void ReaderViewWindow::loadCartridge() {
    // Get cartridge path from manifest
    common::database::LocalDBManager& dbManager = 
        common::database::LocalDBManager::getInstance();
    
    if (!dbManager.isOpen()) {
        emit onError("Local database not open");
        return;
    }
    
    // Query manifest for cartridge path
    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT local_path FROM Local_Library_Manifest WHERE cartridge_guid = ?");
    query.addBindValue(m_cartridgeGuid);
    
    if (!query.exec() || !query.next()) {
        emit onError("Cartridge not found in manifest: " + m_cartridgeGuid);
        return;
    }
    
    QString cartridgePath = query.value(0).toString();
    
    if (cartridgePath.isEmpty()) {
        emit onError("Cartridge path is empty for: " + m_cartridgeGuid);
        return;
    }
    
    // TODO: Implement security verification
    // 1. Verify cartridge signature (SignatureVerifier)
    // 2. Check trust registry
    // 3. Show consent dialog if needed (L2/L3 cartridges)
    // 4. Load content
    
    // For now, load content directly (with cartridge GUID for settings)
    if (m_readerView) {
        m_readerView->loadCartridge(cartridgePath, m_cartridgeGuid);
    }
}

void ReaderViewWindow::onContentLoaded() {
    setWindowTitle("SmartBook Reader - " + m_cartridgeGuid);
}

void ReaderViewWindow::onError(const QString& errorMessage) {
    qWarning() << "Reader View error:" << errorMessage;
    // TODO: Show error dialog to user
}

void ReaderViewWindow::closeEvent(QCloseEvent* event) {
    saveWindowState();
    QMainWindow::closeEvent(event);
}

void ReaderViewWindow::saveWindowState() {
    // TODO: Save window state to Local_Window_State table
    // TODO: Save reading position to Local_Reading_Position table
}

} // namespace reader
} // namespace smartbook
