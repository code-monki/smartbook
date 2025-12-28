#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/ui/ConsentDialog.h"
#include "smartbook/reader/ui/SecurityErrorDialog.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/utils/ThemeManager.h"
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
#include <QKeySequence>
#include <QDebug>

namespace smartbook {
namespace reader {

ReaderViewWindow::ReaderViewWindow(const QString& cartridgeGuid, QWidget* parent)
    : QMainWindow(parent)
    , m_cartridgeGuid(cartridgeGuid)
    , m_readerView(nullptr)
    , m_signatureVerifier(new common::security::SignatureVerifier(this))
    , m_trustRegistry(new common::security::TrustRegistry(this))
    , m_themeGroup(nullptr)
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
    
    QAction* autoAction = themeMenu->addAction("Auto");
    autoAction->setCheckable(true);
    autoAction->setData("auto");
    themeGroup->addAction(autoAction);
    
    QAction* lightAction = themeMenu->addAction("Light");
    lightAction->setCheckable(true);
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
    
    // Store theme group for later access
    m_themeGroup = themeGroup;
    
    // Load saved theme preference and set menu selection
    QString savedTheme = common::utils::ThemeManager::getInstance().getTheme();
    setThemeMenuSelection(savedTheme);
    
    // Connect theme actions
    connect(themeGroup, &QActionGroup::triggered, this, [](QAction* action) {
        QString theme = action->data().toString();
        qDebug() << "ReaderViewWindow: Theme menu triggered, theme=" << theme;
        
        // Save global preference (shared across all apps)
        // This will emit themeChanged signal, which we listen to below
        common::utils::ThemeManager::getInstance().setTheme(theme);
    });
    
    // Connect to theme change notifications (from any window)
    connect(&common::utils::ThemeManager::getInstance(), &common::utils::ThemeManager::themeChanged,
            this, [this](const QString& theme) {
                qDebug() << "ReaderViewWindow: Theme changed to" << theme << ", updating UI";
                setThemeMenuSelection(theme);
                if (m_readerView) {
                    m_readerView->setTheme(theme);
                    qDebug() << "ReaderViewWindow: Theme applied to ReaderView";
                } else {
                    qWarning() << "ReaderViewWindow: m_readerView is null, cannot set theme";
                }
            });
    
    viewMenu->addSeparator();
    
    // Navigation actions
    QAction* nextPageAction = viewMenu->addAction("Next Page");
    nextPageAction->setShortcut(QKeySequence::MoveToNextPage);
    connect(nextPageAction, &QAction::triggered, this, [this]() {
        if (m_readerView) {
            int currentPage = m_readerView->getCurrentPageId();
            if (currentPage >= 1) {
                m_readerView->loadPage(currentPage + 1);
            }
        }
    });
    
    QAction* prevPageAction = viewMenu->addAction("Previous Page");
    prevPageAction->setShortcut(QKeySequence::MoveToPreviousPage);
    connect(prevPageAction, &QAction::triggered, this, [this]() {
        if (m_readerView) {
            int currentPage = m_readerView->getCurrentPageId();
            if (currentPage > 1) {
                m_readerView->loadPage(currentPage - 1);
            } else if (currentPage == -1) {
                // If on first page (pageId -1), go to page 1 explicitly
                m_readerView->loadPage(1);
            }
        }
    });
    
