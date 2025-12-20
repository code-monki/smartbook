#include "smartbook/creator/CartridgeExporter.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QHash>
#include <QVariant>
#include <QDebug>

namespace smartbook {
namespace creator {

CartridgeExporter::CartridgeExporter(QObject* parent)
    : QObject(parent)
{
}

bool CartridgeExporter::exportCartridge(const QString& cartridgePath, const QHash<QString, QVariant>& /* metadata */) {
    qDebug() << "Exporting cartridge to:" << cartridgePath;
    
    // Create cartridge database
    if (!createCartridgeSchema(cartridgePath)) {
        emit exportComplete(false, "Failed to create cartridge schema");
        return false;
    }

    // TODO: Add content pages, forms, embedded apps, etc.
    
    emit exportProgress(100);
    emit exportComplete(true, QString());
    return true;
}

bool CartridgeExporter::signCartridge(const QString& cartridgePath, const QString& certificatePath,
                                     const QString& privateKeyPath, int securityLevel) {
    Q_UNUSED(certificatePath);
    Q_UNUSED(privateKeyPath);
    Q_UNUSED(securityLevel);
    
    qDebug() << "Signing cartridge:" << cartridgePath;
    // TODO: Implement cartridge signing
    return true;
}

bool CartridgeExporter::createCartridgeSchema(const QString& cartridgePath) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "CartridgeCreate");
    db.setDatabaseName(cartridgePath);

    if (!db.open()) {
        qCritical() << "Failed to create cartridge database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);

    // Create Metadata table
    QString metadataTable = R"(
        CREATE TABLE IF NOT EXISTS Metadata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            cartridge_guid TEXT NOT NULL UNIQUE,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publisher TEXT,
            version TEXT NOT NULL,
            publication_year TEXT NOT NULL,
            tags_json TEXT,
            cover_image_path TEXT,
            schema_version TEXT NOT NULL,
            series_name TEXT,
            edition_name TEXT,
            series_order INTEGER
        )
    )";

    if (!query.exec(metadataTable)) {
        qCritical() << "Failed to create Metadata table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Content_Pages table
    QString contentPagesTable = R"(
        CREATE TABLE IF NOT EXISTS Content_Pages (
            page_id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_order INTEGER NOT NULL UNIQUE,
            chapter_title TEXT,
            html_content TEXT NOT NULL,
            associated_css TEXT
        )
    )";

    if (!query.exec(contentPagesTable)) {
        qCritical() << "Failed to create Content_Pages table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create other tables (Form_Definitions, User_Data, Settings, Embedded_Apps, etc.)
    // TODO: Add remaining table creation

    db.close();
    QSqlDatabase::removeDatabase("CartridgeCreate");

    return true;
}

QByteArray CartridgeExporter::calculateContentHash(const QString& cartridgePath) {
    // TODO: Implement content hash calculation (same as SignatureVerifier)
    Q_UNUSED(cartridgePath);
    return QByteArray();
}

} // namespace creator
} // namespace smartbook
