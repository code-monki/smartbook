#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QDateTime>

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

    // Create unique connection name for this instance
    QString connectionName = QString("CartridgeDB_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    // Add SQLite driver with unique connection name
    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName(cartridgePath);

    if (!m_database.open()) {
        qCritical() << "Failed to open cartridge database:" << m_database.lastError().text();
        return false;
    }

    // Configure connection
    configureConnection();

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
    QSqlQuery query(m_database);
    if (!query.exec(queryString)) {
        qWarning() << "Query failed:" << queryString;
        qWarning() << "Error:" << query.lastError().text();
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

} // namespace database
} // namespace common
} // namespace smartbook
