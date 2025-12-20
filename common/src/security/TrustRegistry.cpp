#include "smartbook/common/security/TrustRegistry.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

namespace smartbook {
namespace common {
namespace security {

TrustRegistry::TrustRegistry(QObject* parent)
    : QObject(parent)
    , m_dbManager(&database::LocalDBManager::getInstance())
{
}

bool TrustRegistry::storeTrustDecision(const QString& cartridgeGuid, TrustPolicy policy)
{
    if (!m_dbManager->isOpen()) {
        qWarning() << "Database not open for trust decision storage";
        return false;
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    
    // Use INSERT OR REPLACE to handle updates
    query.prepare(R"(
        INSERT OR REPLACE INTO Local_Trust_Registry 
        (cartridge_guid, trust_policy, granted_timestamp, last_verified_timestamp)
        VALUES (?, ?, ?, ?)
    )");
    
    query.addBindValue(cartridgeGuid);
    query.addBindValue(trustPolicyToString(policy));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    
    if (!query.exec()) {
        qCritical() << "Failed to store trust decision:" << query.lastError().text();
        return false;
    }
    
    return true;
}

TrustRegistry::TrustPolicy TrustRegistry::getTrustDecision(const QString& cartridgeGuid)
{
    if (!m_dbManager->isOpen()) {
        return TrustPolicy::PERSISTENT; // Default
    }
    
    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("SELECT trust_policy FROM Local_Trust_Registry WHERE cartridge_guid = ?");
    query.addBindValue(cartridgeGuid);
    
    if (!query.exec()) {
        qWarning() << "Failed to query trust decision:" << query.lastError().text();
        return TrustPolicy::PERSISTENT; // Default
    }
    
    if (query.next()) {
        QString policyString = query.value(0).toString();
        return stringToTrustPolicy(policyString);
    }
    
    return TrustPolicy::PERSISTENT; // Default if not found
}

bool TrustRegistry::revokeTrust(const QString& cartridgeGuid)
{
    return storeTrustDecision(cartridgeGuid, TrustPolicy::REVOKED);
}

bool TrustRegistry::hasPersistentTrust(const QString& cartridgeGuid)
{
    return getTrustDecision(cartridgeGuid) == TrustPolicy::PERSISTENT;
}

QString TrustRegistry::trustPolicyToString(TrustPolicy policy)
{
    switch (policy) {
        case TrustPolicy::PERSISTENT:
            return "PERSISTENT";
        case TrustPolicy::SESSION:
            return "SESSION";
        case TrustPolicy::REVOKED:
            return "REVOKED";
        default:
            return "PERSISTENT";
    }
}

TrustRegistry::TrustPolicy TrustRegistry::stringToTrustPolicy(const QString& policyString)
{
    if (policyString == "PERSISTENT") {
        return TrustPolicy::PERSISTENT;
    } else if (policyString == "SESSION") {
        return TrustPolicy::SESSION;
    } else if (policyString == "REVOKED") {
        return TrustPolicy::REVOKED;
    }
    return TrustPolicy::PERSISTENT; // Default
}

} // namespace security
} // namespace common
} // namespace smartbook
