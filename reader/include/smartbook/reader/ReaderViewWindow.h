#ifndef SMARTBOOK_READER_READERVIEWWINDOW_H
#define SMARTBOOK_READER_READERVIEWWINDOW_H

#include <QMainWindow>
#include <QString>
#include <memory>
#include "smartbook/common/security/SignatureVerifier.h"
#include "smartbook/reader/ui/SecurityErrorDialog.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/metadata/MetadataExtractor.h"

namespace smartbook {
namespace common {
namespace security {
    class TrustRegistry;
}
}

namespace reader {
namespace ui {
    class ConsentDialog;
}

class ReaderView;
class WebChannelBridge;

/**
 * @brief Reader View Window - container for a single opened cartridge
 * 
 * Owns the web view and data connector. Enforces security policy and
 * data isolation. Each cartridge gets its own Reader View Window.
 */
class ReaderViewWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ReaderViewWindow(const QString& cartridgeGuid, QWidget* parent = nullptr);
    ~ReaderViewWindow();

    /**
     * @brief Get the cartridge GUID
     * @return Cartridge GUID
     */
    QString getCartridgeGuid() const { return m_cartridgeGuid; }

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onContentLoaded();
    void onError(const QString& errorMessage);

private:
    void setupUI();
    void loadCartridge();
    void saveWindowState();
    bool performSecurityVerification(const QString& cartridgePath);
    void handleSecurityError(ui::SecurityErrorType errorType, const QString& errorDetails);
    void handleConsentRequired(common::security::SecurityLevel level, const QString& cartridgeTitle, const QString& authorName);
    void updateManifest(const QString& cartridgePath);

    QString m_cartridgeGuid;
    ReaderView* m_readerView;
    WebChannelBridge* m_webChannelBridge;
    common::security::SignatureVerifier* m_signatureVerifier;
    common::security::TrustRegistry* m_trustRegistry;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_READERVIEWWINDOW_H
