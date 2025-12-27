#include "smartbook/common/metadata/MetadataExtractor.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QUuid>
#include <QDebug>

namespace smartbook {
namespace common {
namespace metadata {

CartridgeMetadata MetadataExtractor::extractMetadata(const QString& cartridgePath) {
    CartridgeMetadata metadata;

    // Use unique connection name to avoid conflicts
    QString connectionName = QString("MetadataExtract_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        qWarning() << "Failed to open cartridge for metadata extraction:" << cartridgePath;
        return metadata;
    }

    QSqlQuery query(db);

    // Extract from Metadata table
    // Use only required columns that exist in all cartridges
    if (query.exec("SELECT cartridge_guid, title, author, publisher, version, publication_year, schema_version FROM Metadata LIMIT 1")) {
        if (query.next()) {
            metadata.cartridgeGuid = query.value(0).toString();
            metadata.title = query.value(1).toString();
            metadata.author = query.value(2).toString();
            metadata.publisher = query.value(3).toString();
            metadata.version = query.value(4).toString();
            metadata.publicationYear = query.value(5).toString();
            metadata.schemaVersion = query.value(6).toString();
            
            // Try to get optional columns if they exist
            QSqlQuery optionalQuery(db);
            if (optionalQuery.exec("SELECT series_name, edition_name, series_order FROM Metadata LIMIT 1")) {
                if (optionalQuery.next()) {
                    metadata.seriesName = optionalQuery.value(0).toString();
                    metadata.editionName = optionalQuery.value(1).toString();
                    metadata.seriesOrder = optionalQuery.value(2).toInt();
                }
            }
        } else {
            qWarning() << "MetadataExtractor: Metadata table is empty or query returned no rows";
        }
    } else {
        qWarning() << "MetadataExtractor: Failed to query Metadata table:" << query.lastError().text();
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
    QSqlDatabase::removeDatabase(connectionName);

    return metadata;
}

QByteArray MetadataExtractor::calculateContentHash(const QString& cartridgePath) {
    // Use the same algorithm as SignatureVerifier::calculateContentHash
    // Use unique connection name to avoid conflicts
    QString connectionName = QString("HashCalc_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
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
    QSqlDatabase::removeDatabase(connectionName);

    return hash.result();
}

} // namespace metadata
} // namespace common
} // namespace smartbook
