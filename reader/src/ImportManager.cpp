#include "smartbook/reader/ImportManager.h"
#include "smartbook/reader/ui/ImportDialog.h"
#include "smartbook/common/manifest/ManifestManager.h"
#include "smartbook/common/metadata/MetadataExtractor.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include "smartbook/common/database/LocalDBManager.h"
#include "smartbook/common/utils/PathUtils.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QDebug>
#include <QUuid>
#include <QRegularExpression>

namespace smartbook {
namespace reader {

ImportManager::ImportManager(QObject* parent)
    : QObject(parent)
    , m_manifestManager(new common::manifest::ManifestManager(this))
{
    // Set default library path
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_libraryPath = QDir(documentsPath).filePath("SmartBook/Library");
    QDir().mkpath(m_libraryPath);
}

ImportManager::~ImportManager() {
}

QString ImportManager::getLibraryPath() const {
    return m_libraryPath;
}

void ImportManager::setLibraryPath(const QString& path) {
    m_libraryPath = path;
    QDir().mkpath(m_libraryPath);
}

bool ImportManager::validateCartridge(const QString& filePath) {
    // Step 1: File format validation
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        qWarning() << "ImportManager::validateCartridge: File does not exist or is not readable:" << filePath;
        return false;
    }

    // Check if it's a SQLite database
    common::database::CartridgeDBConnector connector(this);
    if (!connector.openCartridge(filePath)) {
        QString validationError = connector.getValidationError();
        if (!validationError.isEmpty()) {
            qWarning() << "ImportManager::validateCartridge: Validation failed:" << validationError;
        } else {
            qWarning() << "ImportManager::validateCartridge: Failed to open cartridge:" << filePath;
        }
        return false;
    }

    // Validation is done by CartridgeDBConnector::validateDatabase()
    // which checks:
    // - Required tables exist
    // - Required columns exist
    // - cartridge_guid is valid UUID v4
    // - schema_version is "1.0" for Phase 1

