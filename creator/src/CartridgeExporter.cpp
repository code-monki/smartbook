#include "smartbook/creator/CartridgeExporter.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QHash>
#include <QVariant>
#include <QDebug>

namespace smartbook {
namespace creator {

CartridgeExporter::CartridgeExporter(QObject* parent)
    : QObject(parent)
{
}

bool CartridgeExporter::exportCartridge(const QString& cartridgePath, const QHash<QString, QVariant>& /* metadata */) {
    qDebug() << "Exporting cartridge to:" << cartridgePath;
    
    // Create cartridge database
    if (!createCartridgeSchema(cartridgePath)) {
        emit exportComplete(false, "Failed to create cartridge schema");
        return false;
    }

    // TODO: Add content pages, forms, embedded apps, etc.
    
    emit exportProgress(100);
    emit exportComplete(true, QString());
    return true;
}

bool CartridgeExporter::signCartridge(const QString& cartridgePath, const QString& certificatePath,
                                     const QString& privateKeyPath, int securityLevel) {
    Q_UNUSED(certificatePath);
    Q_UNUSED(privateKeyPath);
    Q_UNUSED(securityLevel);
    
    qDebug() << "Signing cartridge:" << cartridgePath;
    // TODO: Implement cartridge signing
    return true;
}

bool CartridgeExporter::createCartridgeSchema(const QString& cartridgePath) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "CartridgeCreate");
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        qCritical() << "Failed to create cartridge database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    // Create Metadata table
    QString metadataTable = R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            cartridge_guid TEXT NOT NULL UNIQUE,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publisher TEXT,
            version TEXT NOT NULL,
            publication_year TEXT NOT NULL,
            tags_json TEXT,
            cover_image_path TEXT,
            schema_version TEXT NOT NULL,
            series_name TEXT,
            edition_name TEXT,
            series_order INTEGER
        )
    )";

    if (!query.exec(metadataTable)) {
        qCritical() << "Failed to create Metadata table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Content_Pages table
    QString contentPagesTable = R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_order INTEGER NOT NULL UNIQUE,
            chapter_title TEXT,
            html_content TEXT NOT NULL,
            associated_css TEXT
        )
    )";

    if (!query.exec(contentPagesTable)) {
        qCritical() << "Failed to create Content_Pages table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Form_Definitions table
    QString formDefinitionsTable = R"(
        CREATE TABLE IF NOT EXISTS Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_schema_json TEXT NOT NULL,
            form_version INTEGER NOT NULL DEFAULT 1,
            migration_rules_json TEXT
        )
    )";

    if (!query.exec(formDefinitionsTable)) {
        qCritical() << "Failed to create Form_Definitions table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create User_Data table
    QString userDataTable = R"(
        CREATE TABLE IF NOT EXISTS User_Data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            form_key TEXT NOT NULL,
            form_version INTEGER,
            migrated_from_version INTEGER,
            timestamp INTEGER NOT NULL,
            serialized_data TEXT NOT NULL
        )
    )";

    if (!query.exec(userDataTable)) {
        qCritical() << "Failed to create User_Data table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Settings table
    QString settingsTable = R"(
        CREATE TABLE IF NOT EXISTS Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT NOT NULL,
            setting_type TEXT NOT NULL,
            description TEXT
        )
    )";

    if (!query.exec(settingsTable)) {
        qCritical() << "Failed to create Settings table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Embedded_Apps table
    QString embeddedAppsTable = R"(
        CREATE TABLE IF NOT EXISTS Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT NOT NULL,
            manifest_json TEXT NOT NULL,
            entry_html TEXT NOT NULL,
            js_code BLOB,
            css_code BLOB,
            additional_resources BLOB
        )
    )";

    if (!query.exec(embeddedAppsTable)) {
        qCritical() << "Failed to create Embedded_Apps table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Resources table
    QString resourcesTable = R"(
        CREATE TABLE IF NOT EXISTS Resources (
            resource_id TEXT PRIMARY KEY,
            resource_path TEXT NOT NULL,
            resource_type TEXT NOT NULL,
            resource_data BLOB NOT NULL,
            mime_type TEXT NOT NULL
        )
    )";

    if (!query.exec(resourcesTable)) {
        qCritical() << "Failed to create Resources table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Content_Themes table
    QString contentThemesTable = R"(
        CREATE TABLE IF NOT EXISTS Content_Themes (
            theme_id TEXT PRIMARY KEY,
            theme_name TEXT NOT NULL,
            is_builtin INTEGER NOT NULL DEFAULT 0,
            theme_config_json TEXT NOT NULL,
            is_active INTEGER DEFAULT 0
        )
    )";

    if (!query.exec(contentThemesTable)) {
        qCritical() << "Failed to create Content_Themes table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Cartridge_Security table
    QString cartridgeSecurityTable = R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            digest_type TEXT NOT NULL,
            hash_digest BLOB NOT NULL,
            digital_signature BLOB NOT NULL,
            public_key_fingerprint TEXT NOT NULL,
            certificate_data BLOB
        )
    )";

    if (!query.exec(cartridgeSecurityTable)) {
        qCritical() << "Failed to create Cartridge_Security table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Navigation_Structure table
    QString navigationStructureTable = R"(
        CREATE TABLE IF NOT EXISTS Navigation_Structure (
            nav_id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_name TEXT NOT NULL,
            group_order INTEGER NOT NULL,
            item_label TEXT NOT NULL,
            item_order INTEGER NOT NULL,
            target_type TEXT NOT NULL,
            target_value TEXT NOT NULL,
            parent_item_id INTEGER,
            metadata_json TEXT
        )
    )";

    if (!query.exec(navigationStructureTable)) {
        qCritical() << "Failed to create Navigation_Structure table:" << query.lastError().text();
        db.close();
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("CartridgeCreate");

    return true;
}

QByteArray CartridgeExporter::calculateContentHash(const QString& cartridgePath) {
    // TODO: Implement content hash calculation (same as SignatureVerifier)
    Q_UNUSED(cartridgePath);
    return QByteArray();
}

} // namespace creator
} // namespace smartbook
