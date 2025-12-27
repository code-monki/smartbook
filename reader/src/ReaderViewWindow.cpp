#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/ui/ConsentDialog.h"
#include "smartbook/reader/ui/SecurityErrorDialog.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include "smartbook/common/security/TrustRegistry.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/metadata/MetadataExtractor.h"
#include <QCloseEvent>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QScreen>
#include <QApplication>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QDebug>

namespace smartbook {
namespace reader {

ReaderViewWindow::ReaderViewWindow(const QString& cartridgeGuid, QWidget* parent)
    : QMainWindow(parent)
    , m_cartridgeGuid(cartridgeGuid)
    , m_readerView(nullptr)
    , m_signatureVerifier(new common::security::SignatureVerifier(this))
    , m_trustRegistry(new common::security::TrustRegistry(this))
{
    setupUI();
    restoreWindowState();
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
    
    // Create menu bar with theme selector
    QMenuBar* menuBar = this->menuBar();
    
    // View menu
    QMenu* viewMenu = menuBar->addMenu("View");
    
    // Theme submenu
    QMenu* themeMenu = viewMenu->addMenu("Theme");
    QActionGroup* themeGroup = new QActionGroup(this);
    
    QAction* lightAction = themeMenu->addAction("Light");
    lightAction->setCheckable(true);
    lightAction->setChecked(true);
    lightAction->setData("light");
    themeGroup->addAction(lightAction);
    
    QAction* darkAction = themeMenu->addAction("Dark");
    darkAction->setCheckable(true);
    darkAction->setData("dark");
    themeGroup->addAction(darkAction);
    
    QAction* sepiaAction = themeMenu->addAction("Sepia");
    sepiaAction->setCheckable(true);
    sepiaAction->setData("sepia");
    themeGroup->addAction(sepiaAction);
    
    // Connect theme actions
    connect(themeGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        QString theme = action->data().toString();
        if (m_readerView) {
            // Get settings manager from ReaderView and update theme
            // We need to access the SettingsManager through ReaderView
            // For now, we'll add a method to ReaderView to change theme
            m_readerView->setTheme(theme);
        }
    });
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
    
    // Perform security verification
    if (!performSecurityVerification(cartridgePath)) {
        // Security verification failed - error dialog already shown
        return;
    }
    
    // Update manifest (update hash and last_opened timestamp)
    updateManifest(cartridgePath);
    
