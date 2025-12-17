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

ConsentDialog::ConsentDialog(smartbook::common::security::SecurityLevel level, const QString& cartridgeTitle, QWidget* parent)
    : QDialog(parent)
{
    setupUI(level, cartridgeTitle);
}

ConsentDialog::~ConsentDialog() {
}

void ConsentDialog::setupUI(smartbook::common::security::SecurityLevel level, const QString& cartridgeTitle) {
    setWindowTitle("Security Warning");
    setMinimumWidth(500);
    resize(500, 400);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* titleLabel = new QLabel(QString("Security Warning: %1").arg(cartridgeTitle), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14pt;");
    layout->addWidget(titleLabel);

    QTextEdit* warningText = new QTextEdit(this);
    warningText->setReadOnly(true);

    using SecurityLevel = smartbook::common::security::SecurityLevel;
    
    if (level == SecurityLevel::LEVEL_2) {
        warningText->setPlainText(
            "This cartridge is signed with a self-signed certificate.\n\n"
            "Self-signed certificates are not verified by a trusted Certificate Authority. "
            "The publisher's identity cannot be verified, and embedded applications may require your consent to run.\n\n"
            "Do you want to load this cartridge?"
        );
    } else if (level == SecurityLevel::LEVEL_3) {
        warningText->setPlainText(
            "This cartridge is not digitally signed.\n\n"
            "Unsigned cartridges cannot verify the publisher's identity or content integrity. "
            "Embedded applications will require your explicit consent to run, and you will see "
            "persistent warnings when using this cartridge.\n\n"
            "Do you want to load this cartridge?"
        );
    }

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
