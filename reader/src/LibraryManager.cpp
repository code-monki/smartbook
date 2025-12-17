#include "smartbook/reader/LibraryManager.h"
#include "smartbook/reader/ui/LibraryView.h"
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QDebug>

namespace smartbook {
namespace reader {

LibraryManager::LibraryManager(QWidget* parent)
    : QMainWindow(parent)
    , m_libraryView(nullptr)
{
    setupUI();
    setupMenuBar();
    
    // Initialize local database
    smartbook::common::database::LocalDBManager& dbManager = 
        smartbook::common::database::LocalDBManager::getInstance();
    dbManager.initializeConnection(QString());

    loadLibrary();
}

LibraryManager::~LibraryManager() {
    // Close all reader windows
    for (auto* window : m_readerWindows) {
        delete window;
    }
}

void LibraryManager::setupUI() {
    setWindowTitle("SmartBook Library");
    resize(1024, 768);

    m_libraryView = new LibraryView(this);
    setCentralWidget(m_libraryView);

    connect(m_libraryView, &LibraryView::cartridgeDoubleClicked,
            this, &LibraryManager::onCartridgeDoubleClicked);
    connect(m_libraryView, &LibraryView::cartridgeDeleteRequested,
            this, &LibraryManager::onDeleteCartridge);

    statusBar()->showMessage("Ready");
}

void LibraryManager::setupMenuBar() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* importAction = fileMenu->addAction("&Import Cartridge...");
    importAction->setShortcut(QKeySequence::New);
    connect(importAction, &QAction::triggered, this, &LibraryManager::onImportCartridge);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About SmartBook");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About SmartBook",
            "SmartBook Reader v1.0.0\n\n"
            "A secure, offline-first e-book reader with embedded applications.");
    });
}

void LibraryManager::loadLibrary() {
    if (m_libraryView) {
        m_libraryView->refreshLibrary();
    }
}

void LibraryManager::openCartridge(const QString& cartridgeGuid) {
    // Create new Reader View Window
    ReaderViewWindow* readerWindow = new ReaderViewWindow(cartridgeGuid, this);
    m_readerWindows.append(readerWindow);
    
    readerWindow->show();
    
    connect(readerWindow, &QObject::destroyed, this, [this, readerWindow]() {
        m_readerWindows.removeAll(readerWindow);
    });
}

void LibraryManager::onImportCartridge() {
    // TODO: Implement import dialog
    QMessageBox::information(this, "Import Cartridge",
        "Import functionality will be implemented.");
}

void LibraryManager::onDeleteCartridge(const QString& cartridgeGuid) {
    int ret = QMessageBox::question(this, "Delete Cartridge",
        "Are you sure you want to delete this cartridge?",
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // TODO: Implement deletion
        QMessageBox::information(this, "Delete Cartridge",
            "Deletion functionality will be implemented.");
    }
}

void LibraryManager::onCartridgeDoubleClicked(const QString& cartridgeGuid) {
    openCartridge(cartridgeGuid);
}

} // namespace reader
} // namespace smartbook
