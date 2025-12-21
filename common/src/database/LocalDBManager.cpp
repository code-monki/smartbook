#include "smartbook/common/database/LocalDBManager.h"
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

namespace smartbook {
namespace common {
namespace database {

LocalDBManager& LocalDBManager::getInstance() {
    static LocalDBManager instance;
    return instance;
}

bool LocalDBManager::initializeConnection(const QString& dbPath) {
    if (m_initialized && m_database.isOpen()) {
        return true;
    }

    // Use provided path or default location
    QString actualPath = dbPath;
    if (actualPath.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        actualPath = dataDir + "/local_reader.sqlite";
    }

    // Add SQLite driver
    m_database = QSqlDatabase::addDatabase("QSQLITE", "LocalReaderDB");
    m_database.setDatabaseName(actualPath);

    // Configure SQLite (WAL mode, etc.)
    if (!m_database.open()) {
        qCritical() << "Failed to open local database:" << m_database.lastError().text();
        return false;
    }

    // Configure SQLite settings (WAL mode, page size, etc.)
    QSqlQuery query(m_database);
    
    // Enable WAL mode
    if (!query.exec("PRAGMA journal_mode=WAL")) {
        qWarning() << "Failed to enable WAL mode:" << query.lastError().text();
    }

    // Set page size to 4096 bytes
    if (!query.exec("PRAGMA page_size=4096")) {
        qWarning() << "Failed to set page size:" << query.lastError().text();
    }

    // Set cache size (2000 pages for local DB)
    if (!query.exec("PRAGMA cache_size=-2000")) {
        qWarning() << "Failed to set cache size:" << query.lastError().text();
    }

    // Enable foreign keys
    if (!query.exec("PRAGMA foreign_keys=ON")) {
        qWarning() << "Failed to enable foreign keys:" << query.lastError().text();
    }

    // Create schema if needed
    if (!createSchema()) {
        qCritical() << "Failed to create database schema";
        return false;
    }

    m_initialized = true;
    return true;
}

QSqlDatabase& LocalDBManager::getDatabase() {
    return m_database;
}

QSqlQuery LocalDBManager::executeQuery(const QString& queryString) {
    QSqlQuery query(m_database);
    if (!query.exec(queryString)) {
        qWarning() << "Query failed:" << queryString;
        qWarning() << "Error:" << query.lastError().text();
    }
    return query;
}

void LocalDBManager::closeConnection() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    m_initialized = false;
}

bool LocalDBManager::isOpen() const {
    return m_database.isOpen();
}

bool LocalDBManager::createSchema() {
    QSqlQuery query(m_database);

    // Create Local_Library_Manifest table
    QString manifestTable = R"(
        CREATE TABLE IF NOT EXISTS Local_Library_Manifest (
            manifest_id INTEGER PRIMARY KEY AUTOINCREMENT,
            cartridge_guid TEXT NOT NULL UNIQUE,
            cartridge_hash BLOB NOT NULL,
            local_path TEXT NOT NULL,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publisher TEXT,
            version TEXT,
            publication_year TEXT NOT NULL,
            cover_image_data BLOB,
            last_opened INTEGER,
            location_status TEXT,
            series_name TEXT,
            edition_name TEXT,
            series_order INTEGER
        )
    )";

    if (!query.exec(manifestTable)) {
        qCritical() << "Failed to create Local_Library_Manifest table:" << query.lastError().text();
        return false;
    }

    // Create Local_Trust_Registry table
    QString trustTable = R"(
        CREATE TABLE IF NOT EXISTS Local_Trust_Registry (
            trust_id INTEGER PRIMARY KEY AUTOINCREMENT,
            cartridge_guid TEXT NOT NULL UNIQUE,
            trust_policy TEXT NOT NULL,
            granted_timestamp INTEGER NOT NULL,
            last_verified_timestamp INTEGER,
            FOREIGN KEY (cartridge_guid) REFERENCES Local_Library_Manifest(cartridge_guid)
        )
    )";

    if (!query.exec(trustTable)) {
        qCritical() << "Failed to create Local_Trust_Registry table:" << query.lastError().text();
        return false;
    }

    // Create Local_Cartridge_Groups table
    QString groupsTable = R"(
        CREATE TABLE IF NOT EXISTS Local_Cartridge_Groups (
            group_id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_name TEXT NOT NULL,
            group_type TEXT NOT NULL,
            created_timestamp INTEGER NOT NULL,
            last_modified_timestamp INTEGER NOT NULL,
            description TEXT
        )
    )";

    if (!query.exec(groupsTable)) {
        qCritical() << "Failed to create Local_Cartridge_Groups table:" << query.lastError().text();
        return false;
    }

    // Create Local_Cartridge_Group_Members table
    QString membersTable = R"(
        CREATE TABLE IF NOT EXISTS Local_Cartridge_Group_Members (
            membership_id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id INTEGER NOT NULL,
            cartridge_guid TEXT NOT NULL,
            added_timestamp INTEGER NOT NULL,
            display_order INTEGER,
            FOREIGN KEY (group_id) REFERENCES Local_Cartridge_Groups(group_id),
            FOREIGN KEY (cartridge_guid) REFERENCES Local_Library_Manifest(cartridge_guid),
            UNIQUE(group_id, cartridge_guid)
        )
    )";

    if (!query.exec(membersTable)) {
        qCritical() << "Failed to create Local_Cartridge_Group_Members table:" << query.lastError().text();
        return false;
    }

    // Create Local_User_Settings table
    QString userSettingsTable = R"(
        CREATE TABLE IF NOT EXISTS Local_User_Settings (
            settings_id INTEGER PRIMARY KEY AUTOINCREMENT,
            cartridge_guid TEXT NOT NULL,
            setting_key TEXT NOT NULL,
            setting_value TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            FOREIGN KEY (cartridge_guid) REFERENCES Local_Library_Manifest(cartridge_guid),
            UNIQUE(cartridge_guid, setting_key)
        )
    )";

    if (!query.exec(userSettingsTable)) {
        qCritical() << "Failed to create Local_User_Settings table:" << query.lastError().text();
        return false;
    }

    // Create indexes for performance
    query.exec("CREATE INDEX IF NOT EXISTS idx_manifest_guid ON Local_Library_Manifest(cartridge_guid)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_trust_guid ON Local_Trust_Registry(cartridge_guid)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_groups_type ON Local_Cartridge_Groups(group_type)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_members_group ON Local_Cartridge_Group_Members(group_id, cartridge_guid)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_manifest_series ON Local_Library_Manifest(series_name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_manifest_edition ON Local_Library_Manifest(edition_name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_user_settings_guid ON Local_User_Settings(cartridge_guid, setting_key)");

    return true;
}

} // namespace database
} // namespace common
} // namespace smartbook
