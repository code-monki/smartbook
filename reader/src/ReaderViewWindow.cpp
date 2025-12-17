#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QCloseEvent>
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
    // TODO: Implement cartridge loading with security verification
    // 1. Verify cartridge signature
    // 2. Check trust registry
    // 3. Show consent dialog if needed
    // 4. Load content
    qDebug() << "Loading cartridge:" << m_cartridgeGuid;
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
