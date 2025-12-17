#include "smartbook/creator/CreatorMainWindow.h"
#include "smartbook/creator/ContentEditor.h"
#include "smartbook/creator/FormBuilder.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>

namespace smartbook {
namespace creator {

CreatorMainWindow::CreatorMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_contentEditor(nullptr)
    , m_formBuilder(nullptr)
{
    setupUI();
    setupMenuBar();
}

CreatorMainWindow::~CreatorMainWindow() {
}

void CreatorMainWindow::setupUI() {
    setWindowTitle("SmartBook Creator");
    resize(1280, 800);

    m_contentEditor = new ContentEditor(this);
    setCentralWidget(m_contentEditor);

    statusBar()->showMessage("Ready");
}

void CreatorMainWindow::setupMenuBar() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* newAction = fileMenu->addAction("&New Cartridge...");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &CreatorMainWindow::onNewCartridge);

    QAction* openAction = fileMenu->addAction("&Open Cartridge...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &CreatorMainWindow::onOpenCartridge);

    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &CreatorMainWindow::onSaveCartridge);

    fileMenu->addSeparator();

    QAction* exportAction = fileMenu->addAction("&Export Cartridge...");
    connect(exportAction, &QAction::triggered, this, &CreatorMainWindow::onExportCartridge);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About SmartBook Creator");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About SmartBook Creator",
            "SmartBook Creator v1.0.0\n\n"
            "Create and edit Smartbook cartridges with embedded applications.");
    });
}

void CreatorMainWindow::onNewCartridge() {
    // TODO: Implement new cartridge creation
    QMessageBox::information(this, "New Cartridge",
        "New cartridge functionality will be implemented.");
}

void CreatorMainWindow::onOpenCartridge() {
    // TODO: Implement open cartridge
    QMessageBox::information(this, "Open Cartridge",
        "Open cartridge functionality will be implemented.");
}

void CreatorMainWindow::onSaveCartridge() {
    // TODO: Implement save cartridge
    QMessageBox::information(this, "Save Cartridge",
        "Save cartridge functionality will be implemented.");
}

void CreatorMainWindow::onExportCartridge() {
    // TODO: Implement export cartridge
    QMessageBox::information(this, "Export Cartridge",
        "Export cartridge functionality will be implemented.");
}

} // namespace creator
} // namespace smartbook
