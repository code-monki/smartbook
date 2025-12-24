#include "smartbook/creator/ui/CertificateManagerDialog.h"
#include "smartbook/creator/CertificateManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSplitter>
#include <QDateTime>
#include <QDebug>

namespace smartbook {
namespace creator {
namespace ui {

CertificateManagerDialog::CertificateManagerDialog(CertificateManager* certManager, QWidget* parent)
    : QDialog(parent)
    , m_certManager(certManager)
    , m_certificateList(nullptr)
    , m_importButton(nullptr)
    , m_generateButton(nullptr)
    , m_deleteButton(nullptr)
    , m_selectButton(nullptr)
    , m_closeButton(nullptr)
    , m_nameLabel(nullptr)
    , m_subjectLabel(nullptr)
    , m_issuerLabel(nullptr)
    , m_validFromLabel(nullptr)
    , m_validToLabel(nullptr)
    , m_typeLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_commonNameEdit(nullptr)
    , m_organizationEdit(nullptr)
    , m_displayNameEdit(nullptr)
    , m_validityDaysSpin(nullptr)
{
    setupUI();
    refreshCertificateList();
}

CertificateManagerDialog::~CertificateManagerDialog()
{
}

void CertificateManagerDialog::setupUI()
{
    setWindowTitle("Certificate Management");
    resize(900, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create splitter for list and details
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side: Certificate list and buttons
    QWidget* leftWidget = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* listLabel = new QLabel("Certificates:", leftWidget);
    leftLayout->addWidget(listLabel);
    
    m_certificateList = new QListWidget(leftWidget);
    m_certificateList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(m_certificateList);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_importButton = new QPushButton("Import...", leftWidget);
    m_generateButton = new QPushButton("Generate...", leftWidget);
    m_deleteButton = new QPushButton("Delete", leftWidget);
    m_deleteButton->setEnabled(false);
    
    buttonLayout->addWidget(m_importButton);
    buttonLayout->addWidget(m_generateButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    
    leftLayout->addLayout(buttonLayout);
    
    splitter->addWidget(leftWidget);
    
    // Right side: Certificate details
    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    
    QLabel* detailsLabel = new QLabel("Certificate Details:", rightWidget);
    QFont boldFont = detailsLabel->font();
    boldFont.setBold(true);
    detailsLabel->setFont(boldFont);
    rightLayout->addWidget(detailsLabel);
    
    QGroupBox* detailsGroup = new QGroupBox(rightWidget);
    QFormLayout* formLayout = new QFormLayout(detailsGroup);
    
    m_nameLabel = new QLabel("-", detailsGroup);
    m_subjectLabel = new QLabel("-", detailsGroup);
    m_issuerLabel = new QLabel("-", detailsGroup);
    m_validFromLabel = new QLabel("-", detailsGroup);
    m_validToLabel = new QLabel("-", detailsGroup);
    m_typeLabel = new QLabel("-", detailsGroup);
    m_statusLabel = new QLabel("-", detailsGroup);
    
    formLayout->addRow("Name:", m_nameLabel);
    formLayout->addRow("Subject:", m_subjectLabel);
    formLayout->addRow("Issuer:", m_issuerLabel);
    formLayout->addRow("Valid From:", m_validFromLabel);
    formLayout->addRow("Valid To:", m_validToLabel);
    formLayout->addRow("Type:", m_typeLabel);
    formLayout->addRow("Status:", m_statusLabel);
    
    rightLayout->addWidget(detailsGroup);
    rightLayout->addStretch();
    
    splitter->addWidget(rightWidget);
    
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_selectButton = buttonBox->button(QDialogButtonBox::Ok);
    m_selectButton->setText("Select");
    m_selectButton->setEnabled(false);
    m_closeButton = buttonBox->button(QDialogButtonBox::Cancel);
    m_closeButton->setText("Close");
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(m_certificateList, &QListWidget::itemSelectionChanged, this, &CertificateManagerDialog::onCertificateSelectionChanged);
    connect(m_certificateList, &QListWidget::itemDoubleClicked, this, &CertificateManagerDialog::onCertificateDoubleClicked);
    connect(m_importButton, &QPushButton::clicked, this, &CertificateManagerDialog::onImportCertificate);
    connect(m_generateButton, &QPushButton::clicked, this, &CertificateManagerDialog::onGenerateCertificate);
    connect(m_deleteButton, &QPushButton::clicked, this, &CertificateManagerDialog::onDeleteCertificate);
    connect(m_selectButton, &QPushButton::clicked, this, [this]() {
        if (!m_selectedCertificateId.isEmpty()) {
            emit certificateSelected(m_selectedCertificateId);
            accept();
        }
    });
}

void CertificateManagerDialog::refreshCertificateList()
{
    m_certificateList->clear();
    clearCertificateDetails();
    
    if (!m_certManager) {
        return;
    }
    
    QStringList certIds = m_certManager->getCertificateIds();
    for (const QString& certId : certIds) {
        CertificateInfo info = m_certManager->getCertificateInfo(certId);
        if (info.isValid()) {
            QListWidgetItem* item = new QListWidgetItem(info.name, m_certificateList);
            item->setData(Qt::UserRole, certId);
            m_certificateList->addItem(item);
        }
    }
}

void CertificateManagerDialog::onCertificateSelectionChanged()
{
    QList<QListWidgetItem*> selected = m_certificateList->selectedItems();
    if (selected.isEmpty()) {
        clearCertificateDetails();
        m_selectedCertificateId.clear();
        m_deleteButton->setEnabled(false);
        m_selectButton->setEnabled(false);
        return;
    }
    
    QListWidgetItem* item = selected.first();
    QString certId = item->data(Qt::UserRole).toString();
    m_selectedCertificateId = certId;
    
    updateCertificateDetails();
    m_deleteButton->setEnabled(true);
    m_selectButton->setEnabled(true);
}

void CertificateManagerDialog::onCertificateDoubleClicked()
{
    if (!m_selectedCertificateId.isEmpty()) {
        emit certificateSelected(m_selectedCertificateId);
        accept();
    }
}

void CertificateManagerDialog::updateCertificateDetails()
{
    if (!m_certManager || m_selectedCertificateId.isEmpty()) {
        return;
    }
    
    CertificateInfo info = m_certManager->getCertificateInfo(m_selectedCertificateId);
    if (!info.isValid()) {
        clearCertificateDetails();
        return;
    }
    
    m_nameLabel->setText(info.name);
    m_subjectLabel->setText(info.subject.isEmpty() ? "-" : info.subject);
    m_issuerLabel->setText(info.issuer.isEmpty() ? "-" : info.issuer);
    m_validFromLabel->setText(info.validFrom.toString(Qt::ISODate));
    m_validToLabel->setText(info.validTo.toString(Qt::ISODate));
    m_typeLabel->setText(info.isCaSigned ? "CA-Signed" : "Self-Signed");
    
    // Status
    bool expired = CertificateManager::isCertificateExpired(info.certificate);
    bool valid = CertificateManager::validateCertificate(info.certificate);
    
    if (expired) {
        m_statusLabel->setText("Expired");
        m_statusLabel->setStyleSheet("color: red;");
    } else if (!valid) {
        m_statusLabel->setText("Invalid");
        m_statusLabel->setStyleSheet("color: orange;");
    } else {
        m_statusLabel->setText("Valid");
        m_statusLabel->setStyleSheet("color: green;");
    }
}

void CertificateManagerDialog::clearCertificateDetails()
{
    m_nameLabel->setText("-");
    m_subjectLabel->setText("-");
    m_issuerLabel->setText("-");
    m_validFromLabel->setText("-");
    m_validToLabel->setText("-");
    m_typeLabel->setText("-");
    m_statusLabel->setText("-");
    m_statusLabel->setStyleSheet("");
}

void CertificateManagerDialog::onImportCertificate()
{
    if (!m_certManager) {
        return;
    }
    
    // Open certificate file
    QString certPath = QFileDialog::getOpenFileName(this, "Select Certificate File", 
                                                     QString(), "Certificate Files (*.crt *.pem *.der);;All Files (*)");
    if (certPath.isEmpty()) {
        return;
    }
    
    // Open private key file
    QString keyPath = QFileDialog::getOpenFileName(this, "Select Private Key File", 
                                                    QString(), "Key Files (*.key *.pem *.der);;All Files (*)");
    if (keyPath.isEmpty()) {
        return;
    }
    
    // Get display name
    bool ok;
    QString name = QInputDialog::getText(this, "Certificate Name", "Enter a name for this certificate:",
                                         QLineEdit::Normal, QFileInfo(certPath).baseName(), &ok);
    if (!ok || name.isEmpty()) {
        return;
    }
    
    // Import certificate
    QString certId = m_certManager->importCertificate(certPath, keyPath, name);
    if (certId.isEmpty()) {
        QMessageBox::warning(this, "Import Failed", "Failed to import certificate. Please check that the files are valid.");
        return;
    }
    
    refreshCertificateList();
    
    // Select the newly imported certificate
    for (int i = 0; i < m_certificateList->count(); ++i) {
        QListWidgetItem* item = m_certificateList->item(i);
        if (item->data(Qt::UserRole).toString() == certId) {
            m_certificateList->setCurrentItem(item);
            break;
        }
    }
}

void CertificateManagerDialog::onGenerateCertificate()
{
    if (!m_certManager) {
        return;
    }
    
    // Create generate certificate dialog
    QDialog* genDialog = new QDialog(this);
    genDialog->setWindowTitle("Generate Self-Signed Certificate");
    genDialog->resize(400, 200);
    
    QVBoxLayout* layout = new QVBoxLayout(genDialog);
    
    QFormLayout* formLayout = new QFormLayout();
    
    m_commonNameEdit = new QLineEdit(genDialog);
    m_organizationEdit = new QLineEdit(genDialog);
    m_displayNameEdit = new QLineEdit(genDialog);
    m_validityDaysSpin = new QSpinBox(genDialog);
    m_validityDaysSpin->setMinimum(1);
    m_validityDaysSpin->setMaximum(3650); // 10 years
    m_validityDaysSpin->setValue(365); // 1 year default
    
    formLayout->addRow("Common Name (CN):", m_commonNameEdit);
    formLayout->addRow("Organization (O):", m_organizationEdit);
    formLayout->addRow("Display Name:", m_displayNameEdit);
    formLayout->addRow("Validity (days):", m_validityDaysSpin);
    
    layout->addLayout(formLayout);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, genDialog);
    connect(buttonBox, &QDialogButtonBox::accepted, genDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, genDialog, &QDialog::reject);
    
    layout->addWidget(buttonBox);
    
    if (genDialog->exec() == QDialog::Accepted) {
        QString commonName = m_commonNameEdit->text();
        QString organization = m_organizationEdit->text();
        QString displayName = m_displayNameEdit->text();
        int validityDays = m_validityDaysSpin->value();
        
        if (commonName.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Common Name is required.");
            genDialog->deleteLater();
            return;
        }
        
        // Generate certificate
        QString certId = m_certManager->generateSelfSignedCertificate(commonName, organization, validityDays, displayName);
        if (certId.isEmpty()) {
            QMessageBox::warning(this, "Generation Failed", "Failed to generate certificate.");
            genDialog->deleteLater();
            return;
        }
        
        refreshCertificateList();
        
        // Select the newly generated certificate
        for (int i = 0; i < m_certificateList->count(); ++i) {
            QListWidgetItem* item = m_certificateList->item(i);
            if (item->data(Qt::UserRole).toString() == certId) {
                m_certificateList->setCurrentItem(item);
                break;
            }
        }
    }
    
    genDialog->deleteLater();
}

void CertificateManagerDialog::onDeleteCertificate()
{
    if (!m_certManager || m_selectedCertificateId.isEmpty()) {
        return;
    }
    
    CertificateInfo info = m_certManager->getCertificateInfo(m_selectedCertificateId);
    if (!info.isValid()) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Delete Certificate", 
                                    QString("Are you sure you want to delete the certificate '%1'?\n\nThis action cannot be undone.").arg(info.name),
                                    QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (m_certManager->deleteCertificate(m_selectedCertificateId)) {
            refreshCertificateList();
            clearCertificateDetails();
            m_selectedCertificateId.clear();
        } else {
            QMessageBox::warning(this, "Delete Failed", "Failed to delete certificate.");
        }
    }
}

QString CertificateManagerDialog::getSelectedCertificateId() const
{
    return m_selectedCertificateId;
}

} // namespace ui
} // namespace creator
} // namespace smartbook
