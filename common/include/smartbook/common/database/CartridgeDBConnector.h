#ifndef SMARTBOOK_COMMON_DATABASE_CARTRIDGEDBCONNECTOR_H
#define SMARTBOOK_COMMON_DATABASE_CARTRIDGEDBCONNECTOR_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QByteArray>
#include <QObject>

namespace smartbook {
namespace common {
namespace database {

/**
 * @brief Per-instance connector for cartridge database files
 * 
 * Manages the connection and transactional persistence for one specific
 * cartridge's content and User_Data. Each Reader View Instance has its
 * own CartridgeDBConnector to ensure data isolation.
 */
class CartridgeDBConnector : public QObject {
    Q_OBJECT

public:
    explicit CartridgeDBConnector(QObject* parent = nullptr);
    ~CartridgeDBConnector();

    /**
     * @brief Open a cartridge database file
     * @param cartridgePath Path to the .sqlite cartridge file
     * @return true if opened successfully, false otherwise
     */
    bool openCartridge(const QString& cartridgePath);

    /**
     * @brief Close the cartridge database connection
     */
    void closeConnection();

    /**
     * @brief Check if cartridge is open
     * @return true if open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Get the cartridge GUID
     * @return Cartridge GUID or empty string if not loaded
     */
    QString getCartridgeGuid() const;

    /**
     * @brief Execute a query on the cartridge database
     * @param queryString SQL query string
     * @return QSqlQuery object with results
     */
    QSqlQuery executeQuery(const QString& queryString);

    /**
     * @brief Begin a transaction
     * @return true if transaction started, false otherwise
     */
    bool beginTransaction();

    /**
     * @brief Commit the current transaction
     * @return true if committed, false otherwise
     */
    bool commitTransaction();

    /**
     * @brief Rollback the current transaction
     * @return true if rolled back, false otherwise
     */
    bool rollbackTransaction();

    /**
     * @brief Get the database connection
     * @return Reference to the QSqlDatabase
     */
    QSqlDatabase& getDatabase();

private:
    void configureConnection();

    QSqlDatabase m_database;
    QString m_cartridgeGuid;
    QString m_cartridgePath;
    bool m_isOpen = false;
};

} // namespace database
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_DATABASE_CARTRIDGEDBCONNECTOR_H
