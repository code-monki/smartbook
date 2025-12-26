#include "smartbook/reader/ui/SecurityErrorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QIcon>
#include <QMessageBox>
#include <QStyle>

namespace smartbook {
namespace reader {
namespace ui {

SecurityErrorDialog::SecurityErrorDialog(
    SecurityErrorType errorType,
    const QString& cartridgeTitle,
    const QString& errorDetails,
    QWidget* parent)
    : QDialog(parent)
{
    setupUI(errorType, cartridgeTitle, errorDetails);
}

SecurityErrorDialog::~SecurityErrorDialog() {
}

void SecurityErrorDialog::setupUI(
    SecurityErrorType errorType,
    const QString& cartridgeTitle,
    const QString& errorDetails)
{
    setWindowTitle(getErrorTitle(errorType));
    setMinimumWidth(500);
    resize(500, 350);
    setModal(true);
    
    // Set window icon to critical/warning
    QStyle* style = this->style();
    QIcon errorIcon = style->standardIcon(QStyle::SP_MessageBoxCritical);
    setWindowIcon(errorIcon);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);

    // Error icon and title
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setPixmap(errorIcon.pixmap(48, 48));
    titleLayout->addWidget(iconLabel);
    
    QLabel* titleLabel = new QLabel(getErrorTitle(errorType), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 16pt; color: #d32f2f;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    layout->addLayout(titleLayout);

    // Error message
    QTextEdit* messageText = new QTextEdit(this);
    messageText->setReadOnly(true);
    messageText->setPlainText(getErrorMessage(errorType, cartridgeTitle, errorDetails));
    messageText->setStyleSheet("background-color: #ffebee; border: 1px solid #d32f2f; padding: 10px;");
    layout->addWidget(messageText);

    // OK button
    QPushButton* okButton = new QPushButton("OK", this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    layout->addLayout(buttonLayout);
}

QString SecurityErrorDialog::getErrorTitle(SecurityErrorType errorType)
{
    switch (errorType) {
        case SecurityErrorType::TamperingDetected:
            return "Security Error: Content Tampering Detected";
        case SecurityErrorType::InvalidCertificate:
            return "Security Error: Invalid Certificate";
        case SecurityErrorType::FileCorruption:
            return "Security Error: File Corruption";
        case SecurityErrorType::SignatureInvalid:
            return "Security Error: Invalid Digital Signature";
        default:
            return "Security Error";
    }
}

QString SecurityErrorDialog::getErrorMessage(
    SecurityErrorType errorType,
    const QString& cartridgeTitle,
    const QString& errorDetails)
{
    QString message;
    
    switch (errorType) {
        case SecurityErrorType::TamperingDetected:
            message = "Content tampering has been detected in this cartridge.\n\n";
            if (!cartridgeTitle.isEmpty()) {
                message += QString("Cartridge: %1\n\n").arg(cartridgeTitle);
            }
            message += "The cartridge's content hash does not match the stored signature hash. "
                      "This indicates that the cartridge content has been modified after it was signed.\n\n";
            message += "For security reasons, this cartridge cannot be loaded.\n\n";
            if (!errorDetails.isEmpty()) {
                message += QString("Details: %1").arg(errorDetails);
            }
            break;
            
        case SecurityErrorType::InvalidCertificate:
            message = "The cartridge's digital certificate is invalid or cannot be verified.\n\n";
            if (!cartridgeTitle.isEmpty()) {
                message += QString("Cartridge: %1\n\n").arg(cartridgeTitle);
            }
            message += "The certificate may be:\n";
            message += "• Corrupted or malformed\n";
            message += "• Not yet valid (future date)\n";
            message += "• In an unsupported format\n\n";
            message += "This cartridge cannot be loaded for security reasons.\n\n";
            if (!errorDetails.isEmpty()) {
                message += QString("Details: %1").arg(errorDetails);
            }
            break;
            
        case SecurityErrorType::FileCorruption:
            message = "The cartridge file appears to be corrupted or in an invalid format.\n\n";
            if (!cartridgeTitle.isEmpty()) {
                message += QString("Cartridge: %1\n\n").arg(cartridgeTitle);
            }
            message += "The file may be:\n";
            message += "• Damaged or incomplete\n";
            message += "• Not a valid Smartbook cartridge\n";
            message += "• Using an unsupported schema version\n\n";
            message += "This cartridge cannot be loaded.\n\n";
            if (!errorDetails.isEmpty()) {
                message += QString("Details: %1").arg(errorDetails);
            }
            break;
            
        case SecurityErrorType::SignatureInvalid:
            message = "The cartridge's digital signature verification failed.\n\n";
            if (!cartridgeTitle.isEmpty()) {
                message += QString("Cartridge: %1\n\n").arg(cartridgeTitle);
            }
            message += "The digital signature does not match the cartridge content. "
                      "This may indicate:\n";
            message += "• The cartridge was modified after signing\n";
            message += "• The signature was created with a different key\n";
            message += "• The signature data is corrupted\n\n";
            message += "For security reasons, this cartridge cannot be loaded.\n\n";
            if (!errorDetails.isEmpty()) {
                message += QString("Details: %1").arg(errorDetails);
            }
            break;
            
        default:
            message = "A security error has occurred.\n\n";
            if (!cartridgeTitle.isEmpty()) {
                message += QString("Cartridge: %1\n\n").arg(cartridgeTitle);
            }
            if (!errorDetails.isEmpty()) {
                message += QString("Details: %1").arg(errorDetails);
            }
            break;
    }
    
    return message;
}

} // namespace ui
} // namespace reader
} // namespace smartbook

