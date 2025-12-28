#include "smartbook/reader/LibraryManager.h"
#include "smartbook/reader/ui/LibraryView.h"
#include "smartbook/reader/ui/ImportDialog.h"
#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ImportManager.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/security/TrustRegistry.h"
#include "smartbook/common/utils/ThemeManager.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QPalette>
#include <QColor>

namespace smartbook {
namespace reader {

LibraryManager::LibraryManager(QWidget* parent)
    : QMainWindow(parent)
    , m_themeGroup(nullptr)
    , m_libraryView(nullptr)
    , m_importManager(new ImportManager(this))
{
    setupUI();
    setupMenuBar();
    
    // Initialize local database
    smartbook::common::database::LocalDBManager& dbManager = 
        smartbook::common::database::LocalDBManager::getInstance();
    dbManager.initializeConnection(QString());

    // Connect to theme change notifications
    connect(&common::utils::ThemeManager::getInstance(), &common::utils::ThemeManager::themeChanged,
            this, [this](const QString& theme) {
                qDebug() << "LibraryManager: Theme changed to" << theme << ", updating UI";
                setThemeMenuSelection(theme);
                applyTheme();
            });

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

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    
    // Theme submenu
    QMenu* themeMenu = viewMenu->addMenu("&Theme");
    m_themeGroup = new QActionGroup(this);
    
    QAction* autoAction = themeMenu->addAction("&Auto");
    autoAction->setCheckable(true);
    autoAction->setData("auto");
    m_themeGroup->addAction(autoAction);
    
    QAction* lightAction = themeMenu->addAction("&Light");
    lightAction->setCheckable(true);
    lightAction->setData("light");
    m_themeGroup->addAction(lightAction);
    
    QAction* darkAction = themeMenu->addAction("&Dark");
    darkAction->setCheckable(true);
    darkAction->setData("dark");
    m_themeGroup->addAction(darkAction);
    
    QAction* sepiaAction = themeMenu->addAction("&Sepia");
    sepiaAction->setCheckable(true);
    sepiaAction->setData("sepia");
    m_themeGroup->addAction(sepiaAction);
    
    // Load saved theme preference and set menu selection
    QString savedTheme = common::utils::ThemeManager::getInstance().getTheme();
    setThemeMenuSelection(savedTheme);
    
    // Connect theme actions
    connect(m_themeGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        QString theme = action->data().toString();
        qDebug() << "LibraryManager: Theme menu triggered, theme=" << theme;
        
        // Save global preference (shared across all apps)
        common::utils::ThemeManager::getInstance().setTheme(theme);
        
        // Apply theme to this window
        applyTheme();
    });
    
    // Apply initial theme
    applyTheme();

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

QList<CartridgeInfo> LibraryManager::loadLibraryData()
{
    QList<CartridgeInfo> cartridges;
    
    common::database::LocalDBManager& dbManager = common::database::LocalDBManager::getInstance();
    if (!dbManager.isOpen()) {
        qWarning() << "Database not open for library load";
        return cartridges;
    }
    
    QSqlQuery query(dbManager.getDatabase());
    query.prepare(R"(
        SELECT cartridge_guid, title, author, publication_year, 
               publisher, version, local_path, cover_image_data
        FROM Local_Library_Manifest
        ORDER BY title
    )");
    
    if (!query.exec()) {
        qWarning() << "Failed to load library:" << query.lastError().text();
        return cartridges;
    }
    
    while (query.next()) {
        CartridgeInfo info;
        info.cartridgeGuid = query.value(0).toString();
        info.title = query.value(1).toString();
        info.author = query.value(2).toString();
        info.publicationYear = query.value(3).toString();
        info.publisher = query.value(4).toString();
        info.version = query.value(5).toString();
        info.localPath = query.value(6).toString();
        info.coverImageData = query.value(7).toByteArray();
        
        if (info.isValid()) {
            cartridges.append(info);
        }
    }
    
    return cartridges;
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
    // Show file picker dialog
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Import Cartridge",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "SmartBook Cartridges (*.sqlite);;All Files (*)"
    );

    if (filePaths.isEmpty()) {
        return;
    }

    // Filter to only .sqlite files
    QStringList sqliteFiles;
    for (const QString& filePath : filePaths) {
        if (filePath.endsWith(".sqlite", Qt::CaseInsensitive)) {
            sqliteFiles.append(filePath);
        }
    }

    if (sqliteFiles.isEmpty()) {
        QMessageBox::warning(this, "Import Cartridge",
            "No valid cartridge files selected. Please select .sqlite files.");
        return;
    }

    // Show import progress dialog
    ui::ImportDialog* importDialog = new ui::ImportDialog(this);
    importDialog->setFiles(sqliteFiles);
    importDialog->show();

    // Get library path
    QString libraryPath = m_importManager->getLibraryPath();

    // Import cartridges
    QList<ImportManager::ImportResultInfo> results = 
        m_importManager->importCartridges(sqliteFiles, libraryPath, importDialog);

    // Refresh library view
    loadLibrary();

    // Close dialog
    importDialog->close();
    delete importDialog;
}

void LibraryManager::onDeleteCartridge(const QString& /* cartridgeGuid */) {
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

void LibraryManager::setThemeMenuSelection(const QString& theme) {
    if (!m_themeGroup) {
        qWarning() << "LibraryManager::setThemeMenuSelection: Theme group not initialized";
        return;
    }
    
    // Find action with matching data
    for (QAction* action : m_themeGroup->actions()) {
        if (action->data().toString() == theme) {
            action->setChecked(true);
            qDebug() << "LibraryManager::setThemeMenuSelection: Set menu to" << theme;
            return;
        }
    }
    
    qWarning() << "LibraryManager::setThemeMenuSelection: Theme" << theme << "not found in menu";
}

void LibraryManager::applyTheme() {
    QString theme = common::utils::ThemeManager::getInstance().getTheme();
    QString resolvedTheme = common::utils::ThemeManager::getInstance().resolveTheme(theme);
    
    qDebug() << "LibraryManager::applyTheme: Applying theme" << resolvedTheme;
    
    QColor bgColor, textColor;
    
    if (resolvedTheme == "dark") {
        bgColor = QColor(30, 30, 30);
        textColor = QColor(212, 212, 212);
    } else if (resolvedTheme == "sepia") {
        bgColor = QColor(244, 236, 216);
        textColor = QColor(92, 75, 55);
    } else { // light (default)
        bgColor = QColor(255, 255, 255);
        textColor = QColor(0, 0, 0);
    }
    
    // Apply theme to the main window
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, bgColor);
    palette.setColor(QPalette::WindowText, textColor);
    palette.setColor(QPalette::Base, bgColor);
    palette.setColor(QPalette::Text, textColor);
    palette.setColor(QPalette::Button, bgColor);
    palette.setColor(QPalette::ButtonText, textColor);
    this->setPalette(palette);
    
    // Also apply to central widget if it exists
    if (m_libraryView) {
        QPalette widgetPalette = m_libraryView->palette();
        widgetPalette.setColor(QPalette::Window, bgColor);
        widgetPalette.setColor(QPalette::WindowText, textColor);
        widgetPalette.setColor(QPalette::Base, bgColor);
        widgetPalette.setColor(QPalette::Text, textColor);
        m_libraryView->setPalette(widgetPalette);
    }
    
    qDebug() << "LibraryManager::applyTheme: Theme applied successfully";
}

} // namespace reader
} // namespace smartbook
