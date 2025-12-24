#include "smartbook/creator/CartridgeExporter.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QHash>
#include <QVariant>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QSslCertificate>
#include <QSslKey>
#include <QMetaType>
#include <QDebug>

// OpenSSL for cryptographic signing
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <cstring>

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

    // Package metadata from source (same path) to target
    // Note: In a real workflow, source might be a working file and target is the export
    // For now, we assume they're the same (exporting the current cartridge)
    if (!packageMetadata(cartridgePath, cartridgePath)) {
        qWarning() << "Failed to package metadata, but continuing export";
        // Don't fail export if metadata packaging fails - might be a new cartridge
    }

    // Package resources from source (same path) to target
    if (!packageResources(cartridgePath, cartridgePath)) {
        qWarning() << "Failed to package resources, but continuing export";
        // Don't fail export if resource packaging fails - might be a new cartridge
    }

    // Package content pages from source (same path) to target
    if (!packageContentPages(cartridgePath, cartridgePath)) {
        qWarning() << "Failed to package content pages, but continuing export";
        // Don't fail export if content packaging fails - might be a new cartridge
    }
    
    emit exportProgress(100);
    emit exportComplete(true, QString());
    return true;
}

bool CartridgeExporter::signCartridge(const QString& cartridgePath, const QString& certificatePath,
                                     const QString& privateKeyPath, int securityLevel) {
    qDebug() << "Signing cartridge:" << cartridgePath << "Level:" << securityLevel;
    
    // Level 3: No signature required
    if (securityLevel == 3) {
        qDebug() << "Level 3 cartridge - no signing required";
        return true;
    }
    
    // Calculate content hash (H1)
    QByteArray contentHash = calculateContentHash(cartridgePath);
    if (contentHash.isEmpty()) {
        qCritical() << "Failed to calculate content hash";
        return false;
    }
    
    // Open cartridge database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "CartridgeSign");
    db.setDatabaseName(cartridgePath);
    
    if (!db.open()) {
        qCritical() << "Failed to open cartridge for signing:" << db.lastError().text();
        return false;
    }
    
    QByteArray certificateData;
    QByteArray digitalSignature;
    QString publicKeyFingerprint;
    
    // For Level 1 and Level 2, we need certificate and private key
    if (securityLevel == 1 || securityLevel == 2) {
        if (certificatePath.isEmpty() || privateKeyPath.isEmpty()) {
            qCritical() << "Certificate and private key paths required for Level" << securityLevel;
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
        
        // Load certificate
        QFile certFile(certificatePath);
        if (!certFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open certificate file:" << certificatePath;
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
        certificateData = certFile.readAll();
        certFile.close();
        
        // Parse certificate to extract public key and calculate fingerprint
        QSslCertificate cert(certificateData, QSsl::Der);
        if (cert.isNull()) {
            // Try PEM format
            cert = QSslCertificate(certificateData, QSsl::Pem);
        }
        
        if (cert.isNull()) {
            qCritical() << "Failed to parse certificate";
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
        
        // Extract public key and calculate fingerprint
        QSslKey publicKey = cert.publicKey();
        if (publicKey.isNull()) {
            qCritical() << "Failed to extract public key from certificate";
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
        
        // Calculate public key fingerprint (SHA-256 of public key DER)
        QByteArray publicKeyDer = publicKey.toDer();
        QCryptographicHash fingerprintHash(QCryptographicHash::Sha256);
        fingerprintHash.addData(publicKeyDer);
        publicKeyFingerprint = fingerprintHash.result().toHex().toUpper();
        
        // Load private key
        QFile keyFile(privateKeyPath);
        if (!keyFile.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open private key file:" << privateKeyPath;
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
        QByteArray keyData = keyFile.readAll();
        keyFile.close();
        
        // Parse private key
        QSslKey privateKey(keyData, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        if (privateKey.isNull()) {
            // Try DER format
            privateKey = QSslKey(keyData, QSsl::Rsa, QSsl::Der, QSsl::PrivateKey);
        }
        
        if (privateKey.isNull()) {
            qCritical() << "Failed to parse private key";
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
        
        // Sign the content hash using OpenSSL EVP API
        // Note: This requires OpenSSL to be linked
        // For now, we'll use a simplified approach that can be enhanced
        digitalSignature = signHashWithPrivateKey(contentHash, privateKey);
        
        if (digitalSignature.isEmpty()) {
            qCritical() << "Failed to create digital signature";
            db.close();
            QSqlDatabase::removeDatabase("CartridgeSign");
            return false;
        }
    }
    
    // Store security data in Cartridge_Security table
    QSqlQuery query(db);
    
    // Check if security data already exists
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT COUNT(*) FROM Cartridge_Security");
    checkQuery.exec();
    checkQuery.next();
    bool exists = checkQuery.value(0).toInt() > 0;
    
    if (exists) {
        // Update existing security data
        query.prepare(R"(
            UPDATE Cartridge_Security SET
                digest_type = ?, hash_digest = ?, digital_signature = ?,
                public_key_fingerprint = ?, certificate_data = ?
        )");
        query.addBindValue("SHA-256");
        query.addBindValue(contentHash);
        query.addBindValue(digitalSignature);
        query.addBindValue(publicKeyFingerprint);
        query.addBindValue(certificateData.isEmpty() ? QVariant() : certificateData);
    } else {
        // Insert new security data
        query.prepare(R"(
            INSERT INTO Cartridge_Security (
                digest_type, hash_digest, digital_signature,
                public_key_fingerprint, certificate_data
            ) VALUES (?, ?, ?, ?, ?)
        )");
        query.addBindValue("SHA-256");
        query.addBindValue(contentHash);
        query.addBindValue(digitalSignature);
        query.addBindValue(publicKeyFingerprint);
        query.addBindValue(certificateData.isEmpty() ? QVariant() : certificateData);
    }
    
    if (!query.exec()) {
        qCritical() << "Failed to store security data:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("CartridgeSign");
        return false;
    }
    
    qDebug() << "Cartridge signed successfully. Level:" << securityLevel;
    
    db.close();
    QSqlDatabase::removeDatabase("CartridgeSign");
    
    return true;
}

QByteArray CartridgeExporter::signHashWithPrivateKey(const QByteArray& hash, const QSslKey& privateKey) {
    // Digital signature creation using OpenSSL EVP API
    // Signs the provided hash using the private key with RSA PKCS#1 padding and SHA-256
    
    if (privateKey.isNull() || hash.isEmpty()) {
        qWarning() << "Invalid private key or hash for signing";
        return QByteArray();
    }
    
    // Verify key type
    if (privateKey.algorithm() != QSsl::Rsa) {
        qWarning() << "Only RSA keys are supported for signing";
        return QByteArray();
    }
    
    // Verify hash size (SHA-256 produces 32 bytes)
    if (hash.size() != 32) {
        qWarning() << "Hash size must be 32 bytes (SHA-256), got:" << hash.size();
        return QByteArray();
    }
    
    // Get private key data from QSslKey
    // QSslKey doesn't directly expose EVP_PKEY, so we need to parse the key data
    // Try PEM format first (most common)
    QByteArray keyData = privateKey.toPem();
    bool isPem = !keyData.isEmpty();
    
    if (!isPem) {
        // Try DER format
        keyData = privateKey.toDer();
        if (keyData.isEmpty()) {
            qWarning() << "Failed to extract key data from QSslKey";
            return QByteArray();
        }
    }
    
    // Parse private key using OpenSSL BIO
    BIO* bio = BIO_new_mem_buf(keyData.constData(), keyData.size());
    if (!bio) {
        qWarning() << "Failed to create BIO for key parsing";
        return QByteArray();
    }
    
    EVP_PKEY* evpKey = nullptr;
    
    // Try PEM format first
    if (isPem) {
        evpKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    } else {
        // Try DER format
        evpKey = d2i_PrivateKey_bio(bio, nullptr);
    }
    
    BIO_free(bio);
    
    if (!evpKey) {
        qWarning() << "Failed to parse private key:" << ERR_error_string(ERR_get_error(), nullptr);
        return QByteArray();
    }
    
    // Create signing context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(evpKey, nullptr);
    if (!ctx) {
        qWarning() << "Failed to create EVP_PKEY_CTX";
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Initialize signing
    if (EVP_PKEY_sign_init(ctx) <= 0) {
        qWarning() << "Failed to initialize signing:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Set RSA padding to PKCS#1
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        qWarning() << "Failed to set RSA padding:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Set hash algorithm to SHA-256
    if (EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) <= 0) {
        qWarning() << "Failed to set signature hash algorithm:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Determine signature length
    size_t siglen = 0;
    if (EVP_PKEY_sign(ctx, nullptr, &siglen, reinterpret_cast<const unsigned char*>(hash.constData()), hash.size()) <= 0) {
        qWarning() << "Failed to determine signature length:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Allocate signature buffer
    unsigned char* sig = static_cast<unsigned char*>(OPENSSL_malloc(siglen));
    if (!sig) {
        qWarning() << "Failed to allocate signature buffer";
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Perform signing
    if (EVP_PKEY_sign(ctx, sig, &siglen, reinterpret_cast<const unsigned char*>(hash.constData()), hash.size()) <= 0) {
        qWarning() << "Failed to sign hash:" << ERR_error_string(ERR_get_error(), nullptr);
        OPENSSL_free(sig);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(evpKey);
        return QByteArray();
    }
    
    // Copy signature to QByteArray
    QByteArray signature(reinterpret_cast<const char*>(sig), static_cast<int>(siglen));
    
    // Cleanup
    OPENSSL_free(sig);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(evpKey);
    
    qDebug() << "Successfully signed hash, signature size:" << signature.size() << "bytes";
    
    return signature;
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
            content_type TEXT NOT NULL DEFAULT 'book',
            isbn TEXT,
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

    // Create Form_Definitions table
    QString formDefinitionsTable = R"(
        CREATE TABLE IF NOT EXISTS Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_schema_json TEXT NOT NULL,
            form_version INTEGER NOT NULL DEFAULT 1,
            migration_rules_json TEXT
        )
    )";

    if (!query.exec(formDefinitionsTable)) {
        qCritical() << "Failed to create Form_Definitions table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create User_Data table
    QString userDataTable = R"(
        CREATE TABLE IF NOT EXISTS User_Data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            form_key TEXT NOT NULL,
            form_version INTEGER,
            migrated_from_version INTEGER,
            timestamp INTEGER NOT NULL,
            serialized_data TEXT NOT NULL
        )
    )";

    if (!query.exec(userDataTable)) {
        qCritical() << "Failed to create User_Data table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Settings table
    QString settingsTable = R"(
        CREATE TABLE IF NOT EXISTS Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT NOT NULL,
            setting_type TEXT NOT NULL,
            description TEXT
        )
    )";

    if (!query.exec(settingsTable)) {
        qCritical() << "Failed to create Settings table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Embedded_Apps table
    QString embeddedAppsTable = R"(
        CREATE TABLE IF NOT EXISTS Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT NOT NULL,
            manifest_json TEXT NOT NULL,
            entry_html TEXT NOT NULL,
            js_code BLOB,
            css_code BLOB,
            additional_resources BLOB
        )
    )";

    if (!query.exec(embeddedAppsTable)) {
        qCritical() << "Failed to create Embedded_Apps table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Resources table
    QString resourcesTable = R"(
        CREATE TABLE IF NOT EXISTS Resources (
            resource_id TEXT PRIMARY KEY,
            resource_path TEXT NOT NULL,
            resource_type TEXT NOT NULL,
            resource_data BLOB NOT NULL,
            mime_type TEXT NOT NULL
        )
    )";

    if (!query.exec(resourcesTable)) {
        qCritical() << "Failed to create Resources table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Content_Themes table
    QString contentThemesTable = R"(
        CREATE TABLE IF NOT EXISTS Content_Themes (
            theme_id TEXT PRIMARY KEY,
            theme_name TEXT NOT NULL,
            is_builtin INTEGER NOT NULL DEFAULT 0,
            theme_config_json TEXT NOT NULL,
            is_active INTEGER DEFAULT 0
        )
    )";

    if (!query.exec(contentThemesTable)) {
        qCritical() << "Failed to create Content_Themes table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Cartridge_Security table
    QString cartridgeSecurityTable = R"(
        CREATE TABLE IF NOT EXISTS Cartridge_Security (
            digest_type TEXT NOT NULL,
            hash_digest BLOB NOT NULL,
            digital_signature BLOB NOT NULL,
            public_key_fingerprint TEXT NOT NULL,
            certificate_data BLOB
        )
    )";

    if (!query.exec(cartridgeSecurityTable)) {
        qCritical() << "Failed to create Cartridge_Security table:" << query.lastError().text();
        db.close();
        return false;
    }

    // Create Navigation_Structure table
    QString navigationStructureTable = R"(
        CREATE TABLE IF NOT EXISTS Navigation_Structure (
            nav_id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_name TEXT NOT NULL,
            group_order INTEGER NOT NULL,
            item_label TEXT NOT NULL,
            item_order INTEGER NOT NULL,
            target_type TEXT NOT NULL,
            target_value TEXT NOT NULL,
            parent_item_id INTEGER,
            metadata_json TEXT
        )
    )";

    if (!query.exec(navigationStructureTable)) {
        qCritical() << "Failed to create Navigation_Structure table:" << query.lastError().text();
        db.close();
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("CartridgeCreate");

    return true;
}

bool CartridgeExporter::packageContentPages(const QString& sourceCartridgePath, const QString& targetCartridgePath) {
    // If source and target are the same, content is already in place
    if (sourceCartridgePath == targetCartridgePath) {
        qDebug() << "Source and target are the same, content already in place";
        return true;
    }
    
    // Open source cartridge
    common::database::CartridgeDBConnector sourceConnector;
    if (!sourceConnector.openCartridge(sourceCartridgePath)) {
        qWarning() << "Failed to open source cartridge for content packaging:" << sourceCartridgePath;
        return false;
    }
    
    // Open target cartridge
    QSqlDatabase targetDb = QSqlDatabase::addDatabase("QSQLITE", "CartridgeTarget");
    targetDb.setDatabaseName(targetCartridgePath);
    
    if (!targetDb.open()) {
        qCritical() << "Failed to open target cartridge for content packaging:" << targetDb.lastError().text();
        sourceConnector.closeCartridge();
        return false;
    }
    
    // Read pages from source
    QSqlQuery sourceQuery(sourceConnector.getDatabase());
    sourceQuery.prepare("SELECT page_order, chapter_title, html_content, associated_css "
                        "FROM Content_Pages "
                        "ORDER BY page_order");
    
    if (!sourceQuery.exec()) {
        qWarning() << "Failed to read content pages from source:" << sourceQuery.lastError().text();
        targetDb.close();
        QSqlDatabase::removeDatabase("CartridgeTarget");
        sourceConnector.closeCartridge();
        return false;
    }
    
    // Insert pages into target
    QSqlQuery targetQuery(targetDb);
    targetQuery.prepare("INSERT INTO Content_Pages (page_order, chapter_title, html_content, associated_css) "
                        "VALUES (?, ?, ?, ?)");
    
    int pageCount = 0;
    bool success = true;
    
    while (sourceQuery.next()) {
        int pageOrder = sourceQuery.value(0).toInt();
        QString chapterTitle = sourceQuery.value(1).toString();
        QString htmlContent = sourceQuery.value(2).toString();
        QString associatedCss = sourceQuery.value(3).toString();
        
        targetQuery.addBindValue(pageOrder);
        targetQuery.addBindValue(chapterTitle.isEmpty() ? QVariant() : chapterTitle);
        targetQuery.addBindValue(htmlContent);
        targetQuery.addBindValue(associatedCss.isEmpty() ? QVariant() : associatedCss);
        
        if (!targetQuery.exec()) {
            qWarning() << "Failed to insert content page:" << targetQuery.lastError().text();
            success = false;
            break;
        }
        
        pageCount++;
    }
    
    if (success) {
        qDebug() << "Packaged" << pageCount << "content pages";
    }
    
    targetDb.close();
    QSqlDatabase::removeDatabase("CartridgeTarget");
    sourceConnector.closeCartridge();
    
    return success;
}

bool CartridgeExporter::packageMetadata(const QString& sourceCartridgePath, const QString& targetCartridgePath) {
    // If source and target are the same, metadata is already in place
    if (sourceCartridgePath == targetCartridgePath) {
        qDebug() << "Source and target are the same, metadata already in place";
        return true;
    }
    
    // Open source cartridge
    common::database::CartridgeDBConnector sourceConnector;
    if (!sourceConnector.openCartridge(sourceCartridgePath)) {
        qWarning() << "Failed to open source cartridge for metadata packaging:" << sourceCartridgePath;
        return false;
    }
    
    // Open target cartridge
    QSqlDatabase targetDb = QSqlDatabase::addDatabase("QSQLITE", "CartridgeTarget");
    targetDb.setDatabaseName(targetCartridgePath);
    
    if (!targetDb.open()) {
        qCritical() << "Failed to open target cartridge for metadata packaging:" << targetDb.lastError().text();
        sourceConnector.closeCartridge();
        return false;
    }
    
    // Read metadata from source
    QSqlQuery sourceQuery(sourceConnector.getDatabase());
    sourceQuery.prepare(R"(
        SELECT cartridge_guid, title, author, publisher, version, publication_year,
               tags_json, cover_image_path, schema_version, content_type, isbn,
               series_name, edition_name, series_order
        FROM Metadata
        LIMIT 1
    )");
    
    if (!sourceQuery.exec() || !sourceQuery.next()) {
        qWarning() << "Failed to read metadata from source:" << sourceQuery.lastError().text();
        targetDb.close();
        QSqlDatabase::removeDatabase("CartridgeTarget");
        sourceConnector.closeCartridge();
        return false;
    }
    
    // Extract all metadata fields
    QString cartridgeGuid = sourceQuery.value(0).toString();
    QString title = sourceQuery.value(1).toString();
    QString author = sourceQuery.value(2).toString();
    QString publisher = sourceQuery.value(3).toString();
    QString version = sourceQuery.value(4).toString();
    QString publicationYear = sourceQuery.value(5).toString();
    QString tagsJson = sourceQuery.value(6).toString();
    QString coverImagePath = sourceQuery.value(7).toString();
    QString schemaVersion = sourceQuery.value(8).toString();
    QString contentType = sourceQuery.value(9).toString();
    QString isbn = sourceQuery.value(10).toString();
    QString seriesName = sourceQuery.value(11).toString();
    QString editionName = sourceQuery.value(12).toString();
    QVariant seriesOrder = sourceQuery.value(13);
    
    // Check if metadata already exists in target
    QSqlQuery checkQuery(targetDb);
    checkQuery.prepare("SELECT COUNT(*) FROM Metadata WHERE cartridge_guid = ?");
    checkQuery.addBindValue(cartridgeGuid);
    checkQuery.exec();
    checkQuery.next();
    bool exists = checkQuery.value(0).toInt() > 0;
    
    // Insert or update metadata in target
    QSqlQuery targetQuery(targetDb);
    
    if (exists) {
        // Update existing metadata
        targetQuery.prepare(R"(
            UPDATE Metadata SET
                title = ?, author = ?, publisher = ?, version = ?,
                publication_year = ?, tags_json = ?, cover_image_path = ?,
                schema_version = ?, content_type = ?, isbn = ?,
                series_name = ?, edition_name = ?, series_order = ?
            WHERE cartridge_guid = ?
        )");
        targetQuery.addBindValue(title);
        targetQuery.addBindValue(author);
        targetQuery.addBindValue(publisher);
        targetQuery.addBindValue(version);
        targetQuery.addBindValue(publicationYear);
        targetQuery.addBindValue(tagsJson);
        targetQuery.addBindValue(coverImagePath);
        targetQuery.addBindValue(schemaVersion);
        targetQuery.addBindValue(contentType.isEmpty() ? "book" : contentType); // Default to "book" if empty
        targetQuery.addBindValue(isbn.isEmpty() ? QVariant() : isbn);
        targetQuery.addBindValue(seriesName.isEmpty() ? QVariant() : seriesName);
        targetQuery.addBindValue(editionName.isEmpty() ? QVariant() : editionName);
        targetQuery.addBindValue(seriesOrder.isNull() ? QVariant() : seriesOrder);
        targetQuery.addBindValue(cartridgeGuid);
    } else {
        // Insert new metadata
        targetQuery.prepare(R"(
            INSERT INTO Metadata (
                cartridge_guid, title, author, publisher, version,
                publication_year, tags_json, cover_image_path, schema_version,
                content_type, isbn, series_name, edition_name, series_order
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");
        targetQuery.addBindValue(cartridgeGuid);
        targetQuery.addBindValue(title);
        targetQuery.addBindValue(author);
        targetQuery.addBindValue(publisher);
        targetQuery.addBindValue(version);
        targetQuery.addBindValue(publicationYear);
        targetQuery.addBindValue(tagsJson);
        targetQuery.addBindValue(coverImagePath);
        targetQuery.addBindValue(schemaVersion);
        targetQuery.addBindValue(contentType.isEmpty() ? "book" : contentType); // Default to "book" if empty
        targetQuery.addBindValue(isbn.isEmpty() ? QVariant() : isbn);
        targetQuery.addBindValue(seriesName.isEmpty() ? QVariant() : seriesName);
        targetQuery.addBindValue(editionName.isEmpty() ? QVariant() : editionName);
        targetQuery.addBindValue(seriesOrder.isNull() ? QVariant() : seriesOrder);
    }
    
    if (!targetQuery.exec()) {
        qWarning() << "Failed to insert/update metadata:" << targetQuery.lastError().text();
        targetDb.close();
        QSqlDatabase::removeDatabase("CartridgeTarget");
        sourceConnector.closeCartridge();
        return false;
    }
    
    qDebug() << "Packaged metadata successfully";
    
    targetDb.close();
    QSqlDatabase::removeDatabase("CartridgeTarget");
    sourceConnector.closeCartridge();
    
    return true;
}

bool CartridgeExporter::packageResources(const QString& sourceCartridgePath, const QString& targetCartridgePath) {
    // If source and target are the same, resources are already in place
    if (sourceCartridgePath == targetCartridgePath) {
        qDebug() << "Source and target are the same, resources already in place";
        return true;
    }
    
    // Open source cartridge
    common::database::CartridgeDBConnector sourceConnector;
    if (!sourceConnector.openCartridge(sourceCartridgePath)) {
        qWarning() << "Failed to open source cartridge for resource packaging:" << sourceCartridgePath;
        return false;
    }
    
    // Open target cartridge
    QSqlDatabase targetDb = QSqlDatabase::addDatabase("QSQLITE", "CartridgeTarget");
    targetDb.setDatabaseName(targetCartridgePath);
    
    if (!targetDb.open()) {
        qCritical() << "Failed to open target cartridge for resource packaging:" << targetDb.lastError().text();
        sourceConnector.closeCartridge();
        return false;
    }
    
    // Read resources from source
    QSqlQuery sourceQuery(sourceConnector.getDatabase());
    sourceQuery.prepare("SELECT resource_id, resource_path, resource_type, resource_data, mime_type "
                        "FROM Resources "
                        "ORDER BY resource_id");
    
    if (!sourceQuery.exec()) {
        qWarning() << "Failed to read resources from source:" << sourceQuery.lastError().text();
        targetDb.close();
        QSqlDatabase::removeDatabase("CartridgeTarget");
        sourceConnector.closeCartridge();
        return false;
    }
    
    // Insert resources into target
    QSqlQuery targetQuery(targetDb);
    targetQuery.prepare(R"(
        INSERT OR REPLACE INTO Resources (resource_id, resource_path, resource_type, resource_data, mime_type)
        VALUES (?, ?, ?, ?, ?)
    )");
    
    int resourceCount = 0;
    bool success = true;
    
    while (sourceQuery.next()) {
        QString resourceId = sourceQuery.value(0).toString();
        QString resourcePath = sourceQuery.value(1).toString();
        QString resourceType = sourceQuery.value(2).toString();
        QByteArray resourceData = sourceQuery.value(3).toByteArray();
        QString mimeType = sourceQuery.value(4).toString();
        
        targetQuery.addBindValue(resourceId);
        targetQuery.addBindValue(resourcePath);
        targetQuery.addBindValue(resourceType);
        targetQuery.addBindValue(resourceData);
        targetQuery.addBindValue(mimeType);
        
        if (!targetQuery.exec()) {
            qWarning() << "Failed to insert resource:" << targetQuery.lastError().text();
            success = false;
            break;
        }
        
        resourceCount++;
    }
    
    if (success) {
        qDebug() << "Packaged" << resourceCount << "resources";
    }
    
    targetDb.close();
    QSqlDatabase::removeDatabase("CartridgeTarget");
    sourceConnector.closeCartridge();
    
    return success;
}

QByteArray CartridgeExporter::calculateContentHash(const QString& cartridgePath) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "HashCalc");
    db.setDatabaseName(cartridgePath);
    
    if (!db.open()) {
        qWarning() << "Failed to open cartridge for hash calculation:" << cartridgePath;
        return QByteArray();
    }
    
    // Table order as specified in DDD
    QStringList tables = {"Content_Pages", "Content_Themes", "Embedded_Apps", 
                          "Form_Definitions", "Metadata", "Settings"};
    
    QList<QByteArray> tableHashes;
    
    for (const QString& tableName : tables) {
        // Calculate table name prefix (first 4 bytes of SHA-256 of table name)
        QCryptographicHash tableNameHash(QCryptographicHash::Sha256);
        tableNameHash.addData(tableName.toUtf8());
        QByteArray tableNameHashResult = tableNameHash.result();
        QByteArray tablePrefix = tableNameHashResult.left(4);
        
        // Query rows ordered by primary key
        QSqlQuery query(db);
        QString orderBy;
        
        if (tableName == "Content_Pages") {
            orderBy = "ORDER BY page_order ASC";
        } else if (tableName == "Content_Themes") {
            orderBy = "ORDER BY theme_id ASC";
        } else if (tableName == "Embedded_Apps") {
            orderBy = "ORDER BY app_id ASC";
        } else if (tableName == "Form_Definitions") {
            orderBy = "ORDER BY form_id ASC";
        } else if (tableName == "Metadata") {
            orderBy = ""; // Single row, no ordering needed
        } else if (tableName == "Settings") {
            orderBy = "ORDER BY setting_key ASC";
        }
        
        QString queryStr = QString("SELECT * FROM %1 %2").arg(tableName, orderBy);
        
        if (!query.exec(queryStr)) {
            // Table might not exist, hash empty table
            QCryptographicHash emptyHash(QCryptographicHash::Sha256);
            emptyHash.addData(QByteArray());
            QByteArray emptyTableHash = emptyHash.result();
            tableHashes.append(tablePrefix + emptyTableHash);
            continue;
        }
        
        // Serialize all rows
        QByteArray rowData;
        QSqlRecord record = query.record();
        
        // Get column names and sort alphabetically
        QStringList columnNames;
        for (int i = 0; i < record.count(); ++i) {
            QString colName = record.fieldName(i);
            // For Metadata table, only include specified fields
            if (tableName == "Metadata") {
                QStringList allowedFields = {"title", "author", "version", "publication_year", 
                                             "tags_json", "cover_image_path", "schema_version"};
                if (allowedFields.contains(colName)) {
                    columnNames.append(colName);
                }
            } else {
                columnNames.append(colName);
            }
        }
        columnNames.sort();
        
        while (query.next()) {
            // Serialize row in alphabetical column order
            for (const QString& colName : columnNames) {
                QVariant value = query.value(colName);
                
                if (value.isNull()) {
                    rowData.append('\0'); // NULL marker
                } else {
                    QMetaType type = value.metaType();
                    if (type.id() == QMetaType::QString || type.id() == QMetaType::QByteArray) {
                        // TEXT or BLOB: UTF-8 for text, raw bytes for BLOB
                        if (type.id() == QMetaType::QString) {
                            rowData.append(value.toString().toUtf8());
                        } else {
                            rowData.append(value.toByteArray());
                        }
                    } else if (type.id() == QMetaType::Int || type.id() == QMetaType::LongLong) {
                        // INTEGER: 8-byte big-endian
                        qint64 intValue = value.toLongLong();
                        QByteArray intBytes(8, 0);
                        for (int i = 7; i >= 0; --i) {
                            intBytes[i] = static_cast<char>(intValue & 0xFF);
                            intValue >>= 8;
                        }
                        rowData.append(intBytes);
                    } else {
                        // Other types: convert to string and UTF-8 encode
                        rowData.append(value.toString().toUtf8());
                    }
                }
            }
            rowData.append('\n'); // Row delimiter
        }
        
        // Hash concatenated row data
        QCryptographicHash tableHash(QCryptographicHash::Sha256);
        tableHash.addData(rowData);
        QByteArray tableHashResult = tableHash.result();
        
        // Store prefix + hash
        tableHashes.append(tablePrefix + tableHashResult);
    }
    
    // Concatenate all table hashes
    QByteArray concatenatedHashes;
    for (const QByteArray& tableHash : tableHashes) {
        concatenatedHashes.append(tableHash);
    }
    
    // Calculate final hash
    QCryptographicHash finalHash(QCryptographicHash::Sha256);
    finalHash.addData(concatenatedHashes);
    
    db.close();
    QSqlDatabase::removeDatabase("HashCalc");
    
    return finalHash.result();
}

} // namespace creator
} // namespace smartbook
