#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <QString>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

/**
 * @brief Helper functions for creating test cartridges
 * 
 * These helpers reduce code duplication across test files
 * and ensure consistent test cartridge creation.
 */
namespace TestHelpers {

/**
 * @brief Create a minimal test cartridge with required tables
 * @param path Path where cartridge should be created
 * @param guid Cartridge GUID
 * @param title Cartridge title
 * @param connectionName Unique connection name for QSqlDatabase
 * @return true if created successfully, false otherwise
 */
bool createMinimalCartridge(const QString& path, const QString& guid, 
                            const QString& title, const QString& connectionName);

/**
 * @brief Create an L1 (CA-signed) test cartridge
 * @param path Path where cartridge should be created
 * @param guid Cartridge GUID
 * @param connectionName Unique connection name for QSqlDatabase
 * @return Path to created cartridge, or empty string on failure
 */
QString createL1Cartridge(const QString& path, const QString& guid, const QString& connectionName);

/**
 * @brief Create an L2 (self-signed) test cartridge
 * @param path Path where cartridge should be created
 * @param guid Cartridge GUID
 * @param connectionName Unique connection name for QSqlDatabase
 * @return Path to created cartridge, or empty string on failure
 */
QString createL2Cartridge(const QString& path, const QString& guid, const QString& connectionName);

/**
 * @brief Create an L3 (unsigned) test cartridge
 * @param path Path where cartridge should be created
 * @param guid Cartridge GUID
 * @param connectionName Unique connection name for QSqlDatabase
 * @return Path to created cartridge, or empty string on failure
 */
QString createL3Cartridge(const QString& path, const QString& guid, const QString& connectionName);

/**
 * @brief Update H1 hash in a cartridge to match calculated H2
 * @param cartridgePath Path to cartridge file
 * @param h2Hash The H2 hash to set as H1
 * @param connectionName Unique connection name for QSqlDatabase
 * @return true if updated successfully, false otherwise
 */
bool updateCartridgeH1Hash(const QString& cartridgePath, const QByteArray& h2Hash, const QString& connectionName);

} // namespace TestHelpers

#endif // TEST_HELPERS_H
