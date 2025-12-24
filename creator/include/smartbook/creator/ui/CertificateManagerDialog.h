#ifndef SMARTBOOK_CREATOR_UI_CERTIFICATEMANAGERDIALOG_H
#define SMARTBOOK_CREATOR_UI_CERTIFICATEMANAGERDIALOG_H

#include <QDialog>
#include <QString>

class QListWidget;
class QPushButton;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace smartbook {
namespace creator {

class CertificateManager;

/**
 * @brief Certificate management dialog
 * 
 * Provides UI for managing certificates used for cartridge signing.
 * Implements FR-CT-3.33 (Certificate Management UI)
 */
class CertificateManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CertificateManagerDialog(CertificateManager* certManager, QWidget* parent = nullptr);
    ~CertificateManagerDialog();

    /**
     * @brief Get selected certificate ID
     * @return Certificate ID if one is selected, empty string otherwise
     */
    QString getSelectedCertificateId() const;

signals:
    void certificateSelected(const QString& certificateId);

private slots:
    void onImportCertificate();
    void onGenerateCertificate();
    void onDeleteCertificate();
    void onCertificateSelectionChanged();
    void onCertificateDoubleClicked();
    void refreshCertificateList();

private:
    void setupUI();
    void updateCertificateDetails();
    void clearCertificateDetails();

    CertificateManager* m_certManager;
    QListWidget* m_certificateList;
    QPushButton* m_importButton;
    QPushButton* m_generateButton;
    QPushButton* m_deleteButton;
    QPushButton* m_selectButton;
    QPushButton* m_closeButton;
    
    // Certificate details
    QLabel* m_nameLabel;
    QLabel* m_subjectLabel;
    QLabel* m_issuerLabel;
    QLabel* m_validFromLabel;
    QLabel* m_validToLabel;
    QLabel* m_typeLabel;
    QLabel* m_statusLabel;
    
    // Generate certificate dialog fields
    QLineEdit* m_commonNameEdit;
    QLineEdit* m_organizationEdit;
    QLineEdit* m_displayNameEdit;
    QSpinBox* m_validityDaysSpin;
    
    QString m_selectedCertificateId;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_UI_CERTIFICATEMANAGERDIALOG_H
