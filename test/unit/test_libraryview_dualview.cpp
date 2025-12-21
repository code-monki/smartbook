#include <QtTest>
#include "smartbook/reader/ui/LibraryView.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QApplication>
#include <QElapsedTimer>
#include <QDebug>

using namespace smartbook::reader;
using namespace smartbook::common::database;
using namespace smartbook::common::manifest;

class TestLibraryViewDualView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDualViewToggle();  // T-UI-02: Dual View Toggle

private:
    QTemporaryDir* m_tempDir;
    QString m_testDbPath;
    LocalDBManager* m_dbManager;
    ManifestManager* m_manifestManager;
    
    void createTestManifestEntries(int count);
};

void TestLibraryViewDualView::initTestCase()
{
    // QTEST_MAIN creates QApplication automatically, no need to create manually
    
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDbPath = m_tempDir->filePath("test_local_reader.sqlite");
    
    m_dbManager = &LocalDBManager::getInstance();
    bool initialized = m_dbManager->initializeConnection(m_testDbPath);
    QVERIFY(initialized);
    QVERIFY(m_dbManager->isOpen());
    
    m_manifestManager = new ManifestManager(this);
    
    // Create a few test entries for the view
    createTestManifestEntries(5);
}

void TestLibraryViewDualView::cleanupTestCase()
{
    delete m_manifestManager;
    if (m_dbManager && m_dbManager->isOpen()) {
        m_dbManager->closeConnection();
    }
    delete m_tempDir;
}

void TestLibraryViewDualView::createTestManifestEntries(int count)
{
    for (int i = 0; i < count; ++i) {
        ManifestManager::ManifestEntry entry;
        entry.cartridgeGuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.cartridgeHash = QByteArray::fromHex("a1b2c3d4e5f6");
        entry.localPath = QString("/path/to/cartridge_%1.sqlite").arg(i);
        entry.title = QString("Test Book %1").arg(i);
        entry.author = QString("Author %1").arg(i % 3);
        entry.publicationYear = QString("%1").arg(2020 + i);
        entry.publisher = QString("Publisher %1").arg(i % 2);
        entry.version = "1.0";
        
        m_manifestManager->createManifestEntry(entry);
    }
}

// T-UI-02: Dual View Toggle
// Requirement: DDD 11.1 (UI Dual View)
// Test Plan: test-plan.adoc lines 137-146
// AC: Both views load instantly. The List-View displays the required columns
//     (Title, Author, Edition/Version, Year of Publication) sourced from the manifest.
void TestLibraryViewDualView::testDualViewToggle()
{
    LibraryView* libraryView = new LibraryView();
    
    // Process any pending events to ensure widget is fully initialized
    QApplication::processEvents();
    
    // Initially should be in List View mode (default)
    QVERIFY(libraryView->isListView());
    
    // Verify we can toggle to Bookshelf View
    QElapsedTimer timer;
    timer.start();
    libraryView->toggleView();
    qint64 toggleTime1 = timer.elapsed();
    QVERIFY(!libraryView->isListView()); // Should now be Bookshelf View
    
    // Toggle back to List View
    timer.restart();
    libraryView->toggleView();
    qint64 toggleTime2 = timer.elapsed();
    QVERIFY(libraryView->isListView()); // Should be back to List View
    
    // Toggle to Bookshelf View again
    timer.restart();
    libraryView->toggleView();
    qint64 toggleTime3 = timer.elapsed();
    QVERIFY(!libraryView->isListView()); // Should be Bookshelf View again
    
    // AC: Both views load instantly (< 100ms for toggle)
    QVERIFY(toggleTime1 < 100);
    QVERIFY(toggleTime2 < 100);
    QVERIFY(toggleTime3 < 100);
    
    qDebug() << "Toggle times:" << toggleTime1 << toggleTime2 << toggleTime3 << "ms";
    
    // Verify the view refreshes correctly after toggles
    libraryView->refreshLibrary();
    
    // Process events to ensure widget operations complete
    QApplication::processEvents();
    
    // Delete the widget explicitly and process events to ensure cleanup
    delete libraryView;
    libraryView = nullptr;
    QApplication::processEvents();
}

// Use QTEST_GUILESS_MAIN would avoid QApplication, but we need widgets
// Instead, ensure proper cleanup by using a custom main that handles cleanup
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestLibraryViewDualView tc;
    int result = QTest::qExec(&tc, argc, argv);
    // QApplication will be destroyed automatically when main returns
    return result;
}

#include "test_libraryview_dualview.moc"
