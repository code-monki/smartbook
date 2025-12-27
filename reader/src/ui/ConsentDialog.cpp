#include "smartbook/reader/ui/ConsentDialog.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QCloseEvent>

namespace smartbook {
namespace reader {
namespace ui {

ConsentDialog::ConsentDialog(
    smartbook::common::security::SecurityLevel level,
    const QString& cartridgeTitle,
    const QString& authorName,
    QWidget* parent)
    : QDialog(parent)
{
    setupUI(level, cartridgeTitle, authorName);
}

ConsentDialog::~ConsentDialog() {
}

void ConsentDialog::setupUI(
    smartbook::common::security::SecurityLevel level,
    const QString& cartridgeTitle,
    const QString& authorName)
{
    using SecurityLevel = smartbook::common::security::SecurityLevel;
    
    // Set window title based on security level
    if (level == SecurityLevel::LEVEL_2) {
        setWindowTitle("Security Warning: Self-Signed Cartridge");
    } else if (level == SecurityLevel::LEVEL_3) {
        setWindowTitle("Security Warning: Unsigned Cartridge");
    } else {
        setWindowTitle("Security Warning");
    }
    
    setMinimumWidth(500);
    resize(500, 450);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);

    QLabel* titleLabel = new QLabel(
        level == SecurityLevel::LEVEL_2 ? "Security Warning: Self-Signed Cartridge" :
        level == SecurityLevel::LEVEL_3 ? "Security Warning: Unsigned Cartridge" :
        "Security Warning",
        this
    );
    titleLabel->setStyleSheet("font-weight: bold; font-size: 16pt; color: #f57c00;");
    layout->addWidget(titleLabel);

    QTextEdit* warningText = new QTextEdit(this);
    warningText->setReadOnly(true);
    // Use dark text color for readability on light orange background (works with all themes including sepia)
    warningText->setStyleSheet("background-color: #fff3e0; border: 1px solid #f57c00; padding: 10px; color: #1a1a1a;");

    QString message;
    
    if (level == SecurityLevel::LEVEL_2) {
        message = "This cartridge is signed with a self-signed certificate and cannot be verified by a trusted authority.\n\n";
        
        if (!cartridgeTitle.isEmpty()) {
            message += QString("Cartridge: %1\n").arg(cartridgeTitle);
        }
        if (!authorName.isEmpty()) {
            message += QString("Author: %1\n").arg(authorName);
        }
        message += "Security Level: Level 2 (Self-Signed Trust)\n\n";
        
        message += "⚠️ WARNING: This cartridge has not been verified by a trusted certificate authority. "
                  "The content may have been modified or could contain potentially unsafe embedded applications.\n\n";
        
        message += "Do you want to load this cartridge?";
    } else if (level == SecurityLevel::LEVEL_3) {
        message = "This cartridge has no digital signature and cannot be verified.\n\n";
        
        if (!cartridgeTitle.isEmpty()) {
            message += QString("Cartridge: %1\n").arg(cartridgeTitle);
        }
        if (!authorName.isEmpty()) {
            message += QString("Author: %1\n").arg(authorName);
        }
        message += "Security Level: Level 3 (No Signature)\n\n";
        
        message += "⚠️ WARNING: This cartridge has no digital signature. The content cannot be verified "
                  "and may have been modified. Embedded applications may pose security risks.\n\n";
        
        message += "Do you want to load this cartridge?";
    }
    
    warningText->setPlainText(message);

    layout->addWidget(warningText);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    
    QPushButton* alwaysTrustButton = new QPushButton("Load and Always Trust", this);
    QPushButton* sessionOnlyButton = new QPushButton("Load for This Session Only", this);
    QPushButton* cancelButton = new QPushButton("Cancel", this);

    buttonBox->addButton(alwaysTrustButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(sessionOnlyButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    connect(alwaysTrustButton, &QPushButton::clicked, this, &ConsentDialog::onLoadAndAlwaysTrust);
    connect(sessionOnlyButton, &QPushButton::clicked, this, &ConsentDialog::onLoadForSessionOnly);
    connect(cancelButton, &QPushButton::clicked, this, &ConsentDialog::onCancel);

    layout->addWidget(buttonBox);
}

void ConsentDialog::onLoadAndAlwaysTrust() {
    m_result = LoadAndAlwaysTrust;
    accept();
}

void ConsentDialog::onLoadForSessionOnly() {
    m_result = LoadForSessionOnly;
    accept();
}

void ConsentDialog::onCancel() {
    m_result = Cancel;
    reject();
}

} // namespace ui
} // namespace reader
} // namespace smartbook