    connector.closeCartridge();
    return true;
}

ImportManager::ImportResultInfo ImportManager::importCartridge(
    const QString& sourcePath,
    const QString& libraryPath,
    ui::ImportDialog::DuplicateAction duplicateAction)
{
    ImportResultInfo result;
    result.result = ValidationFailed;

    // Step 1: Validate cartridge
    if (!validateCartridge(sourcePath)) {
        result.errorMessage = "Cartridge validation failed";
        return result;
    }

    // Step 2: Extract metadata to get cartridge GUID
    common::metadata::CartridgeMetadata metadata = 
        common::metadata::MetadataExtractor::extractMetadata(sourcePath);
    
    if (metadata.cartridgeGuid.isEmpty()) {
        result.errorMessage = "Failed to extract cartridge GUID";
        return result;
    }

    result.cartridgeGuid = metadata.cartridgeGuid;
    result.cartridgeTitle = metadata.title;

    // Step 3: Check for duplicate
    if (m_manifestManager->manifestEntryExists(metadata.cartridgeGuid)) {
        if (duplicateAction == ui::ImportDialog::Skip) {
            result.result = DuplicateSkipped;
            result.errorMessage = "Duplicate cartridge skipped";
            return result;
        } else if (duplicateAction == ui::ImportDialog::Replace) {
            // Delete existing file and manifest entry
            common::manifest::ManifestManager::ManifestEntry existingEntry = 
                m_manifestManager->getManifestEntry(metadata.cartridgeGuid);
            
            if (!existingEntry.localPath.isEmpty() && QFile::exists(existingEntry.localPath)) {
                QFile::remove(existingEntry.localPath);
            }
            
            m_manifestManager->deleteManifestEntry(metadata.cartridgeGuid);
            // Continue with import
        } else if (duplicateAction == ui::ImportDialog::KeepBoth) {
            // Generate unique filename - will be handled in copyFileToLibrary
        }
    }

    // Step 4: Copy file to library
    QFileInfo sourceInfo(sourcePath);
    QString desiredFilename = sourceInfo.fileName();
    QString destPath = copyFileToLibrary(sourcePath, libraryPath, desiredFilename);
    
    if (destPath.isEmpty()) {
        result.result = CopyFailed;
        result.errorMessage = "Failed to copy file to library directory";
        return result;
    }

    // Step 5: Create or update manifest entry
    // For KeepBoth, we update the existing entry with the new file path
    if (duplicateAction == ui::ImportDialog::KeepBoth && 
        m_manifestManager->manifestEntryExists(metadata.cartridgeGuid)) {
        // Update existing manifest entry with new file path
        common::manifest::ManifestManager::ManifestEntry existingEntry = 
            m_manifestManager->getManifestEntry(metadata.cartridgeGuid);
        existingEntry.localPath = destPath;
        // Update hash and other metadata
        QByteArray contentHash = common::metadata::MetadataExtractor::calculateContentHash(destPath);
        existingEntry.cartridgeHash = contentHash;
        existingEntry.title = metadata.title;
        existingEntry.author = metadata.author;
        existingEntry.publisher = metadata.publisher;
        existingEntry.version = metadata.version;
        existingEntry.publicationYear = metadata.publicationYear;
        existingEntry.coverImageData = metadata.coverImageData;
        
        if (!m_manifestManager->updateManifestEntry(existingEntry)) {
            QFile::remove(destPath);
            result.result = ManifestFailed;
            result.errorMessage = "Failed to update manifest entry";
            return result;
        }
    } else {
        // Create new manifest entry
        if (!createManifestEntry(destPath, libraryPath)) {
            // If manifest creation fails, remove the copied file
            QFile::remove(destPath);
            result.result = ManifestFailed;
            result.errorMessage = "Failed to create manifest entry";
            return result;
        }
    }

    if (duplicateAction == ui::ImportDialog::Replace) {
        result.result = DuplicateReplaced;
    } else if (duplicateAction == ui::ImportDialog::KeepBoth) {
        result.result = DuplicateKeepBoth;
    } else {
        result.result = Success;
    }

    return result;
}

QList<ImportManager::ImportResultInfo> ImportManager::importCartridges(
    const QStringList& sourcePaths,
    const QString& libraryPath,
    ui::ImportDialog* progressDialog)
{
    QList<ImportResultInfo> results;
    
    if (sourcePaths.isEmpty()) {
        return results;
    }

    for (const QString& sourcePath : sourcePaths) {
        if (progressDialog && progressDialog->wasCancelled()) {
            ImportResultInfo cancelled;
            cancelled.result = Cancelled;
            cancelled.errorMessage = "Import cancelled by user";
            results.append(cancelled);
            break;
        }

        // Update progress
        if (progressDialog) {
            progressDialog->updateFileProgress(sourcePath, "Validating...", 0);
        }
        emit importProgress(sourcePath, "Validating...", 0);

        // Extract metadata to check for duplicates
        common::metadata::CartridgeMetadata metadata = 
            common::metadata::MetadataExtractor::extractMetadata(sourcePath);
        
        ui::ImportDialog::DuplicateAction duplicateAction = ui::ImportDialog::Skip;
        
        if (!metadata.cartridgeGuid.isEmpty() && 
            m_manifestManager->manifestEntryExists(metadata.cartridgeGuid)) {
            // Show duplicate dialog
            if (progressDialog) {
                duplicateAction = progressDialog->showDuplicateDialog(
                    metadata.title, metadata.cartridgeGuid);
            }
            
            if (duplicateAction == ui::ImportDialog::Skip) {
                ImportResultInfo skipped;
                skipped.result = DuplicateSkipped;
                skipped.cartridgeGuid = metadata.cartridgeGuid;
                skipped.cartridgeTitle = metadata.title;
                skipped.errorMessage = "Duplicate cartridge skipped";
                results.append(skipped);
                
                if (progressDialog) {
                    progressDialog->updateFileProgress(sourcePath, "Skipped (duplicate)", 100);
                }
                continue;
            }
        }

        // Import the cartridge
        if (progressDialog) {
            progressDialog->updateFileProgress(sourcePath, "Importing...", 50);
        }
        emit importProgress(sourcePath, "Importing...", 50);

        ImportResultInfo result = importCartridge(sourcePath, libraryPath, duplicateAction);
        results.append(result);

        if (progressDialog) {
            QString status = (result.result == Success || 
                             result.result == DuplicateReplaced || 
                             result.result == DuplicateKeepBoth) 
                ? "Completed" : result.errorMessage;
            progressDialog->updateFileProgress(sourcePath, status, 100);
        }
        emit importProgress(sourcePath, 
                           result.result == Success ? "Completed" : result.errorMessage, 
                           100);
        emit importComplete(sourcePath, result.result, result.errorMessage);
    }

    // Show completion summary
    if (progressDialog) {
        int successCount = 0;
        int errorCount = 0;
        int skippedCount = 0;
        
        for (const ImportResultInfo& result : results) {
            if (result.result == Success || result.result == DuplicateReplaced || 
                result.result == DuplicateKeepBoth) {
                successCount++;
            } else if (result.result == DuplicateSkipped) {
                skippedCount++;
            } else {
                errorCount++;
            }
        }
        
        progressDialog->showCompletionSummary(successCount, errorCount, skippedCount);
    }

    return results;
}

QString ImportManager::generateUniqueFilename(const QString& basePath, const QString& desiredFilename) {
    QFileInfo fileInfo(desiredFilename);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();
    QString path = QDir(basePath).filePath(desiredFilename);

    if (!QFile::exists(path)) {
        return desiredFilename;
    }

    // Generate unique filename by appending _1, _2, etc.
    int counter = 1;
    QString uniqueFilename;
    do {
        uniqueFilename = QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix);
        path = QDir(basePath).filePath(uniqueFilename);
        counter++;
    } while (QFile::exists(path) && counter < 1000);