    // Quick navigation to page 2 for testing
    QAction* goToPage2Action = viewMenu->addAction("Go to Page 2 (Test Form)");
    connect(goToPage2Action, &QAction::triggered, this, [this]() {
        if (m_readerView) {
            m_readerView->loadPage(2);
        }
    });
}

void ReaderViewWindow::loadCartridge() {
    qDebug() << "ReaderViewWindow::loadCartridge: Starting, cartridgeGuid=" << m_cartridgeGuid;
    
    // Get cartridge path from manifest
    common::database::LocalDBManager& dbManager = 
        common::database::LocalDBManager::getInstance();
    
    if (!dbManager.isOpen()) {
        QString error = "Local database not open";
        qWarning() << "ReaderViewWindow::loadCartridge:" << error;
        emit onError(error);
        return;
    }
    
    // Query manifest for cartridge path
    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT local_path FROM Local_Library_Manifest WHERE cartridge_guid = ?");
    query.addBindValue(m_cartridgeGuid);
    
    if (!query.exec()) {
        QString error = "Failed to query manifest: " + query.lastError().text();
        qWarning() << "ReaderViewWindow::loadCartridge:" << error;
        emit onError(error);
        return;
    }
    
    if (!query.next()) {
        QString error = "Cartridge not found in manifest: " + m_cartridgeGuid;
        qWarning() << "ReaderViewWindow::loadCartridge:" << error;
        emit onError(error);
        return;
    }
    
    QString cartridgePath = query.value(0).toString();
    qDebug() << "ReaderViewWindow::loadCartridge: Found cartridge path:" << cartridgePath;
    
    if (cartridgePath.isEmpty()) {
        QString error = "Cartridge path is empty for: " + m_cartridgeGuid;
        qWarning() << "ReaderViewWindow::loadCartridge:" << error;
        emit onError(error);
        return;
    }
    
    // Perform security verification
    qDebug() << "ReaderViewWindow::loadCartridge: Performing security verification...";
    if (!performSecurityVerification(cartridgePath)) {
        qWarning() << "ReaderViewWindow::loadCartridge: Security verification failed";
        // Security verification failed - error dialog already shown
        return;
    }
    qDebug() << "ReaderViewWindow::loadCartridge: Security verification passed";
    
    // Update manifest (update hash and last_opened timestamp)
    updateManifest(cartridgePath);
    
    // Load content (with cartridge GUID for settings)
    if (m_readerView) {
        qDebug() << "ReaderViewWindow::loadCartridge: Calling m_readerView->loadCartridge()";
        m_readerView->loadCartridge(cartridgePath, m_cartridgeGuid);
        
        // Restore reading position if available
        if (m_restoredPageId >= 0) {
            qDebug() << "ReaderViewWindow::loadCartridge: Restoring page" << m_restoredPageId;
            m_readerView->loadPage(m_restoredPageId);
            // TODO: Restore scroll position and anchor (requires JavaScript bridge)
        }
    } else {
        qWarning() << "ReaderViewWindow::loadCartridge: m_readerView is null!";
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
    qDebug() << "ReaderViewWindow::performSecurityVerification: Starting verification for" << cartridgePath;
    
    // Step 1: Verify cartridge signature
    common::security::VerificationResult result = m_signatureVerifier->verifyCartridge(cartridgePath, m_cartridgeGuid);
    
    qDebug() << "ReaderViewWindow::performSecurityVerification: Verification result:";
    qDebug() << "  - Security Level:" << static_cast<int>(result.securityLevel);
    qDebug() << "  - Effective Policy:" << static_cast<int>(result.effectivePolicy);
    qDebug() << "  - Is Tampered:" << result.isTampered;
    qDebug() << "  - Error Message:" << result.errorMessage;
    
    // Step 2: Handle tampering detection
    if (result.isTampered) {
        qWarning() << "ReaderViewWindow::performSecurityVerification: Tampering detected";
        handleSecurityError(ui::SecurityErrorType::TamperingDetected, 
                          "Content hash mismatch detected. H1 != H2");
        return false;
    }
    
    // Step 3: Handle rejected policy (invalid signature, fingerprint mismatch, etc.)
    if (result.effectivePolicy == common::security::TrustPolicy::REJECTED) {
        qWarning() << "ReaderViewWindow::performSecurityVerification: Policy REJECTED";
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
        qDebug() << "ReaderViewWindow::performSecurityVerification: Consent required, showing dialog";
        qDebug() << "ReaderViewWindow::performSecurityVerification: Cartridge title:" << cartridgeTitle << ", author:" << authorName;
        
        // Store trust decision before showing dialog to check after
        auto trustBefore = m_trustRegistry->getTrustDecision(m_cartridgeGuid);
        qDebug() << "ReaderViewWindow::performSecurityVerification: Trust decision before consent:" << static_cast<int>(trustBefore);
        
        handleConsentRequired(result.securityLevel, cartridgeTitle, authorName);
        
        // Check if trust was stored (user accepted) or if window was closed (user cancelled)
        // Note: Window might not be visible yet if called during construction, so check trust decision instead
        auto trustDecision = m_trustRegistry->getTrustDecision(m_cartridgeGuid);
        qDebug() << "ReaderViewWindow::performSecurityVerification: Trust decision after consent:" << static_cast<int>(trustDecision);
        
        // If trust decision is still the same as before (and not PERSISTENT/SESSION), user cancelled
        if (trustDecision == trustBefore && 
            trustDecision != common::security::TrustRegistry::TrustPolicy::PERSISTENT &&
            trustDecision != common::security::TrustRegistry::TrustPolicy::SESSION) {
            qWarning() << "ReaderViewWindow::performSecurityVerification: No trust stored (user cancelled)";
            return false; // User cancelled
        }
        
        // Check if trust was revoked after consent dialog
        if (trustDecision == common::security::TrustRegistry::TrustPolicy::REVOKED) {
            qWarning() << "ReaderViewWindow::performSecurityVerification: Trust was revoked";
            return false; // Trust was revoked
        }
        
        // Also check if window was explicitly closed (user cancelled via close button)
        if (!isVisible() && trustDecision == trustBefore) {
            qWarning() << "ReaderViewWindow::performSecurityVerification: Window closed and no trust stored (user cancelled)";
            return false; // User cancelled, window was closed
        }
        
        qDebug() << "ReaderViewWindow::performSecurityVerification: Consent granted, proceeding";
    }
    
    qDebug() << "ReaderViewWindow::performSecurityVerification: Verification successful";
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
    qDebug() << "ReaderViewWindow::handleConsentRequired: Showing consent dialog";
    qDebug() << "ReaderViewWindow::handleConsentRequired: Level=" << static_cast<int>(level) 
             << ", Title=" << cartridgeTitle << ", Author=" << authorName;
    
    // Show consent dialog
    reader::ui::ConsentDialog consentDialog(level, cartridgeTitle, authorName, this);
    
    qDebug() << "ReaderViewWindow::handleConsentRequired: Executing dialog...";
    int dialogResult = consentDialog.exec();
    reader::ui::ConsentDialog::ConsentResult userChoice = consentDialog.getResult();
    
    qDebug() << "ReaderViewWindow::handleConsentRequired: Dialog result=" << dialogResult 
             << ", User choice=" << static_cast<int>(userChoice);
    
    if (dialogResult == QDialog::Accepted && userChoice != reader::ui::ConsentDialog::Cancel) {
        qDebug() << "ReaderViewWindow::handleConsentRequired: User accepted, storing trust decision";
        // User chose to load the cartridge
        if (userChoice == reader::ui::ConsentDialog::LoadAndAlwaysTrust) {
            // Store persistent trust
            m_trustRegistry->storeTrustDecision(
                m_cartridgeGuid, 
                common::security::TrustRegistry::TrustPolicy::PERSISTENT
            );
            qDebug() << "ReaderViewWindow::handleConsentRequired: Stored PERSISTENT trust";
        } else if (userChoice == reader::ui::ConsentDialog::LoadForSessionOnly) {
            // Store session trust
            m_trustRegistry->storeTrustDecision(
                m_cartridgeGuid,
                common::security::TrustRegistry::TrustPolicy::SESSION
            );
            qDebug() << "ReaderViewWindow::handleConsentRequired: Stored SESSION trust";
        }
        // Continue loading - return true (handled by caller)
    } else {
        // User cancelled or closed dialog - don't load cartridge
        qWarning() << "ReaderViewWindow::handleConsentRequired: User cancelled or rejected, closing window";
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

QString ReaderViewWindow::loadGlobalThemePreference()
{
    // Use ThemeManager for global theme preference
    return common::utils::ThemeManager::getInstance().getTheme();
}

void ReaderViewWindow::saveGlobalThemePreference(const QString& theme)
{
    // Use ThemeManager for global theme preference
    common::utils::ThemeManager::getInstance().setTheme(theme);
}

void ReaderViewWindow::setThemeMenuSelection(const QString& theme)
{
    if (!m_themeGroup) {
        qWarning() << "ReaderViewWindow::setThemeMenuSelection: Theme group not initialized";
        return;
    }
    
    // Find action with matching data
    for (QAction* action : m_themeGroup->actions()) {
        if (action->data().toString() == theme) {
            action->setChecked(true);
            qDebug() << "ReaderViewWindow::setThemeMenuSelection: Set menu to" << theme;
            return;
        }
    }
    
    qWarning() << "ReaderViewWindow::setThemeMenuSelection: Theme" << theme << "not found in menu";
}

} // namespace reader
} // namespace smartbook
