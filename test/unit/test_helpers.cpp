#include "test_helpers.h"
#include <QSqlError>
#include <QDebug>

namespace TestHelpers {

bool createMinimalCartridge(const QString& path, const QString& guid, 
                            const QString& title, const QString& connectionName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qWarning() << "Failed to create test cartridge:" << path;
        return false;
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL
        )
    )");
    
    // Insert metadata
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year) VALUES (?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(title);
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    if (!query.exec()) {
        qWarning() << "Failed to insert metadata:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }
    
    // Create minimal content tables for H2 calculation
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY,
            content_html TEXT
        )
    )");
    query.exec(QString("INSERT INTO Content_Pages (page_id, content_html) VALUES (1, '<p>%1 content</p>')").arg(title));
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Content_Themes (
            theme_id TEXT PRIMARY KEY,
            theme_config_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_json TEXT
        )
    )");
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT
        )
    )");
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    
    return true;
}

QString createL1Cartridge(const QString& path, const QString& guid, const QString& connectionName)
{
    if (!createMinimalCartridge(path, guid, "L1 Test Book", connectionName)) {
        return QString();
    }
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName + "_security");
    db.setDatabaseName(path);
    
    if (!db.open()) {
        return QString();
    }
    
    QSqlQuery query(db);
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            security_id INTEGER PRIMARY KEY,
            hash_digest BLOB,
            certificate_data BLOB,
            signature_data BLOB
        )
    )");
    
    // L1 has CA-signed certificate marker
    QByteArray certData = "CA_SIGNED_CERTIFICATE_PLACEHOLDER";
    QByteArray hashData = QByteArray::fromHex("a1b2c3d4e5f6"); // Placeholder, will be updated
    
    query.prepare("INSERT INTO Cartridge_Security (hash_digest, certificate_data) VALUES (?, ?)");
    query.addBindValue(hashData);
    query.addBindValue(certData);
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName + "_security");
    
    return path;
}

QString createL2Cartridge(const QString& path, const QString& guid, const QString& connectionName)
{
    if (!createMinimalCartridge(path, guid, "L2 Test Book", connectionName)) {
        return QString();
    }
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName + "_security");
    db.setDatabaseName(path);
    
    if (!db.open()) {
        return QString();
    }
    
    QSqlQuery query(db);
    
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            security_id INTEGER PRIMARY KEY,
            hash_digest BLOB,
            certificate_data BLOB,
            signature_data BLOB
        )
    )");
    
    // L2 has self-signed certificate (no CA_SIGNED marker)
    QByteArray certData = "SELF_SIGNED_CERTIFICATE_PLACEHOLDER";
    QByteArray hashData = QByteArray::fromHex("b2c3d4e5f6a1"); // Placeholder, will be updated
    
    query.prepare("INSERT INTO Cartridge_Security (hash_digest, certificate_data) VALUES (?, ?)");
    query.addBindValue(hashData);
    query.addBindValue(certData);
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName + "_security");
    
    return path;
}

QString createL3Cartridge(const QString& path, const QString& guid, const QString& connectionName)
{
    // L3 has no security table
    if (!createMinimalCartridge(path, guid, "L3 Test Book", connectionName)) {
        return QString();
    }
    
    return path;
}

bool updateCartridgeH1Hash(const QString& cartridgePath, const QByteArray& h2Hash, const QString& connectionName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(cartridgePath);
    
    if (!db.open()) {
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("UPDATE Cartridge_Security SET hash_digest = ?");
    query.addBindValue(h2Hash);
    bool success = query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    
    return success;
}

} // namespace TestHelpers
