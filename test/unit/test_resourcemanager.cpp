#include <QtTest>
#include "smartbook/creator/ResourceManager.h"
#include "test_helpers.h"
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSignalSpy>
#include <QByteArray>
#include <QDebug>

using namespace smartbook::creator;

class TestResourceManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // T-CT-25: Asset Management (FR-CT-3.24)
    void testResourceImport();
    void testResourceImportFromData();
    void testResourceReference();
    
    // T-CT-26: Resource Storage (FR-CT-3.25)
    void testResourceStorage();
    void testResourcePersistence();
    void testResourceDeletion();
    void testResourceList();

private:
    QTemporaryDir* m_tempDir;
    QString m_testCartridgePath;
    ResourceManager* m_manager;
    
    QString createTestCartridge();
    void verifyResourceInDatabase(const QString& resourceId, const QString& resourceType);
};

// Custom main to ensure proper QApplication lifecycle
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestResourceManager tc;
    int result = QTest::qExec(&tc, argc, argv);
    return result;
}

void TestResourceManager::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    m_testCartridgePath = m_tempDir->filePath("test_cartridge.sqlite");
    m_manager = new ResourceManager(this);
    QVERIFY(m_manager != nullptr);
}

void TestResourceManager::cleanupTestCase()
{
    if (m_manager) {
        m_manager->closeCartridge();
    }
    delete m_manager;
    delete m_tempDir;
}

QString TestResourceManager::createTestCartridge()
{
    QString path = m_tempDir->filePath("source_cartridge.sqlite");
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString connectionName = QString("TestCartridge_ResourceManager_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    bool created = TestHelpers::createMinimalCartridge(path, guid, "Test Book", connectionName);
    if (!created) {
        return QString();
    }
    
    return path;
}

void TestResourceManager::verifyResourceInDatabase(const QString& resourceId, const QString& resourceType)
{
    QString connectionName = QString("TestCartridge_Verify_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(m_testCartridgePath);
    QVERIFY(db.open());
    
    QSqlQuery query(db);
    query.prepare("SELECT resource_type FROM Resources WHERE resource_id = ?");
    query.addBindValue(resourceId);
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), resourceType);
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
}

// T-CT-25: Asset Management (FR-CT-3.24)
// AC: Resource is stored within the cartridge. Resource can be referenced in content.
void TestResourceManager::testResourceImport()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    // Copy cartridge to test path
    QFile::copy(cartridgePath, m_testCartridgePath);
    
    bool opened = m_manager->openCartridge(m_testCartridgePath);
    QVERIFY(opened);
    
    // Create a test image file
    QString testImagePath = m_tempDir->filePath("test_image.png");
    QFile testImage(testImagePath);
    QVERIFY(testImage.open(QIODevice::WriteOnly));
    QByteArray imageData("fake PNG data");
    testImage.write(imageData);
    testImage.close();
    
    // Import resource
    QSignalSpy changedSpy(m_manager, &ResourceManager::resourceListChanged);
    QString resourceId = m_manager->importResource(testImagePath);
    QVERIFY(!resourceId.isEmpty());
    QCOMPARE(changedSpy.count(), 1);
    
    // Verify resource exists
    QVERIFY(m_manager->resourceExists(resourceId));
    
    // Verify resource data
    ResourceInfo info = m_manager->getResource(resourceId);
    QVERIFY(info.isValid());
    QCOMPARE(info.resourceType, QString("image"));
    QCOMPARE(info.resourceData, imageData);
    
    // Verify in database
    verifyResourceInDatabase(resourceId, "image");
}

// T-CT-25: Asset Management - Import from data
void TestResourceManager::testResourceImportFromData()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QFile::copy(cartridgePath, m_testCartridgePath);
    
    bool opened = m_manager->openCartridge(m_testCartridgePath);
    QVERIFY(opened);
    
    // Import resource from data
    QByteArray fontData("fake font data");
    QString resourceId = "test_font_123";
    
    QSignalSpy changedSpy(m_manager, &ResourceManager::resourceListChanged);
    bool imported = m_manager->importResourceData(fontData, resourceId, "font", "font/ttf");
    QVERIFY(imported);
    QCOMPARE(changedSpy.count(), 1);
    
    // Verify resource
    ResourceInfo info = m_manager->getResource(resourceId);
    QVERIFY(info.isValid());
    QCOMPARE(info.resourceId, resourceId);
    QCOMPARE(info.resourceType, QString("font"));
    QCOMPARE(info.resourceData, fontData);
    QCOMPARE(info.mimeType, QString("font/ttf"));
}

// T-CT-25: Asset Management - Resource reference
void TestResourceManager::testResourceReference()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QFile::copy(cartridgePath, m_testCartridgePath);
    
    bool opened = m_manager->openCartridge(m_testCartridgePath);
    QVERIFY(opened);
    
    // Import a resource
    QByteArray imageData("fake image data");
    QString resourceId = "image_001";
    bool imported = m_manager->importResourceData(imageData, resourceId, "image", "image/png");
    QVERIFY(imported);
    
    // Get resource data (simulating content reference)
    QByteArray retrievedData = m_manager->getResourceData("image_001");
    QCOMPARE(retrievedData, imageData);
    
    // Verify resource can be retrieved by ID
    ResourceInfo info = m_manager->getResource("image_001");
    QVERIFY(info.isValid());
    QCOMPARE(info.resourceData, imageData);
}

