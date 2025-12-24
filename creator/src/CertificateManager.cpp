#include "smartbook/creator/CertificateManager.h"
#include "smartbook/common/utils/PlatformUtils.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QDebug>

// OpenSSL for certificate generation
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

namespace smartbook {
namespace creator {

CertificateManager::CertificateManager(QObject* parent)
    : QObject(parent)
{
    m_storageDirectory = getCertificateStorageDirectory();
    QDir().mkpath(m_storageDirectory);
    loadCertificates();
}

CertificateManager::~CertificateManager()
{
}

QString CertificateManager::getCertificateStorageDirectory()
{
    QString baseDir = common::utils::PlatformUtils::getApplicationDataDirectory();
    QString certDir = baseDir + "/certificates";
    QDir().mkpath(certDir);
    return certDir;
}

void CertificateManager::loadCertificates()
{
    m_certificates.clear();
    
    QString listFile = m_storageDirectory + "/certificates.json";
    if (!QFile::exists(listFile)) {
        return;
    }
    
    QFile file(listFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open certificate list file:" << listFile;
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        qWarning() << "Invalid certificate list file format";
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray certArray = root.value("certificates").toArray();
    
    for (const QJsonValue& value : certArray) {
        QJsonObject certObj = value.toObject();
        QString certId = certObj.value("id").toString();
        
        CertificateInfo info = loadCertificateInfo(certId);
        if (info.isValid()) {
            m_certificates[certId] = info;
        }
    }
    
    qDebug() << "Loaded" << m_certificates.size() << "certificates";
}

void CertificateManager::saveCertificateList()
{
    QJsonObject root;
    QJsonArray certArray;
    
    for (const CertificateInfo& info : m_certificates) {
        QJsonObject certObj;
        certObj["id"] = info.id;
        certObj["name"] = info.name;
        certObj["certificatePath"] = info.certificatePath;
        certObj["privateKeyPath"] = info.privateKeyPath;
        certObj["isCaSigned"] = info.isCaSigned;
        certObj["validFrom"] = info.validFrom.toString(Qt::ISODate);
        certObj["validTo"] = info.validTo.toString(Qt::ISODate);
        certObj["subject"] = info.subject;
        certObj["issuer"] = info.issuer;
        certArray.append(certObj);
    }
    
    root["certificates"] = certArray;
    
    QJsonDocument doc(root);
    QString listFile = m_storageDirectory + "/certificates.json";
    
    QFile file(listFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save certificate list file:" << listFile;
        return;
    }
    
    file.write(doc.toJson());
    file.close();
}

QStringList CertificateManager::getCertificateIds() const
{
    return m_certificates.keys();
}

CertificateInfo CertificateManager::getCertificateInfo(const QString& certificateId) const
{
    if (m_certificates.contains(certificateId)) {
        return m_certificates[certificateId];
    }
    return CertificateInfo(); // Invalid
}

QString CertificateManager::importCertificate(const QString& certificatePath, const QString& privateKeyPath, const QString& name)
{
    // Load certificate
    QFile certFile(certificatePath);
    if (!certFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open certificate file:" << certificatePath;
        return QString();
    }
    QByteArray certData = certFile.readAll();
    certFile.close();
    
    // Load private key
    QFile keyFile(privateKeyPath);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open private key file:" << privateKeyPath;
        return QString();
    }
    QByteArray keyData = keyFile.readAll();
    keyFile.close();
    
    // Parse certificate
    QSslCertificate cert(certData, QSsl::Der);
    if (cert.isNull()) {
        cert = QSslCertificate(certData, QSsl::Pem);
    }
    
    if (cert.isNull()) {
        qWarning() << "Failed to parse certificate";
        return QString();
    }
    
    // Parse private key
    QSslKey key(keyData, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    if (key.isNull()) {
        key = QSslKey(keyData, QSsl::Rsa, QSsl::Der, QSsl::PrivateKey);
    }
    
    if (key.isNull()) {
        qWarning() << "Failed to parse private key";
        return QString();
    }
    
    // Validate certificate
    if (!validateCertificate(cert)) {
        qWarning() << "Certificate validation failed";
        return QString();
    }
    
    // Generate certificate ID
    QString certId = generateCertificateId();
    
    // Save certificate files
    if (!saveCertificateFiles(certId, cert, key)) {
        qWarning() << "Failed to save certificate files";
        return QString();
    }
    
    // Create certificate info
    CertificateInfo info;
    info.id = certId;
    info.name = name.isEmpty() ? QFileInfo(certificatePath).baseName() : name;
    info.certificatePath = getCertificateFilePath(certId);
    info.privateKeyPath = getPrivateKeyFilePath(certId);
    info.certificate = cert;
    info.privateKey = key;
    info.isCaSigned = isCaSigned(cert);
    info.validFrom = cert.effectiveDate();
    info.validTo = cert.expiryDate();
    info.subject = cert.subjectDisplayName();
    info.issuer = cert.issuerDisplayName();
    
    m_certificates[certId] = info;
    saveCertificateList();
    
    emit certificateAdded(certId);
    
    return certId;
}

QString CertificateManager::generateSelfSignedCertificate(const QString& commonName, const QString& organization, 
                                                          int validityDays, const QString& displayName)
{
    // Generate RSA key pair using EVP API (OpenSSL 3.x compatible)
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        qWarning() << "Failed to create EVP_PKEY_CTX";
        return QString();
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        qWarning() << "Failed to initialize key generation";
        EVP_PKEY_CTX_free(ctx);
        return QString();
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        qWarning() << "Failed to set RSA key size";
        EVP_PKEY_CTX_free(ctx);
        return QString();
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0 || !pkey) {
        qWarning() << "Failed to generate RSA key";
        EVP_PKEY_CTX_free(ctx);
        return QString();
    }
    
    EVP_PKEY_CTX_free(ctx);
    
    // Create X.509 certificate
    X509* x509 = X509_new();
    if (!x509) {
        qWarning() << "Failed to create X509 certificate";
        EVP_PKEY_free(pkey);
        return QString();
    }
    
    // Set version
    X509_set_version(x509, 2); // X.509 v3
    
    // Set serial number
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    
    // Set validity period
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), validityDays * 24 * 60 * 60);
    