    // Load content (with cartridge GUID for settings)
    if (m_readerView) {
        m_readerView->loadCartridge(cartridgePath, m_cartridgeGuid);
        
        // Restore reading position if available
        if (m_restoredPageId >= 0) {
            m_readerView->loadPage(m_restoredPageId);
            // TODO: Restore scroll position and anchor (requires JavaScript bridge)
        }
    }
}

void ReaderViewWindow::onContentLoaded() {
    setWindowTitle("SmartBook Reader - " + m_cartridgeGuid);
}

void ReaderViewWindow::onError(const QString& errorMessage) {
    qWarning() << "Reader View error:" << errorMessage;
    QMessageBox::critical(this, "Error", "Failed to load cartridge:\n\n" + errorMessage);
}

void ReaderViewWindow::closeEvent(QCloseEvent* event) {
    saveWindowState();
    QMainWindow::closeEvent(event);
}

void ReaderViewWindow::saveWindowState() {
    // DDD Section: Window State Persistence
    // Save window state and reading position atomically
    
    common::database::LocalDBManager& dbManager = 
        common::database::LocalDBManager::getInstance();
    
    if (!dbManager.isOpen()) {
        qWarning() << "Local database not open, cannot save window state";
        return;
    }
    
    QSqlDatabase db = dbManager.getDatabase();
    
    // Begin transaction for atomic save
    if (!db.transaction()) {
        qWarning() << "Failed to begin transaction for window state save";
        return;
    }
    
    try {
        // Save window state
        QRect geometry = this->geometry();
        bool isMaximized = this->isMaximized();
        qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        
        QSqlQuery query(db);
        query.prepare(R"(
            INSERT OR REPLACE INTO Local_Window_State
            (cartridge_guid, window_width, window_height, window_x, window_y, is_maximized, last_updated)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        )");
        query.addBindValue(m_cartridgeGuid);
        query.addBindValue(geometry.width());
        query.addBindValue(geometry.height());
        query.addBindValue(geometry.x());
        query.addBindValue(geometry.y());
        query.addBindValue(isMaximized ? 1 : 0);
        query.addBindValue(timestamp);
        
        if (!query.exec()) {
            qWarning() << "Failed to save window state:" << query.lastError().text();
            db.rollback();
            return;
        }
        
        // Save reading position
        int currentPageId = -1;
        if (m_readerView) {
            currentPageId = m_readerView->getCurrentPageId();
        }
        
        // TODO: Get scroll position from WebEngine view (requires JavaScript bridge)
        int scrollPosition = 0;
        QString anchorId; // TODO: Get anchor ID from WebEngine view
        
        query.prepare(R"(
            INSERT OR REPLACE INTO Local_Reading_Position
            (cartridge_guid, page_id, anchor_id, scroll_position, last_access_timestamp)
            VALUES (?, ?, ?, ?, ?)
        )");
        query.addBindValue(m_cartridgeGuid);
        query.addBindValue(currentPageId >= 0 ? currentPageId : 1); // Default to page 1 if no page loaded
        query.addBindValue(anchorId);
        query.addBindValue(scrollPosition);
        query.addBindValue(timestamp);
        
        if (!query.exec()) {
            qWarning() << "Failed to save reading position:" << query.lastError().text();
            db.rollback();
            return;
        }
        
        // Commit transaction
        if (!db.commit()) {
            qWarning() << "Failed to commit window state transaction";
            db.rollback();
        }
    } catch (...) {
        qWarning() << "Exception during window state save";
        db.rollback();
    }
}

void ReaderViewWindow::restoreWindowState() {
    // DDD Section: Window State Restoration
    // Restore window geometry and reading position
    
    common::database::LocalDBManager& dbManager = 
        common::database::LocalDBManager::getInstance();
    
    if (!dbManager.isOpen()) {
        // Use default geometry
        resize(1024, 768);
        centerWindow();
        return;
    }
    
    QSqlDatabase db = dbManager.getDatabase();
    QSqlQuery query(db);
    
    // Restore window geometry
    query.prepare(R"(
        SELECT window_width, window_height, window_x, window_y, is_maximized
        FROM Local_Window_State
        WHERE cartridge_guid = ?
    )");
    query.addBindValue(m_cartridgeGuid);
    
    if (query.exec() && query.next()) {
        int width = query.value(0).toInt();
        int height = query.value(1).toInt();
        int x = query.value(2).toInt();
        int y = query.value(3).toInt();
        bool isMaximized = query.value(4).toInt() != 0;
        
        // Validate geometry
        QScreen* screen = QApplication::primaryScreen();
        QRect screenGeometry = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
        
        // Ensure reasonable size (minimum 400x300, maximum screen size)
        width = qBound(400, width, screenGeometry.width());
        height = qBound(300, height, screenGeometry.height());
        
        // Ensure position is within screen bounds
        if (x < 0 || y < 0 || x + width > screenGeometry.width() || y + height > screenGeometry.height()) {
            // Center window if position is invalid
            centerWindow();
        } else {
            setGeometry(x, y, width, height);
        }
        
        // Restore maximized state
        if (isMaximized) {
            showMaximized();
        }
    } else {
        // No saved state, use default geometry
        resize(1024, 768);
        centerWindow();
    }
    
    // Restore reading position (will be applied after content loads)
    query.prepare(R"(
        SELECT page_id, anchor_id, scroll_position
        FROM Local_Reading_Position
        WHERE cartridge_guid = ?
    )");
    query.addBindValue(m_cartridgeGuid);
    
    if (query.exec() && query.next()) {
        int savedPageId = query.value(0).toInt();
        QString anchorId = query.value(1).toString();
        int scrollPosition = query.value(2).toInt();
        
        // Store for later use when content loads
        m_restoredPageId = savedPageId;
        m_restoredAnchorId = anchorId;
        m_restoredScrollPosition = scrollPosition;
    }
}

void ReaderViewWindow::centerWindow() {
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        QRect windowGeometry = geometry();
        int x = (screenGeometry.width() - windowGeometry.width()) / 2 + screenGeometry.x();
        int y = (screenGeometry.height() - windowGeometry.height()) / 2 + screenGeometry.y();
        move(x, y);
    }
}

bool ReaderViewWindow::performSecurityVerification(const QString& cartridgePath)
{
    // Step 1: Verify cartridge signature
    common::security::VerificationResult result = m_signatureVerifier->verifyCartridge(cartridgePath, m_cartridgeGuid);
    
    // Step 2: Handle tampering detection
    if (result.isTampered) {
        handleSecurityError(ui::SecurityErrorType::TamperingDetected, 
                          "Content hash mismatch detected. H1 != H2");
        return false;
    }
    
    // Step 3: Handle rejected policy (invalid signature, fingerprint mismatch, etc.)
    if (result.effectivePolicy == common::security::TrustPolicy::REJECTED) {
        if (!result.errorMessage.isEmpty()) {
            handleSecurityError(ui::SecurityErrorType::SignatureInvalid, result.errorMessage);
        } else {
            handleSecurityError(ui::SecurityErrorType::InvalidCertificate, 
                              "Certificate validation failed or signature invalid");
        }
        return false;
    }
    
    // Step 4: Get cartridge metadata for consent dialog
    QString cartridgeTitle;
    QString authorName;
    
    common::database::CartridgeDBConnector connector(this);
    if (connector.openCartridge(cartridgePath)) {
        QSqlQuery query(connector.getDatabase());
        if (query.exec("SELECT title, author FROM Metadata LIMIT 1") && query.next()) {
            cartridgeTitle = query.value(0).toString();
            authorName = query.value(1).toString();
        }
        connector.closeCartridge();
    }
    
    // Step 5: Handle consent requirement for L2/L3 cartridges
    if (result.effectivePolicy == common::security::TrustPolicy::CONSENT_REQUIRED) {
        handleConsentRequired(result.securityLevel, cartridgeTitle, authorName);
        
        // Check if window is still open (user didn't cancel)
        // If window was closed, handleConsentRequired called close()
        // We can check if the window is visible or if trust was revoked
        if (!isVisible()) {
            return false; // User cancelled, window was closed
        }
        
        // Check if trust was revoked after consent dialog
        if (m_trustRegistry->getTrustDecision(m_cartridgeGuid) == 
            common::security::TrustRegistry::TrustPolicy::REVOKED) {
            return false; // Trust was revoked
        }
    }
    
    return true;
}

