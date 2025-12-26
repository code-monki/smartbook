#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QRegularExpression>

namespace smartbook {
namespace common {
namespace database {

CartridgeDBConnector::CartridgeDBConnector(QObject* parent)
    : QObject(parent)
{
}

CartridgeDBConnector::~CartridgeDBConnector() {
    closeConnection();
}

bool CartridgeDBConnector::openCartridge(const QString& cartridgePath) {
    if (m_isOpen) {
        closeConnection();
    }

    m_cartridgePath = cartridgePath;
    m_validationError.clear();

    // Create unique connection name for this instance
    QString connectionName = QString("CartridgeDB_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    // Add SQLite driver with unique connection name
    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName(cartridgePath);

    if (!m_database.open()) {
        m_validationError = QString("Failed to open cartridge database: %1").arg(m_database.lastError().text());
        qCritical() << m_validationError;
        return false;
    }

    // Configure connection
    configureConnection();

    // Validate database schema
    if (!validateDatabase()) {
        qCritical() << "Cartridge database validation failed:" << m_validationError;
        closeConnection();
        return false;
    }

    // Extract cartridge GUID
    QSqlQuery query(m_database);
    if (query.exec("SELECT cartridge_guid FROM Metadata LIMIT 1")) {
        if (query.next()) {
            m_cartridgeGuid = query.value(0).toString();
        }
    }

    m_isOpen = true;
    return true;
}

void CartridgeDBConnector::closeConnection() {
    if (m_database.isOpen()) {
        m_database.close();
    }
    m_database = QSqlDatabase(); // Remove connection
    m_isOpen = false;
    m_cartridgeGuid.clear();
}

bool CartridgeDBConnector::isOpen() const {
    return m_isOpen && m_database.isOpen();
}

QString CartridgeDBConnector::getCartridgeGuid() const {
    return m_cartridgeGuid;
}

QSqlQuery CartridgeDBConnector::executeQuery(const QString& queryString) {
    if (!m_isOpen || !m_database.isOpen()) {
        qWarning() << "CartridgeDBConnector: Cannot execute query - database not open";
        return QSqlQuery();
    }
    
    QSqlQuery query(m_database);
    if (!query.exec(queryString)) {
        qWarning() << "CartridgeDBConnector: Query failed:" << queryString;
        qWarning() << "CartridgeDBConnector: Error:" << query.lastError().text();
        qWarning() << "CartridgeDBConnector: Database error type:" << query.lastError().type();
        qWarning() << "CartridgeDBConnector: Cartridge GUID:" << m_cartridgeGuid;
    }
    return query;
}

bool CartridgeDBConnector::beginTransaction() {
    return m_database.transaction();
}

bool CartridgeDBConnector::commitTransaction() {
    return m_database.commit();
}

bool CartridgeDBConnector::rollbackTransaction() {
    return m_database.rollback();
}

QSqlDatabase& CartridgeDBConnector::getDatabase() {
    return m_database;
}

void CartridgeDBConnector::configureConnection() {
    QSqlQuery query(m_database);

    // Enable WAL mode
    if (!query.exec("PRAGMA journal_mode=WAL")) {
        qWarning() << "Failed to enable WAL mode:" << query.lastError().text();
    }

    // Set page size to 4096 bytes
    if (!query.exec("PRAGMA page_size=4096")) {
        qWarning() << "Failed to set page size:" << query.lastError().text();
    }

    // Set cache size (1000 pages for cartridge)
    if (!query.exec("PRAGMA cache_size=-1000")) {
        qWarning() << "Failed to set cache size:" << query.lastError().text();
    }

    // Set synchronous mode to NORMAL (safe with WAL)
    if (!query.exec("PRAGMA synchronous=NORMAL")) {
        qWarning() << "Failed to set synchronous mode:" << query.lastError().text();
    }

    // Enable foreign keys
    if (!query.exec("PRAGMA foreign_keys=ON")) {
        qWarning() << "Failed to enable foreign keys:" << query.lastError().text();
    }

    // Set busy timeout (5 seconds)
    if (!query.exec("PRAGMA busy_timeout=5000")) {
        qWarning() << "Failed to set busy timeout:" << query.lastError().text();
    }
    
    // Create User_Data table if it doesn't exist
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS User_Data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            form_id TEXT NOT NULL,
            data_json TEXT NOT NULL,
            saved_timestamp INTEGER NOT NULL,
            UNIQUE(form_id)
        )
    )");
}

bool CartridgeDBConnector::saveFormData(const QString& formId, const QString& dataJson)
{
    if (!m_isOpen || !m_database.isOpen()) {
        qWarning() << "Cannot save form data: cartridge not open";
        return false;
    }
    
    QSqlQuery query(m_database);
    
    // Use INSERT OR REPLACE to handle updates
    query.prepare(R"(
        INSERT OR REPLACE INTO User_Data (form_id, data_json, saved_timestamp)
        VALUES (?, ?, ?)
    )");
    
    query.addBindValue(formId);
    query.addBindValue(dataJson);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    
    if (!query.exec()) {
        qCritical() << "Failed to save form data:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QString CartridgeDBConnector::loadFormData(const QString& formId)
{
    if (!m_isOpen || !m_database.isOpen()) {
        qWarning() << "Cannot load form data: cartridge not open";
        return QString();
    }
    
    QSqlQuery query(m_database);
    query.prepare("SELECT data_json FROM User_Data WHERE form_id = ?");
    query.addBindValue(formId);
    
    if (!query.exec()) {
        qWarning() << "Failed to load form data:" << query.lastError().text();
        return QString();
    }
    
    if (query.next()) {
        return query.value(0).toString();
    }
    
    return QString(); // Not found
}

bool CartridgeDBConnector::validateDatabase()
{
    if (!m_database.isOpen()) {
        m_validationError = "Database is not open";
        return false;
    }

    // Check for required tables
    if (!hasRequiredTables()) {
        return false;
    }

    // Validate Metadata table has required columns
    QStringList metadataColumns = {"cartridge_guid", "title", "author", "version", "publication_year", "schema_version"};
    if (!checkTableSchema("Metadata", metadataColumns)) {
        m_validationError = QString("Metadata table missing required columns: %1").arg(m_validationError);
        return false;
    }

    // Validate cartridge_guid is UUID v4 format
    QSqlQuery query(m_database);
    if (query.exec("SELECT cartridge_guid FROM Metadata LIMIT 1")) {
        if (query.next()) {
            QString guid = query.value(0).toString();
            // Basic UUID v4 validation (format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
            QRegularExpression uuidRegex("^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$", 
                                         QRegularExpression::CaseInsensitiveOption);
            if (!uuidRegex.match(guid).hasMatch()) {
                m_validationError = QString("Invalid cartridge_guid format: %1 (expected UUID v4)").arg(guid);
                return false;
            }
        } else {
            m_validationError = "Metadata table is empty (cartridge_guid not found)";
            return false;
        }
    } else {
        m_validationError = QString("Failed to query Metadata table: %1").arg(query.lastError().text());
        return false;
    }

    // Validate schema_version
    if (query.exec("SELECT schema_version FROM Metadata LIMIT 1")) {
        if (query.next()) {
            QString schemaVersion = query.value(0).toString();
            // For Phase 1, we expect schema version "1.0"
            if (schemaVersion != "1.0") {
                m_validationError = QString("Unsupported schema_version: %1 (expected 1.0)").arg(schemaVersion);
                return false;
            }
        }
    }

    return true;
}

bool CartridgeDBConnector::hasRequiredTables()
{
    // Required tables for Phase 1 (minimum set)
    QStringList requiredTables = {
        "Metadata",
        "Content_Pages"
    };

    for (const QString& tableName : requiredTables) {
        if (!checkTableExists(tableName)) {
            m_validationError = QString("Required table missing: %1").arg(tableName);
            return false;
        }
    }

    return true;
}

bool CartridgeDBConnector::checkTableExists(const QString& tableName)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    query.addBindValue(tableName);
    
    if (!query.exec()) {
        qWarning() << "Failed to check table existence:" << query.lastError().text();
        return false;
    }
    
    return query.next();
}

bool CartridgeDBConnector::checkTableSchema(const QString& tableName, const QStringList& requiredColumns)
{
    QSqlQuery query(m_database);
    query.prepare(QString("PRAGMA table_info(%1)").arg(tableName));
    
    if (!query.exec()) {
        m_validationError = QString("Failed to get table info for %1: %2").arg(tableName, query.lastError().text());
        return false;
    }
    
    QStringList existingColumns;
    while (query.next()) {
        existingColumns.append(query.value(1).toString()); // Column name is at index 1
    }
    
    QStringList missingColumns;
    for (const QString& requiredCol : requiredColumns) {
        if (!existingColumns.contains(requiredCol)) {
            missingColumns.append(requiredCol);
        }
    }
    
    if (!missingColumns.isEmpty()) {
        m_validationError = QString("Missing columns in %1: %2").arg(tableName, missingColumns.join(", "));
        return false;
    }
    
    return true;
}

} // namespace database
} // namespace common
} // namespace smartbook
