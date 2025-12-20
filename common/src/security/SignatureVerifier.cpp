#include "smartbook/common/security/SignatureVerifier.h"
#include <QtSql/QSqlRecord>
#include "smartbook/common/database/LocalDBManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDebug>

namespace smartbook {
namespace common {
namespace security {

SignatureVerifier::SignatureVerifier(QObject* parent)
    : QObject(parent)
{
}

VerificationResult SignatureVerifier::verifyCartridge(const QString& cartridgePath, const QString& cartridgeGuid) {
    VerificationResult result;
    result.effectivePolicy = TrustPolicy::REJECTED;
    result.isTampered = false;

    QString guid = cartridgeGuid;
    QByteArray h1Hash;
    SecurityLevel level = SecurityLevel::LEVEL_3;

    // Phase 1: Identity
    if (!phase1_Identity(cartridgePath, guid, h1Hash, level)) {
        result.errorMessage = "Failed to read cartridge identity";
        return result;
    }

    result.h1Hash = h1Hash;
    result.securityLevel = level;

    // Phase 2: Integrity
    QByteArray h2Hash;
    bool isTampered = false;
    if (!phase2_Integrity(cartridgePath, h1Hash, h2Hash, isTampered)) {
        result.errorMessage = "Failed to verify cartridge integrity";
        return result;
    }

    result.h2Hash = h2Hash;
    result.isTampered = isTampered;

    // Phase 3: Local Trust
    TrustPolicy localTrust = phase3_LocalTrust(guid, isTampered);

    // Phase 4: Final Policy
    result.effectivePolicy = phase4_FinalPolicy(level, localTrust, isTampered);

    return result;
}

QByteArray SignatureVerifier::calculateContentHash(const QString& cartridgePath) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "HashCalc");
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        qWarning() << "Failed to open cartridge for hash calculation";
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);

    // Hash tables in fixed order: Content_Pages, Content_Themes, Embedded_Apps, Form_Definitions, Metadata, Settings
    QStringList tables = {"Content_Pages", "Content_Themes", "Embedded_Apps", "Form_Definitions", "Metadata", "Settings"};

    for (const QString& tableName : tables) {
        QSqlQuery query(db);
        QString tableQuery = QString("SELECT * FROM %1 ORDER BY rowid").arg(tableName);
        
        if (!query.exec(tableQuery)) {
            // Table might not exist, hash empty
            hash.addData(QByteArray());
            continue;
        }

        // Hash each row
        while (query.next()) {
            QByteArray rowData;
            for (int i = 0; i < query.record().count(); ++i) {
                QVariant value = query.value(i);
                if (value.isNull()) {
                    rowData.append('\0');
                } else {
                    rowData.append(value.toString().toUtf8());
                }
            }
            hash.addData(rowData);
            hash.addData("\n");
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("HashCalc");

    return hash.result();
}

bool SignatureVerifier::phase1_Identity(const QString& cartridgePath, QString& cartridgeGuid, QByteArray& h1Hash, SecurityLevel& level) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "Phase1");
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        return false;
    }

    QSqlQuery query(db);

    // Read cartridge GUID
    if (query.exec("SELECT cartridge_guid FROM Metadata LIMIT 1")) {
        if (query.next()) {
            cartridgeGuid = query.value(0).toString();
        }
    }

    // Read security data
    if (query.exec("SELECT hash_digest, certificate_data FROM Cartridge_Security LIMIT 1")) {
        if (query.next()) {
            h1Hash = query.value(0).toByteArray();
            QByteArray certData = query.value(1).toByteArray();
            
            if (!certData.isEmpty()) {
                // Check if CA-signed or self-signed
                // For now, use a simple heuristic: if cert data contains "CA_SIGNED" marker,
                // treat as Level 1. In production, this would use QSslCertificate to verify
                // against system CA store.
                if (certData.contains("CA_SIGNED")) {
                    level = SecurityLevel::LEVEL_1;
                } else {
                    level = SecurityLevel::LEVEL_2; // Self-signed
                }
            } else {
                level = SecurityLevel::LEVEL_3; // No certificate
            }
        } else {
            level = SecurityLevel::LEVEL_3;
        }
    } else {
        level = SecurityLevel::LEVEL_3;
    }

    db.close();
    QSqlDatabase::removeDatabase("Phase1");

    return !cartridgeGuid.isEmpty();
}

bool SignatureVerifier::phase2_Integrity(const QString& cartridgePath, const QByteArray& h1Hash, QByteArray& h2Hash, bool& isTampered) {
    h2Hash = calculateContentHash(cartridgePath);
    
    if (h2Hash.isEmpty()) {
        return false;
    }

    isTampered = (h1Hash != h2Hash);
    return true;
}

TrustPolicy SignatureVerifier::phase3_LocalTrust(const QString& cartridgeGuid, bool isTampered) {
    if (isTampered) {
        return TrustPolicy::REJECTED;
    }

    database::LocalDBManager& dbManager = database::LocalDBManager::getInstance();
    if (!dbManager.isOpen()) {
        return TrustPolicy::CONSENT_REQUIRED;
    }

    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT trust_policy FROM Local_Trust_Registry WHERE cartridge_guid = ?");
    query.addBindValue(cartridgeGuid);

    if (query.exec() && query.next()) {
        QString policy = query.value(0).toString();
        if (policy == "PERSISTENT") {
            return TrustPolicy::WHITELISTED;
        } else if (policy == "REVOKED") {
            return TrustPolicy::REJECTED;
        }
    }

    return TrustPolicy::CONSENT_REQUIRED;
}

TrustPolicy SignatureVerifier::phase4_FinalPolicy(SecurityLevel level, TrustPolicy localTrust, bool isTampered) {
    if (isTampered) {
        return TrustPolicy::REJECTED;
    }

    if (localTrust == TrustPolicy::WHITELISTED) {
        return TrustPolicy::WHITELISTED;
    }

    if (level == SecurityLevel::LEVEL_1 && !isTampered) {
        return TrustPolicy::WHITELISTED;
    }

    return TrustPolicy::CONSENT_REQUIRED;
}

} // namespace security
} // namespace common
} // namespace smartbook
