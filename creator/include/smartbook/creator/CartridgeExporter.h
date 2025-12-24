#ifndef SMARTBOOK_CREATOR_CARTRIDGEEXPORTER_H
#define SMARTBOOK_CREATOR_CARTRIDGEEXPORTER_H

#include <QString>
#include <QObject>
#include <QSslKey>

namespace smartbook {
namespace creator {

/**
 * @brief Cartridge exporter
 * 
 * Handles cartridge creation, signing, and export.
 */
class CartridgeExporter : public QObject {
    Q_OBJECT

public:
    explicit CartridgeExporter(QObject* parent = nullptr);

    /**
     * @brief Export cartridge to file
     * @param cartridgePath Path to save cartridge
     * @param metadata Cartridge metadata
     * @return true if export successful, false otherwise
     */
    bool exportCartridge(const QString& cartridgePath, const QHash<QString, QVariant>& metadata);

    /**
     * @brief Sign cartridge with certificate
     * @param cartridgePath Path to cartridge file
     * @param certificatePath Path to certificate file
     * @param privateKeyPath Path to private key file
     * @param securityLevel Security level (1, 2, or 3)
     * @return true if signing successful, false otherwise
     */
    bool signCartridge(const QString& cartridgePath, const QString& certificatePath,
                      const QString& privateKeyPath, int securityLevel);

    /**
     * @brief Package content pages from source cartridge to target cartridge
     * @param sourceCartridgePath Path to source cartridge (where content is)
     * @param targetCartridgePath Path to target cartridge (where to copy)
     * @return true if packaging successful, false otherwise
     */
    bool packageContentPages(const QString& sourceCartridgePath, const QString& targetCartridgePath);

    /**
     * @brief Package metadata from source cartridge to target cartridge
     * @param sourceCartridgePath Path to source cartridge (where metadata is)
     * @param targetCartridgePath Path to target cartridge (where to copy)
     * @return true if packaging successful, false otherwise
     */
    bool packageMetadata(const QString& sourceCartridgePath, const QString& targetCartridgePath);

    /**
     * @brief Package resources from source cartridge to target cartridge
     * @param sourceCartridgePath Path to source cartridge (where resources are)
     * @param targetCartridgePath Path to target cartridge (where to copy)
     * @return true if packaging successful, false otherwise
     */
    bool packageResources(const QString& sourceCartridgePath, const QString& targetCartridgePath);

signals:
    void exportProgress(int percentage);
    void exportComplete(bool success, const QString& errorMessage);

private:
    bool createCartridgeSchema(const QString& cartridgePath);
    QByteArray calculateContentHash(const QString& cartridgePath);
    QByteArray signHashWithPrivateKey(const QByteArray& hash, const QSslKey& privateKey);
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_CARTRIDGEEXPORTER_H
