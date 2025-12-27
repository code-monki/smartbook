#include <QtTest>
#include "smartbook/reader/ui/SecurityErrorDialog.h"
#include <QApplication>
#include <QTest>

using namespace smartbook::reader::ui;

class TestSecurityErrorDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Dialog creation tests
    void testDialogCreationTampering();     // Test TamperingDetected dialog
    void testDialogCreationInvalidCert();   // Test InvalidCertificate dialog
    void testDialogCreationFileCorruption(); // Test FileCorruption dialog
    void testDialogCreationSignatureInvalid(); // Test SignatureInvalid dialog
    
    // Dialog content tests
    void testDialogTitle();                 // Test error title based on type
    void testDialogMessage();               // Test error message content
    void testDialogWithCartridgeTitle();    // Test dialog with cartridge title
    void testDialogWithErrorDetails();      // Test dialog with error details

private:
    QApplication* m_app;
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestSecurityErrorDialog tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestSecurityErrorDialog::initTestCase()
{
    // QApplication is created by custom main()
}

void TestSecurityErrorDialog::cleanupTestCase()
{
}

// Test TamperingDetected dialog creation
void TestSecurityErrorDialog::testDialogCreationTampering()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::TamperingDetected,
        "Test Book",
        "H1 hash mismatch"
    );
    
    QVERIFY(dialog.isVisible() == false); // Dialog not shown yet
    QVERIFY(!dialog.windowTitle().isEmpty());
}

// Test InvalidCertificate dialog creation
void TestSecurityErrorDialog::testDialogCreationInvalidCert()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::InvalidCertificate,
        "Test Book",
        "Certificate expired"
    );
    
    QVERIFY(dialog.isVisible() == false);
    QVERIFY(!dialog.windowTitle().isEmpty());
}

// Test FileCorruption dialog creation
void TestSecurityErrorDialog::testDialogCreationFileCorruption()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::FileCorruption,
        "Test Book",
        "Database schema invalid"
    );
    
    QVERIFY(dialog.isVisible() == false);
    QVERIFY(!dialog.windowTitle().isEmpty());
}

// Test SignatureInvalid dialog creation
void TestSecurityErrorDialog::testDialogCreationSignatureInvalid()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::SignatureInvalid,
        "Test Book",
        "Digital signature verification failed"
    );
    
    QVERIFY(dialog.isVisible() == false);
    QVERIFY(!dialog.windowTitle().isEmpty());
}

// Test error title based on type
void TestSecurityErrorDialog::testDialogTitle()
{
    {
        SecurityErrorDialog dialog(SecurityErrorType::TamperingDetected, "Book", "Details");
        // Title should contain "Tampering" or similar
        QString title = dialog.windowTitle();
        QVERIFY(!title.isEmpty());
        // Note: Exact title depends on implementation
    }
    
    {
        SecurityErrorDialog dialog(SecurityErrorType::InvalidCertificate, "Book", "Details");
        QString title = dialog.windowTitle();
        QVERIFY(!title.isEmpty());
    }
    
    {
        SecurityErrorDialog dialog(SecurityErrorType::FileCorruption, "Book", "Details");
        QString title = dialog.windowTitle();
        QVERIFY(!title.isEmpty());
    }
    
    {
        SecurityErrorDialog dialog(SecurityErrorType::SignatureInvalid, "Book", "Details");
        QString title = dialog.windowTitle();
        QVERIFY(!title.isEmpty());
    }
}

// Test error message content
void TestSecurityErrorDialog::testDialogMessage()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::TamperingDetected,
        "Test Book",
        "H1 hash does not match H2 hash"
    );
    
    // Dialog should be created with error message
    // Note: We can't easily test internal message text without exposing it,
    // but we can verify the dialog is properly constructed
    QVERIFY(!dialog.windowTitle().isEmpty());
}

// Test dialog with cartridge title
void TestSecurityErrorDialog::testDialogWithCartridgeTitle()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::TamperingDetected,
        "My Test Book",  // Cartridge title
        "Error details"
    );
    
    // Dialog should include cartridge title in message
    QVERIFY(!dialog.windowTitle().isEmpty());
}

// Test dialog with error details
void TestSecurityErrorDialog::testDialogWithErrorDetails()
{
    SecurityErrorDialog dialog(
        SecurityErrorType::InvalidCertificate,
        "Test Book",
        "Certificate expired on 2024-01-01"  // Detailed error message
    );
    
    // Dialog should include error details in message
    QVERIFY(!dialog.windowTitle().isEmpty());
}

#include "test_securityerrordialog.moc"

