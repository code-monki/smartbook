#include "smartbook/reader/ReaderViewWindow.h"
#include "smartbook/reader/ui/ReaderView.h"
#include "smartbook/reader/ui/ConsentDialog.h"
#include "smartbook/reader/ui/SecurityErrorDialog.h"
#include "smartbook/reader/WebChannelBridge.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include "smartbook/common/security/TrustRegistry.h"
#include <QCloseEvent>
#include <QSqlQuery>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>

namespace smartbook {
namespace reader {

ReaderViewWindow::ReaderViewWindow(const QString& cartridgeGuid, QWidget* parent)
    : QMainWindow(parent)
    , m_cartridgeGuid(cartridgeGuid)
    , m_readerView(nullptr)
    , m_webChannelBridge(nullptr)
    , m_signatureVerifier(new common::security::SignatureVerifier(this))
    , m_trustRegistry(new common::security::TrustRegistry(this))
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
    
    // Perform security verification
    if (!performSecurityVerification(cartridgePath)) {
        // Security verification failed - error dialog already shown
        return;
    }
    
    // Load content (with cartridge GUID for settings)
    if (m_readerView) {
        m_readerView->loadCartridge(cartridgePath, m_cartridgeGuid);
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
    // TODO: Save window state to Local_Window_State table
    // TODO: Save reading position to Local_Reading_Position table
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

} // namespace reader
} // namespace smartbook
