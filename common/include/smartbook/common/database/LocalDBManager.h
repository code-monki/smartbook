#ifndef SMARTBOOK_COMMON_DATABASE_LOCALDBMANAGER_H
#define SMARTBOOK_COMMON_DATABASE_LOCALDBMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QObject>
#include <memory>

namespace smartbook {
namespace common {
namespace database {

/**
 * @brief Singleton manager for the local reader database
 * 
 * Manages the single connection to the global manifest and trust registry.
 * Used by the Library Manager and the Signature Verifier.
 * 
 * Implements the Singleton pattern to ensure only one connection exists.
 */
class LocalDBManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the LocalDBManager instance
     */
    static LocalDBManager& getInstance();

    /**
     * @brief Initialize the database connection
     * @param dbPath Path to the local reader database file
     * @return true if initialization successful, false otherwise
     */
    bool initializeConnection(const QString& dbPath);

    /**
     * @brief Get the database connection
     * @return Reference to the QSqlDatabase
     */
    QSqlDatabase& getDatabase();

    /**
     * @brief Execute a query on the local database
     * @param queryString SQL query string
     * @return QSqlQuery object with results
     */
    QSqlQuery executeQuery(const QString& queryString);

    /**
     * @brief Close the database connection
     */
    void closeConnection();

    /**
     * @brief Check if database is open
     * @return true if database is open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Create database schema if it doesn't exist
     * @return true if schema creation successful, false otherwise
     */
    bool createSchema();

private:
    LocalDBManager() = default;
    ~LocalDBManager() = default;
    LocalDBManager(const LocalDBManager&) = delete;
    LocalDBManager& operator=(const LocalDBManager&) = delete;

    QSqlDatabase m_database;
    bool m_initialized = false;
};

} // namespace database
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_DATABASE_LOCALDBMANAGER_H