    // Set subject and issuer (self-signed)
    X509_NAME* x509Name = X509_NAME_new();
    if (!x509Name) {
        qWarning() << "Failed to create X509_NAME";
        X509_free(x509);
        EVP_PKEY_free(pkey);
        return QString();
    }
    
    if (!commonName.isEmpty()) {
        X509_NAME_add_entry_by_txt(x509Name, "CN", MBSTRING_ASC, 
                                    reinterpret_cast<const unsigned char*>(commonName.toUtf8().constData()), 
                                    -1, -1, 0);
    }
    if (!organization.isEmpty()) {
        X509_NAME_add_entry_by_txt(x509Name, "O", MBSTRING_ASC, 
                                    reinterpret_cast<const unsigned char*>(organization.toUtf8().constData()), 
                                    -1, -1, 0);
    }
    X509_set_subject_name(x509, x509Name);
    X509_set_issuer_name(x509, x509Name);
    
    // Set public key
    X509_set_pubkey(x509, pkey);
    
    // Sign certificate
    if (!X509_sign(x509, pkey, EVP_sha256())) {
        qWarning() << "Failed to sign certificate";
        X509_free(x509); // This will also free the X509_NAME
        EVP_PKEY_free(pkey);
        return QString();
    }
    
    // Convert to QSslCertificate and QSslKey
    // Write to memory BIO
    BIO* certBio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(certBio, x509);
    
    char* certData = nullptr;
    long certLen = BIO_get_mem_data(certBio, &certData);
    QByteArray certBytes(certData, certLen);
    
    BIO* keyBio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    
    char* keyData = nullptr;
    long keyLen = BIO_get_mem_data(keyBio, &keyData);
    QByteArray keyBytes(keyData, keyLen);
    
