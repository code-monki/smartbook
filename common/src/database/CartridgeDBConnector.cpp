#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlError>
#include <QDebug>
#include <QUuid>

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
}

} // namespace database
} // namespace common
} // namespace smartbook