void ReaderViewWindow::handleSecurityError(reader::ui::SecurityErrorType errorType, const QString& errorDetails)
{
    // Get cartridge title for error dialog
    QString cartridgeTitle;
    common::database::LocalDBManager& dbManager = 
        common::database::LocalDBManager::getInstance();
    
    if (dbManager.isOpen()) {
        QSqlQuery query(dbManager.getDatabase());
        query.prepare("SELECT title FROM Local_Library_Manifest WHERE cartridge_guid = ?");
        query.addBindValue(m_cartridgeGuid);
        if (query.exec() && query.next()) {
            cartridgeTitle = query.value(0).toString();
        }
    }
    
    // Show security error dialog
    reader::ui::SecurityErrorDialog errorDialog(errorType, cartridgeTitle, errorDetails, this);
    errorDialog.exec();
    
    // Close the window since we can't load the cartridge
    close();
}

void ReaderViewWindow::handleConsentRequired(
    common::security::SecurityLevel level,
    const QString& cartridgeTitle,
    const QString& authorName)
{
    // Show consent dialog
    reader::ui::ConsentDialog consentDialog(level, cartridgeTitle, authorName, this);
    
    int dialogResult = consentDialog.exec();
    reader::ui::ConsentDialog::ConsentResult userChoice = consentDialog.getResult();
    
    if (dialogResult == QDialog::Accepted && userChoice != reader::ui::ConsentDialog::Cancel) {
        // User chose to load the cartridge
        if (userChoice == reader::ui::ConsentDialog::LoadAndAlwaysTrust) {
            // Store persistent trust
            m_trustRegistry->storeTrustDecision(
                m_cartridgeGuid, 
                common::security::TrustRegistry::TrustPolicy::PERSISTENT
            );
        } else if (userChoice == reader::ui::ConsentDialog::LoadForSessionOnly) {
            // Store session trust
            m_trustRegistry->storeTrustDecision(
                m_cartridgeGuid,
                common::security::TrustRegistry::TrustPolicy::SESSION
            );
        }
        // Continue loading - return true (handled by caller)
    } else {
        // User cancelled or closed dialog - don't load cartridge
        close();
    }
}

void ReaderViewWindow::updateManifest(const QString& cartridgePath)
{
    // Update manifest entry with current hash and last_opened timestamp
    common::manifest::ManifestManager manifestManager;
    
    if (!manifestManager.manifestEntryExists(m_cartridgeGuid)) {
        // Manifest entry doesn't exist - this shouldn't happen if cartridge was imported
        // but we'll create it anyway
        qWarning() << "Manifest entry not found for cartridge:" << m_cartridgeGuid;
        
        // Extract metadata and create manifest entry
        common::metadata::CartridgeMetadata metadata = 
            common::metadata::MetadataExtractor::extractMetadata(cartridgePath);
        
        if (metadata.cartridgeGuid.isEmpty()) {
            qWarning() << "Failed to extract metadata for manifest update";
            return;
        }
        
        QByteArray contentHash = common::metadata::MetadataExtractor::calculateContentHash(cartridgePath);
        
        common::manifest::ManifestManager::ManifestEntry entry;
        entry.cartridgeGuid = metadata.cartridgeGuid;
        entry.cartridgeHash = contentHash;
        entry.localPath = cartridgePath;
        entry.title = metadata.title;
        entry.author = metadata.author;
        entry.publisher = metadata.publisher;
        entry.version = metadata.version;
        entry.publicationYear = metadata.publicationYear;
        entry.coverImageData = metadata.coverImageData;
        
        manifestManager.createManifestEntry(entry);
        return;
    }
    
    // Get existing entry
    common::manifest::ManifestManager::ManifestEntry entry = 
        manifestManager.getManifestEntry(m_cartridgeGuid);
    
    if (!entry.isValid()) {
        qWarning() << "Invalid manifest entry for cartridge:" << m_cartridgeGuid;
        return;
    }
    
    // Recalculate content hash (H2) and update
    QByteArray contentHash = common::metadata::MetadataExtractor::calculateContentHash(cartridgePath);
    entry.cartridgeHash = contentHash;
    
    // Update manifest entry
    manifestManager.updateManifestEntry(entry);
    
    // Note: last_opened timestamp would be updated here if we had that field
    // For now, we're updating the hash to detect tampering on next load
}

} // namespace reader
} // namespace smartbook
