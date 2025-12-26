#ifndef SMARTBOOK_COMMON_SECURITY_SIGNATUREVERIFIER_H
#define SMARTBOOK_COMMON_SECURITY_SIGNATUREVERIFIER_H

#include <QString>
#include <QByteArray>
#include <QObject>
#include <QSslCertificate>
#include <QSslKey>

namespace smartbook {
namespace common {
namespace security {

/**
 * @brief Trust policy enumeration
 */
enum class TrustPolicy {
    WHITELISTED,        // Level 1 or persistent trust, no consent required
    CONSENT_REQUIRED,   // Level 2 or Level 3, requires user consent
    REJECTED            // Invalid or tampered cartridge
};

/**
 * @brief Security level enumeration
 */
enum class SecurityLevel {
    LEVEL_1,    // CA-signed certificate
    LEVEL_2,    // Self-signed certificate
    LEVEL_3     // No signature
};

/**
 * @brief Result of signature verification
 */
struct VerificationResult {
    TrustPolicy effectivePolicy;
    SecurityLevel securityLevel;
    bool isTampered;
    QString errorMessage;
    QByteArray h1Hash;
    QByteArray h2Hash;
};

/**
 * @brief Executes the 4-Phase Verification Algorithm
 * 
 * Determines the final Effective Trust Policy before content execution.
 * Centralizes all security logic for cartridge verification.
 */
class SignatureVerifier : public QObject {
    Q_OBJECT

public:
    explicit SignatureVerifier(QObject* parent = nullptr);

    /**
     * @brief Verify a cartridge and determine trust policy
     * @param cartridgePath Path to the cartridge file
     * @param cartridgeGuid Cartridge GUID (if known)
     * @return VerificationResult with trust policy and verification status
     */
    VerificationResult verifyCartridge(const QString& cartridgePath, const QString& cartridgeGuid = QString());

    /**
     * @brief Calculate content hash (H2) for a cartridge
     * @param cartridgePath Path to the cartridge file
     * @return H2 hash as QByteArray, or empty if calculation failed
     */
    QByteArray calculateContentHash(const QString& cartridgePath);

private:
    /**
     * @brief Phase 1: Identity - Read cartridge GUID and security data
     */
    bool phase1_Identity(const QString& cartridgePath, QString& cartridgeGuid, QByteArray& h1Hash, SecurityLevel& level);

    /**
     * @brief Phase 2: Integrity - Calculate H2 and compare with H1
     */
    bool phase2_Integrity(const QString& cartridgePath, const QByteArray& h1Hash, QByteArray& h2Hash, bool& isTampered);

    /**
     * @brief Phase 3: Local Trust - Check persistent trust registry
     */
    TrustPolicy phase3_LocalTrust(const QString& cartridgeGuid, bool isTampered);

    /**
     * @brief Phase 4: Final Policy - Determine effective trust policy
     */
    TrustPolicy phase4_FinalPolicy(SecurityLevel level, TrustPolicy localTrust, bool isTampered);

    /**
     * @brief Validate certificate and determine security level
     * @param certData Certificate data from Cartridge_Security table
     * @return Security level (LEVEL_1, LEVEL_2, or LEVEL_3)
     */
    SecurityLevel validateCertificate(const QByteArray& certData);

    /**
     * @brief Verify digital signature using public key
     * @param signature Digital signature from Cartridge_Security
     * @param hash Hash to verify (H1)
     * @param publicKey Public key extracted from certificate
     * @return true if signature is valid, false otherwise
     */
    bool verifyDigitalSignature(const QByteArray& signature, const QByteArray& hash, const QSslKey& publicKey);

    /**
     * @brief Verify public key fingerprint
     * @param calculatedFingerprint Fingerprint calculated from certificate
     * @param storedFingerprint Fingerprint stored in Cartridge_Security
     * @return true if fingerprints match, false otherwise
     */
    bool verifyFingerprint(const QString& calculatedFingerprint, const QString& storedFingerprint);

    /**
     * @brief Calculate public key fingerprint
     * @param publicKey Public key from certificate
     * @return SHA-256 fingerprint as hexadecimal string
     */
    QString calculatePublicKeyFingerprint(const QSslKey& publicKey);

    /**
     * @brief Extract certificate from certificate data
     * @param certData Certificate data (DER or PEM format)
     * @return QSslCertificate object, or null if invalid
     */
    QSslCertificate extractCertificate(const QByteArray& certData);
};

} // namespace security
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_SECURITY_SIGNATUREVERIFIER_H