    return uniqueFilename;
}

QString ImportManager::copyFileToLibrary(
    const QString& sourcePath,
    const QString& libraryPath,
    const QString& desiredFilename)
{
    QFileInfo sourceInfo(sourcePath);
    QString filename = desiredFilename.isEmpty() ? sourceInfo.fileName() : desiredFilename;
    
    // Check if file already exists and generate unique name if needed
    QString destPath = QDir(libraryPath).filePath(filename);
    if (QFile::exists(destPath)) {
        filename = generateUniqueFilename(libraryPath, filename);
        destPath = QDir(libraryPath).filePath(filename);
    }

    // Copy file
    if (!QFile::copy(sourcePath, destPath)) {
        qCritical() << "Failed to copy file from" << sourcePath << "to" << destPath;
        return QString();
    }

    return destPath;
}

bool ImportManager::createManifestEntry(const QString& cartridgePath, const QString& /* libraryPath */) {
    // Extract metadata
    common::metadata::CartridgeMetadata metadata = 
        common::metadata::MetadataExtractor::extractMetadata(cartridgePath);
    
    if (metadata.cartridgeGuid.isEmpty()) {
        qWarning() << "Cannot create manifest entry: empty cartridge GUID";
        return false;
    }

    // Calculate content hash (H2)
    QByteArray contentHash = common::metadata::MetadataExtractor::calculateContentHash(cartridgePath);
    
    // Create manifest entry
    common::manifest::ManifestManager::ManifestEntry entry;
    entry.cartridgeGuid = metadata.cartridgeGuid;
    entry.cartridgeHash = contentHash;
    entry.localPath = cartridgePath;
    entry.title = metadata.title;
    entry.author = metadata.author;
    entry.publisher = metadata.publisher;
    entry.version = metadata.version;
    entry.publicationYear = metadata.publicationYear;
    entry.coverImageData = metadata.coverImageData;

    return m_manifestManager->createManifestEntry(entry);
}

} // namespace reader
} // namespace smartbook