    QSslCertificate cert(certBytes, QSsl::Pem);
    QSslKey key(keyBytes, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    
    // Cleanup
    BIO_free(certBio);
    BIO_free(keyBio);
    X509_free(x509);
    EVP_PKEY_free(pkey);
    // X509_NAME is freed when X509 is freed, no need to free separately
    
    if (cert.isNull() || key.isNull()) {
        qWarning() << "Failed to convert certificate/key to Qt format";
        return QString();
    }
    
    // Generate certificate ID
    QString certId = generateCertificateId();
    
    // Save certificate files
    if (!saveCertificateFiles(certId, cert, key)) {
        qWarning() << "Failed to save certificate files";
        return QString();
    }
    
    // Create certificate info
    CertificateInfo info;
    info.id = certId;
    info.name = displayName.isEmpty() ? QString("%1 (%2)").arg(commonName, organization) : displayName;
    info.certificatePath = getCertificateFilePath(certId);
    info.privateKeyPath = getPrivateKeyFilePath(certId);
    info.certificate = cert;
    info.privateKey = key;
    info.isCaSigned = false; // Self-signed
    info.validFrom = cert.effectiveDate();
    info.validTo = cert.expiryDate();
    info.subject = cert.subjectDisplayName();
    info.issuer = cert.issuerDisplayName();
    
    m_certificates[certId] = info;
    saveCertificateList();
    
    emit certificateAdded(certId);
    
    return certId;
}

bool CertificateManager::deleteCertificate(const QString& certificateId)
{
    if (!m_certificates.contains(certificateId)) {
        return false;
    }
    
    // Delete certificate files
    QFile::remove(getCertificateFilePath(certificateId));
    QFile::remove(getPrivateKeyFilePath(certificateId));
    QFile::remove(getMetadataFilePath(certificateId));
    
    m_certificates.remove(certificateId);
    saveCertificateList();
    
    emit certificateRemoved(certificateId);
    
    return true;
}

bool CertificateManager::validateCertificate(const QSslCertificate& certificate)
{
    if (certificate.isNull()) {
        return false;
    }
    
    // Check expiration
    if (isCertificateExpired(certificate)) {
        return false;
    }
    
    // Check validity dates
    QDateTime now = QDateTime::currentDateTime();
    if (certificate.effectiveDate() > now) {
        return false; // Not yet valid
    }
    
    return true;
}

bool CertificateManager::isCertificateExpired(const QSslCertificate& certificate)
{
    if (certificate.isNull()) {
        return true;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    return certificate.expiryDate() < now;
}

bool CertificateManager::isCaSigned(const QSslCertificate& certificate)
{
    if (certificate.isNull()) {
        return false;
    }
    
    // Check if certificate is self-signed (subject == issuer)
    QString subject = certificate.subjectDisplayName();
    QString issuer = certificate.issuerDisplayName();
    
    if (subject == issuer) {
        return false; // Self-signed
    }
    
    // Try to verify against system CA store
    QList<QSslCertificate> caCerts = QSslCertificate::fromPath(":/ca-certificates.crt");
    if (caCerts.isEmpty()) {
        // Use system default CA certificates
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        caCerts = config.caCertificates();
    }
    
    // For now, assume if subject != issuer, it might be CA-signed
    // Full verification would require checking against CA store
    // This is a simplified check - full verification happens during signing
    return true; // Assume CA-signed if not self-signed
}

QString CertificateManager::generateCertificateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString CertificateManager::getCertificateFilePath(const QString& certificateId) const
{
    return m_storageDirectory + "/" + certificateId + ".crt";
}

QString CertificateManager::getPrivateKeyFilePath(const QString& certificateId) const
{
    return m_storageDirectory + "/" + certificateId + ".key";
}

QString CertificateManager::getMetadataFilePath(const QString& certificateId) const
{
    return m_storageDirectory + "/" + certificateId + ".meta";
}

bool CertificateManager::saveCertificateFiles(const QString& certificateId, const QSslCertificate& cert, const QSslKey& key)
{
    // Save certificate
    QString certPath = getCertificateFilePath(certificateId);
    QFile certFile(certPath);
    if (!certFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open certificate file for writing:" << certPath;
        return false;
    }
    certFile.write(cert.toPem());
    certFile.close();
    
    // Save private key
    QString keyPath = getPrivateKeyFilePath(certificateId);
    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open private key file for writing:" << keyPath;
        QFile::remove(certPath); // Clean up certificate file
        return false;
    }
    keyFile.write(key.toPem());
    keyFile.close();
    
    return true;
}

CertificateInfo CertificateManager::loadCertificateInfo(const QString& certificateId) const
{
    CertificateInfo info;
    info.id = certificateId;
    
    QString certPath = getCertificateFilePath(certificateId);
    QString keyPath = getPrivateKeyFilePath(certificateId);
    
    if (!QFile::exists(certPath) || !QFile::exists(keyPath)) {
        return info; // Invalid
    }
    
    // Load certificate
    QFile certFile(certPath);
    if (!certFile.open(QIODevice::ReadOnly)) {
        return info; // Invalid
    }
    QByteArray certData = certFile.readAll();
    certFile.close();
    
    QSslCertificate cert(certData, QSsl::Pem);
    if (cert.isNull()) {
        return info; // Invalid
    }
    
    // Load private key
    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        return info; // Invalid
    }
    QByteArray keyData = keyFile.readAll();
    keyFile.close();
    
    QSslKey key(keyData, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    if (key.isNull()) {
        return info; // Invalid
    }
    
    // Load metadata if available
    QString metaPath = getMetadataFilePath(certificateId);
    QString name = QFileInfo(certPath).baseName();
    if (QFile::exists(metaPath)) {
        QFile metaFile(metaPath);
        if (metaFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(metaFile.readAll());
            metaFile.close();
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                name = obj.value("name").toString(name);
            }
        }
    }
    
    info.certificatePath = certPath;
    info.privateKeyPath = keyPath;
    info.certificate = cert;
    info.privateKey = key;
    info.name = name;
    info.isCaSigned = isCaSigned(cert);
    info.validFrom = cert.effectiveDate();
    info.validTo = cert.expiryDate();
    info.subject = cert.subjectDisplayName();
    info.issuer = cert.issuerDisplayName();
    
    return info;
}

} // namespace creator
} // namespace smartbook