// T-CT-26: Resource Storage (FR-CT-3.25)
// AC: All resources are stored within the cartridge database. Cartridge file is portable.
void TestResourceManager::testResourceStorage()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    // Use a unique path for this test to avoid conflicts
    QString uniquePath = m_tempDir->filePath("test_storage_cartridge.sqlite");
    QFile::copy(cartridgePath, uniquePath);
    
    bool opened = m_manager->openCartridge(uniquePath);
    QVERIFY(opened);
    
    // Import multiple resources
    QByteArray image1("image1 data");
    QByteArray image2("image2 data");
    QByteArray font1("font1 data");
    
    bool imported1 = m_manager->importResourceData(image1, "img1", "image", "image/png");
    bool imported2 = m_manager->importResourceData(image2, "img2", "image", "image/jpeg");
    bool imported3 = m_manager->importResourceData(font1, "font1", "font", "font/ttf");
    
    QVERIFY(imported1);
    QVERIFY(imported2);
    QVERIFY(imported3);
    
    // Close and reopen cartridge
    m_manager->closeCartridge();
    opened = m_manager->openCartridge(uniquePath);
    QVERIFY(opened);
    
    // Verify all resources persist
    QVERIFY(m_manager->resourceExists("img1"));
    QVERIFY(m_manager->resourceExists("img2"));
    QVERIFY(m_manager->resourceExists("font1"));
    
    QList<ResourceInfo> resources = m_manager->getResources();
    QCOMPARE(resources.size(), 3);
    
    m_manager->closeCartridge();
}

// T-CT-26: Resource Storage - Persistence
void TestResourceManager::testResourcePersistence()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QFile::copy(cartridgePath, m_testCartridgePath);
    
    bool opened = m_manager->openCartridge(m_testCartridgePath);
    QVERIFY(opened);
    
    // Import resource
    QByteArray resourceData("persistent resource data");
    QString resourceId = "persistent_001";
    bool imported = m_manager->importResourceData(resourceData, resourceId, "image", "image/png");
    QVERIFY(imported);
    
    // Close cartridge
    m_manager->closeCartridge();
    
    // Reopen and verify persistence
    opened = m_manager->openCartridge(m_testCartridgePath);
    QVERIFY(opened);
    
    ResourceInfo info = m_manager->getResource("persistent_001");
    QVERIFY(info.isValid());
    QCOMPARE(info.resourceData, resourceData);
}

// T-CT-26: Resource Storage - Deletion
void TestResourceManager::testResourceDeletion()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    QFile::copy(cartridgePath, m_testCartridgePath);
    
    bool opened = m_manager->openCartridge(m_testCartridgePath);
    QVERIFY(opened);
    
    // Import resource
    QString resourceId = "delete_me";
    bool imported = m_manager->importResourceData(QByteArray("data"), resourceId, "image", "image/png");
    QVERIFY(imported);
    QVERIFY(m_manager->resourceExists("delete_me"));
    
    // Delete resource
    QSignalSpy changedSpy(m_manager, &ResourceManager::resourceListChanged);
    bool deleted = m_manager->deleteResource("delete_me");
    QVERIFY(deleted);
    QCOMPARE(changedSpy.count(), 1);
    
    // Verify deletion
    QVERIFY(!m_manager->resourceExists("delete_me"));
    
    ResourceInfo info = m_manager->getResource("delete_me");
    QVERIFY(!info.isValid());
}

// T-CT-26: Resource Storage - List
void TestResourceManager::testResourceList()
{
    QString cartridgePath = createTestCartridge();
    QVERIFY(!cartridgePath.isEmpty());
    
    // Use a unique path for this test to avoid conflicts
    QString uniquePath = m_tempDir->filePath("test_list_cartridge.sqlite");
    QFile::copy(cartridgePath, uniquePath);
    
    bool opened = m_manager->openCartridge(uniquePath);
    QVERIFY(opened);
    
    // Import multiple resources
    bool imported1 = m_manager->importResourceData(QByteArray("data1"), "res1", "image", "image/png");
    bool imported2 = m_manager->importResourceData(QByteArray("data2"), "res2", "image", "image/jpeg");
    bool imported3 = m_manager->importResourceData(QByteArray("data3"), "res3", "font", "font/ttf");
    
    QVERIFY(imported1);
    QVERIFY(imported2);
    QVERIFY(imported3);
    
    // Get all resources
    QList<ResourceInfo> resources = m_manager->getResources();
    QVERIFY(resources.size() >= 3);
    
    // Verify all resources are present
    QStringList resourceIds;
    for (const ResourceInfo& info : resources) {
        resourceIds.append(info.resourceId);
    }
    
    QVERIFY(resourceIds.contains("res1"));
    QVERIFY(resourceIds.contains("res2"));
    QVERIFY(resourceIds.contains("res3"));
    
    m_manager->closeCartridge();
}

#include "test_resourcemanager.moc"

