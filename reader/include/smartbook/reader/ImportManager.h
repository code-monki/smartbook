#ifndef SMARTBOOK_READER_IMPORTMANAGER_H
#define SMARTBOOK_READER_IMPORTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include "smartbook/reader/ui/ImportDialog.h"

namespace smartbook {
namespace common {
namespace manifest {
    class ManifestManager;
}
namespace metadata {
    class MetadataExtractor;
}
namespace database {
    class CartridgeDBConnector;
}
}

namespace reader {
namespace ui {
    class ImportDialog;
}

/**
 * @brief Manages cartridge import operations
 * 
 * Handles validation, file copying, metadata extraction, and manifest creation
 * for importing cartridges into the library.
 */
class ImportManager : public QObject {
    Q_OBJECT

public:
    enum ImportResult {
        Success,
        ValidationFailed,
        DuplicateSkipped,
        DuplicateReplaced,
        DuplicateKeepBoth,
        CopyFailed,
        ManifestFailed,
        Cancelled
    };

    struct ImportResultInfo {
        ImportResult result;
        QString errorMessage;
        QString cartridgeGuid;
        QString cartridgeTitle;
    };

    explicit ImportManager(QObject* parent = nullptr);
    ~ImportManager();

    /**
     * @brief Import a single cartridge file
     * @param sourcePath Path to source cartridge file
     * @param libraryPath Library directory path
     * @param duplicateAction Action to take if duplicate found
     * @return ImportResultInfo with result details
     */
    ImportResultInfo importCartridge(
        const QString& sourcePath,
        const QString& libraryPath,
        ui::ImportDialog::DuplicateAction duplicateAction = ui::ImportDialog::Skip
    );

    /**
     * @brief Import multiple cartridge files
     * @param sourcePaths List of source cartridge file paths
     * @param libraryPath Library directory path
     * @param progressDialog Progress dialog for status updates
     * @return List of ImportResultInfo for each file
     */
    QList<ImportResultInfo> importCartridges(
        const QStringList& sourcePaths,
        const QString& libraryPath,
        ui::ImportDialog* progressDialog
    );

    /**
     * @brief Validate a cartridge file
     * @param filePath Path to cartridge file
     * @return true if valid, false otherwise
     */
    bool validateCartridge(const QString& filePath);

    /**
     * @brief Get library directory path (default or user-configured)
     * @return Library directory path
     */
    QString getLibraryPath() const;

    /**
     * @brief Set library directory path
     * @param path Library directory path
     */
    void setLibraryPath(const QString& path);

signals:
    void importProgress(const QString& filePath, const QString& status, int progress);
    void importComplete(const QString& filePath, ImportResult result, const QString& errorMessage);

private:
    QString generateUniqueFilename(const QString& basePath, const QString& desiredFilename);
    QString copyFileToLibrary(const QString& sourcePath, const QString& libraryPath, const QString& desiredFilename = QString());
    bool createManifestEntry(const QString& cartridgePath, const QString& libraryPath);

    QString m_libraryPath;
    common::manifest::ManifestManager* m_manifestManager;
    common::metadata::MetadataExtractor* m_metadataExtractor;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_IMPORTMANAGER_H

