#include "smartbook/common/metadata/MetadataExtractor.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFileInfo>
#include <QDebug>

namespace smartbook {
namespace common {
namespace metadata {

CartridgeMetadata MetadataExtractor::extractMetadata(const QString& cartridgePath) {
    CartridgeMetadata metadata;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "MetadataExtract");
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        qWarning() << "Failed to open cartridge for metadata extraction:" << cartridgePath;
        return metadata;
    }

    QSqlQuery query(db);

    // Extract from Metadata table
    if (query.exec("SELECT cartridge_guid, title, author, publisher, version, publication_year, series_name, edition_name, series_order, schema_version FROM Metadata LIMIT 1")) {
        if (query.next()) {
            metadata.cartridgeGuid = query.value(0).toString();
            metadata.title = query.value(1).toString();
            metadata.author = query.value(2).toString();
            metadata.publisher = query.value(3).toString();
            metadata.version = query.value(4).toString();
            metadata.publicationYear = query.value(5).toString();
            metadata.seriesName = query.value(6).toString();
            metadata.editionName = query.value(7).toString();
            metadata.seriesOrder = query.value(8).toInt();
            metadata.schemaVersion = query.value(9).toString();
        }
    }

    // Extract cover image
    QString coverImagePath;
    if (query.exec("SELECT cover_image_path FROM Metadata LIMIT 1")) {
        if (query.next()) {
            coverImagePath = query.value(0).toString();
        }
    }

    // Load cover image if path is relative
    if (!coverImagePath.isEmpty()) {
        QFileInfo fileInfo(cartridgePath);
        QString fullCoverPath = fileInfo.absoluteDir().absoluteFilePath(coverImagePath);
        QFile coverFile(fullCoverPath);
        if (coverFile.exists() && coverFile.open(QIODevice::ReadOnly)) {
            metadata.coverImageData = coverFile.readAll();
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("MetadataExtract");

    return metadata;
}

QByteArray MetadataExtractor::calculateContentHash(const QString& cartridgePath) {
    // This would use the same algorithm as SignatureVerifier
    // For now, return empty - would need to implement full hash calculation
    Q_UNUSED(cartridgePath);
    return QByteArray();
}

} // namespace metadata
} // namespace common
} // namespace smartbook
