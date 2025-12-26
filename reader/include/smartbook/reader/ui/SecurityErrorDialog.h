#ifndef SMARTBOOK_READER_UI_SECURITYERRORDIALOG_H
#define SMARTBOOK_READER_UI_SECURITYERRORDIALOG_H

#include <QDialog>
#include <QString>

namespace smartbook {
namespace reader {
namespace ui {

/**
 * @brief Security error dialog types
 */
enum class SecurityErrorType {
    TamperingDetected,      // H1 != H2 or fingerprint mismatch
    InvalidCertificate,     // Certificate validation failed
    FileCorruption,         // Database corruption or invalid format
    SignatureInvalid        // Digital signature verification failed
};

/**
 * @brief Security error dialog for displaying security violations
 * 
 * Displays user-friendly error messages for security issues that prevent
 * cartridge loading. These errors are non-recoverable and block cartridge access.
 */
class SecurityErrorDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param errorType Type of security error
     * @param cartridgeTitle Title of the cartridge (if known)
     * @param errorDetails Additional error details
     * @param parent Parent widget
     */
    explicit SecurityErrorDialog(
        SecurityErrorType errorType,
        const QString& cartridgeTitle = QString(),
        const QString& errorDetails = QString(),
        QWidget* parent = nullptr
    );
    ~SecurityErrorDialog();

private:
    void setupUI(SecurityErrorType errorType, const QString& cartridgeTitle, const QString& errorDetails);
    QString getErrorTitle(SecurityErrorType errorType);
    QString getErrorMessage(SecurityErrorType errorType, const QString& cartridgeTitle, const QString& errorDetails);
};

} // namespace ui
} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_UI_SECURITYERRORDIALOG_H

