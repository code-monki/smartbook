#include <QtTest>
#include "smartbook/common/security/TrustRegistry.h"
#include "smartbook/common/database/LocalDBManager.h"
#include <QTemporaryDir>
#include <QUuid>

using namespace smartbook::common::security;
using namespace smartbook::common::database;

class TestTrustRegistry : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testPersistentTrust();  // T-SEC-03: Persistent Trust (FR-2.4.1)
    void testTrustRevocation();   // T-SEC-05: Trust Revocation (FR-2.4.3)

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
};

void TestTrustRegistry::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
}

void TestTrustRegistry::cleanupTestCase()
{
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

// T-SEC-03: Persistent Trust
// Requirement: FR-2.4.1
// Test Plan: test-plan.adoc lines 46-56
// AC: A PERSISTENT record is written to Local_Trust_Registry.
//     The second load skips the dialog, and the consent call returns TRUE.
void TestTrustRegistry::testPersistentTrust()
{
    TrustRegistry registry(this);
    
    QString cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Store persistent trust
    bool stored = registry.storeTrustDecision(cartridgeGuid, TrustRegistry::TrustPolicy::PERSISTENT);
    QVERIFY(stored);
    
    // Verify trust exists
    bool hasTrust = registry.hasPersistentTrust(cartridgeGuid);
    QVERIFY(hasTrust);
    
    // Verify trust policy
    TrustRegistry::TrustPolicy policy = registry.getTrustDecision(cartridgeGuid);
    QCOMPARE(policy, TrustRegistry::TrustPolicy::PERSISTENT);
    
    // Simulate second load - should still have persistent trust
    TrustRegistry registry2(this); // New instance
    bool hasTrust2 = registry2.hasPersistentTrust(cartridgeGuid);
    QVERIFY(hasTrust2);
}

// T-SEC-05: Trust Revocation
// Requirement: FR-2.4.3
// Test Plan: test-plan.adoc lines 72-82
// AC: The Local_Trust_Registry record is set to app_trust_policy: REVOKED.
//     The next load forces the user to re-consent.
void TestTrustRegistry::testTrustRevocation()
{
    TrustRegistry registry(this);
    
    QString cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // First, store persistent trust
    bool stored = registry.storeTrustDecision(cartridgeGuid, TrustRegistry::TrustPolicy::PERSISTENT);
    QVERIFY(stored);
    QVERIFY(registry.hasPersistentTrust(cartridgeGuid));
    
    // Revoke trust
    bool revoked = registry.revokeTrust(cartridgeGuid);
    QVERIFY(revoked);
    
    // Verify trust is revoked
    TrustRegistry::TrustPolicy policy = registry.getTrustDecision(cartridgeGuid);
    QCOMPARE(policy, TrustRegistry::TrustPolicy::REVOKED);
    
    // Verify persistent trust no longer exists
    bool hasTrust = registry.hasPersistentTrust(cartridgeGuid);
    QVERIFY(!hasTrust);
}

QTEST_MAIN(TestTrustRegistry)
#include "test_trustregistry.moc"
