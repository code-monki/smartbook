#include "smartbook/common/metadata/MetadataExtractor.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QFileInfo>
#include <QCryptographicHash>
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
    // Use the same algorithm as SignatureVerifier::calculateContentHash
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "HashCalc");
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        qWarning() << "Failed to open cartridge for hash calculation";
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);

    // Hash tables in fixed order: Content_Pages, Content_Themes, Embedded_Apps, Form_Definitions, Metadata, Settings
    QStringList tables = {"Content_Pages", "Content_Themes", "Embedded_Apps", "Form_Definitions", "Metadata", "Settings"};

    for (const QString& tableName : tables) {
        QSqlQuery query(db);
        QString tableQuery = QString("SELECT * FROM %1 ORDER BY rowid").arg(tableName);
        
        if (!query.exec(tableQuery)) {
            // Table might not exist, hash empty
            hash.addData(QByteArray());
            continue;
        }

        // Hash each row
        while (query.next()) {
            QByteArray rowData;
            for (int i = 0; i < query.record().count(); ++i) {
                QVariant value = query.value(i);
                if (value.isNull()) {
                    rowData.append('\0');
                } else {
                    rowData.append(value.toString().toUtf8());
                }
            }
            hash.addData(rowData);
            hash.addData("\n");
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("HashCalc");

    return hash.result();
}

} // namespace metadata
} // namespace common
} // namespace smartbook
