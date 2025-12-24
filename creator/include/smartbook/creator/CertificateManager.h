#ifndef SMARTBOOK_CREATOR_CERTIFICATEMANAGER_H
#define SMARTBOOK_CREATOR_CERTIFICATEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QSslCertificate>
#include <QSslKey>

namespace smartbook {
namespace creator {

/**
 * @brief Certificate information structure
 */
struct CertificateInfo {
    QString id;                    // Unique identifier for the certificate
    QString name;                  // Display name
    QString certificatePath;       // Path to certificate file
    QString privateKeyPath;        // Path to private key file
    QSslCertificate certificate;  // Parsed certificate
    QSslKey privateKey;            // Parsed private key
    bool isCaSigned;               // True if CA-signed, false if self-signed
    QDateTime validFrom;           // Certificate valid from date
    QDateTime validTo;             // Certificate valid to date
    QString subject;                // Certificate subject (CN, O, etc.)
    QString issuer;                // Certificate issuer
    bool isValid() const { return !certificate.isNull() && !privateKey.isNull(); }
};

/**
 * @brief Certificate manager for Creator Tool
 * 
 * Handles certificate import, generation, storage, and management.
 * Implements FR-CT-3.33 (Certificate Management)
 */
class CertificateManager : public QObject {
    Q_OBJECT

public:
    explicit CertificateManager(QObject* parent = nullptr);
    ~CertificateManager();

    /**
     * @brief Get list of all managed certificates
     * @return List of certificate IDs
     */
    QStringList getCertificateIds() const;

    /**
     * @brief Get certificate information by ID
     * @param certificateId Certificate ID
     * @return CertificateInfo, or invalid CertificateInfo if not found
     */
    CertificateInfo getCertificateInfo(const QString& certificateId) const;

    /**
     * @brief Import certificate from files
     * @param certificatePath Path to certificate file (PEM or DER)
     * @param privateKeyPath Path to private key file (PEM or DER)
     * @param name Display name for the certificate
     * @return Certificate ID if successful, empty string on failure
     */
    QString importCertificate(const QString& certificatePath, const QString& privateKeyPath, const QString& name);

    /**
     * @brief Generate self-signed certificate
     * @param commonName Common name (CN) for the certificate
     * @param organization Organization (O) for the certificate
     * @param validityDays Number of days the certificate should be valid
     * @param name Display name for the certificate
     * @return Certificate ID if successful, empty string on failure
     */
    QString generateSelfSignedCertificate(const QString& commonName, const QString& organization, 
                                         int validityDays, const QString& name);

    /**
     * @brief Delete certificate
     * @param certificateId Certificate ID to delete
     * @return true if deleted successfully, false otherwise
     */
    bool deleteCertificate(const QString& certificateId);

    /**
     * @brief Validate certificate
     * @param certificate Certificate to validate
     * @return true if certificate is valid, false otherwise
     */
    static bool validateCertificate(const QSslCertificate& certificate);

    /**
     * @brief Check if certificate is expired
     * @param certificate Certificate to check
     * @return true if expired, false otherwise
     */
    static bool isCertificateExpired(const QSslCertificate& certificate);

    /**
     * @brief Check if certificate is CA-signed
     * @param certificate Certificate to check
     * @return true if CA-signed, false if self-signed
     */
    static bool isCaSigned(const QSslCertificate& certificate);

    /**
     * @brief Get certificate storage directory
     * @return Path to certificate storage directory
     */
    static QString getCertificateStorageDirectory();

signals:
    void certificateAdded(const QString& certificateId);
    void certificateRemoved(const QString& certificateId);
    void certificateUpdated(const QString& certificateId);

private:
    void loadCertificates();
    void saveCertificateList();
    QString generateCertificateId() const;
    QString getCertificateFilePath(const QString& certificateId) const;
    QString getPrivateKeyFilePath(const QString& certificateId) const;
    QString getMetadataFilePath(const QString& certificateId) const;
    bool saveCertificateFiles(const QString& certificateId, const QSslCertificate& cert, const QSslKey& key);
    CertificateInfo loadCertificateInfo(const QString& certificateId) const;

    QMap<QString, CertificateInfo> m_certificates;
    QString m_storageDirectory;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_CERTIFICATEMANAGER_H
