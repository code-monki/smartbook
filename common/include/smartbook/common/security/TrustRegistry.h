#ifndef SMARTBOOK_COMMON_SECURITY_TRUSTREGISTRY_H
#define SMARTBOOK_COMMON_SECURITY_TRUSTREGISTRY_H

#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/security/SignatureVerifier.h"
#include <QString>
#include <QObject>

namespace smartbook {
namespace common {
namespace security {

/**
 * @brief Manages trust decisions in Local_Trust_Registry table
 * 
 * Handles persistent trust storage and retrieval for cartridges.
 * Implements FR-2.4.1 (Persistent Trust) and FR-2.4.3 (Trust Revocation).
 */
class TrustRegistry : public QObject {
    Q_OBJECT

public:
    enum class TrustPolicy {
        PERSISTENT,     // Always trust (across sessions)
        SESSION,        // Trust for current session only
        REVOKED         // Trust revoked, execution blocked
    };

    explicit TrustRegistry(QObject* parent = nullptr);
    
    /**
     * @brief Store persistent trust decision for a cartridge
     * @param cartridgeGuid Cartridge GUID
     * @param policy Trust policy (PERSISTENT, SESSION, or REVOKED)
     * @return true if stored successfully, false otherwise
     */
    bool storeTrustDecision(const QString& cartridgeGuid, TrustPolicy policy);
    
    /**
     * @brief Get trust decision for a cartridge
     * @param cartridgeGuid Cartridge GUID
     * @return TrustPolicy, or PERSISTENT if not found (default)
     */
    TrustPolicy getTrustDecision(const QString& cartridgeGuid);
    
    /**
     * @brief Revoke trust for a cartridge
     * @param cartridgeGuid Cartridge GUID
     * @return true if revoked successfully, false otherwise
     */
    bool revokeTrust(const QString& cartridgeGuid);
    
    /**
     * @brief Check if cartridge has persistent trust
     * @param cartridgeGuid Cartridge GUID
     * @return true if PERSISTENT trust exists, false otherwise
     */
    bool hasPersistentTrust(const QString& cartridgeGuid);

private:
    database::LocalDBManager* m_dbManager;
    
    QString trustPolicyToString(TrustPolicy policy);
    TrustPolicy stringToTrustPolicy(const QString& policyString);
};

} // namespace security
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_SECURITY_TRUSTREGISTRY_H
