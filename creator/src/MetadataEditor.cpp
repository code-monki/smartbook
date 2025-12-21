#include "smartbook/creator/MetadataEditor.h"
#include "smartbook/creator/ResourceManager.h"
#include "smartbook/common/database/CartridgeDBConnector.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QPixmap>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QBuffer>
#include <QDebug>

namespace smartbook {
namespace creator {

MetadataEditor::MetadataEditor(QWidget* parent)
    : QWidget(parent)
    , m_schemaVersion("1.0")
    , m_resourceManager(nullptr)
    , m_titleEdit(nullptr)
    , m_authorEdit(nullptr)
    , m_publisherEdit(nullptr)
    , m_versionEdit(nullptr)
    , m_publicationYearEdit(nullptr)
    , m_tagsEdit(nullptr)
    , m_coverImageLabel(nullptr)
    , m_importCoverButton(nullptr)
    , m_removeCoverButton(nullptr)
    , m_guidLabel(nullptr)
    , m_schemaVersionLabel(nullptr)
{
    m_resourceManager = new ResourceManager(this);
    setupUI();
}

MetadataEditor::~MetadataEditor() {
}

void MetadataEditor::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Form layout for metadata fields
    QFormLayout* formLayout = new QFormLayout();
    
    // Title
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Enter book title");
    formLayout->addRow("Title:", m_titleEdit);
    
    // Author
    m_authorEdit = new QLineEdit(this);
    m_authorEdit->setPlaceholderText("Enter author name(s)");
    formLayout->addRow("Author:", m_authorEdit);
    
    // Publisher (optional)
    m_publisherEdit = new QLineEdit(this);
    m_publisherEdit->setPlaceholderText("Enter publisher (optional)");
    formLayout->addRow("Publisher:", m_publisherEdit);
    
    // Version
    m_versionEdit = new QLineEdit(this);
    m_versionEdit->setPlaceholderText("e.g., 1.0");
    formLayout->addRow("Version:", m_versionEdit);
    
    // Publication Year
    m_publicationYearEdit = new QLineEdit(this);
    m_publicationYearEdit->setPlaceholderText("e.g., 2025");
    formLayout->addRow("Publication Year:", m_publicationYearEdit);
    
    // Tags
    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText("Comma-separated tags");
    formLayout->addRow("Tags:", m_tagsEdit);
    
    mainLayout->addLayout(formLayout);
    
    // Cover image section
    QHBoxLayout* coverLayout = new QHBoxLayout();
    m_coverImageLabel = new QLabel("No cover image", this);
    m_coverImageLabel->setMinimumHeight(150);
    m_coverImageLabel->setAlignment(Qt::AlignCenter);
    m_coverImageLabel->setStyleSheet("border: 1px solid #ddd; background-color: #f5f5f5;");
    
    QVBoxLayout* coverButtonsLayout = new QVBoxLayout();
    m_importCoverButton = new QPushButton("Import Cover Image", this);
    m_removeCoverButton = new QPushButton("Remove", this);
    m_removeCoverButton->setEnabled(false);
    
    coverButtonsLayout->addWidget(m_importCoverButton);
    coverButtonsLayout->addWidget(m_removeCoverButton);
    coverButtonsLayout->addStretch();
    
    coverLayout->addWidget(m_coverImageLabel, 2);
    coverLayout->addLayout(coverButtonsLayout, 1);
    
    mainLayout->addLayout(coverLayout);
    
    // GUID and Schema Version (read-only)
    QHBoxLayout* infoLayout = new QHBoxLayout();
    m_guidLabel = new QLabel("GUID: (not set)", this);
    m_schemaVersionLabel = new QLabel(QString("Schema Version: %1").arg(m_schemaVersion), this);
    infoLayout->addWidget(m_guidLabel);
    infoLayout->addWidget(m_schemaVersionLabel);
    infoLayout->addStretch();
    
    mainLayout->addLayout(infoLayout);
    mainLayout->addStretch();
    
    // Connect signals
    connect(m_titleEdit, &QLineEdit::textChanged, this, &MetadataEditor::onFieldChanged);
    connect(m_authorEdit, &QLineEdit::textChanged, this, &MetadataEditor::onFieldChanged);
    connect(m_publisherEdit, &QLineEdit::textChanged, this, &MetadataEditor::onFieldChanged);
    connect(m_versionEdit, &QLineEdit::textChanged, this, &MetadataEditor::onFieldChanged);
    connect(m_publicationYearEdit, &QLineEdit::textChanged, this, &MetadataEditor::onFieldChanged);
    connect(m_tagsEdit, &QLineEdit::textChanged, this, &MetadataEditor::onFieldChanged);
    connect(m_importCoverButton, &QPushButton::clicked, this, &MetadataEditor::onImportCoverImage);
    connect(m_removeCoverButton, &QPushButton::clicked, this, &MetadataEditor::onRemoveCoverImage);
}

bool MetadataEditor::loadMetadata(const QString& cartridgePath) {
    return loadMetadataFromDatabase(cartridgePath);
}

bool MetadataEditor::saveMetadata(const QString& cartridgePath) {
    // Open ResourceManager for this cartridge
    if (m_resourceManager) {
        m_resourceManager->openCartridge(cartridgePath);
    }
    
    // Update fields from UI
    m_title = m_titleEdit->text();
    m_author = m_authorEdit->text();
    m_publisher = m_publisherEdit->text();
    m_version = m_versionEdit->text();
    m_publicationYear = m_publicationYearEdit->text();
    m_tags = parseTags(m_tagsEdit->text());
    
    // If cover image path is a file path (not resource_id), import it first
    if (!m_coverImagePath.isEmpty() && m_coverImagePath.contains('/')) {
        // It's a file path, need to import it
        QFile file(m_coverImagePath);
        if (file.exists() && m_resourceManager) {
            QString resourceId = QString("cover_image_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
            QString importedId = m_resourceManager->importResource(m_coverImagePath, resourceId);
            if (!importedId.isEmpty()) {
                m_coverImagePath = importedId;
            } else {
                qWarning() << "Failed to import cover image as resource";
            }
        }
    }
    
    return saveMetadataToDatabase(cartridgePath);
}

void MetadataEditor::setCartridgeGuid(const QString& guid) {
    m_cartridgeGuid = guid;
    m_guidLabel->setText(QString("GUID: %1").arg(guid));
}

QString MetadataEditor::generateGuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void MetadataEditor::onFieldChanged() {
    emit metadataChanged();
}

void MetadataEditor::onImportCoverImage() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Cover Image",
        "",
        "Image Files (*.png *.jpg *.jpeg *.gif *.bmp)");
    
    if (!fileName.isEmpty()) {
        QPixmap pixmap(fileName);
        if (!pixmap.isNull()) {
            // Generate resource ID for cover image
            QString resourceId = QString("cover_image_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
            
            // Convert pixmap to byte array
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            QFileInfo fileInfo(fileName);
            QString format = fileInfo.suffix().toLower();
            if (format == "jpg") format = "jpeg";
            pixmap.save(&buffer, format.toUpper().toLocal8Bit().constData());
            
            // Import to Resources table if cartridge is open
            if (m_resourceManager && !m_resourceManager->getCartridgePath().isEmpty()) {
                QString importedId = m_resourceManager->importResource(fileName, resourceId);
                if (!importedId.isEmpty()) {
                    // Remove old cover image if exists
                    if (!m_coverImagePath.isEmpty() && m_coverImagePath != importedId) {
                        m_resourceManager->deleteResource(m_coverImagePath);
                    }
                    m_coverImagePath = importedId;
                } else {
                    // Fallback: try importing from data
                    QString mimeType = QString("image/%1").arg(format);
                    if (m_resourceManager->importResourceData(imageData, resourceId, "image", mimeType)) {
                        if (!m_coverImagePath.isEmpty() && m_coverImagePath != resourceId) {
                            m_resourceManager->deleteResource(m_coverImagePath);
                        }
                        m_coverImagePath = resourceId;
                    } else {
                        QMessageBox::warning(this, "Import Failed", "Failed to import cover image to cartridge.");
                        return;
                    }
                }
            } else {
                // Cartridge not open yet, just store the path temporarily
                // This will be imported when saveMetadata is called
                m_coverImagePath = fileName;  // Temporary: will be converted to resource_id on save
            }
            
            // Display preview
            QPixmap scaled = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_coverImageLabel->setPixmap(scaled);
            m_removeCoverButton->setEnabled(true);
            
            emit metadataChanged();
        } else {
            QMessageBox::warning(this, "Invalid Image", "Failed to load image file.");
        }
    }
}

void MetadataEditor::onRemoveCoverImage() {
    // Remove from Resources table if it's a resource_id
    if (m_resourceManager && !m_coverImagePath.isEmpty() && !m_coverImagePath.contains('/')) {
        // Likely a resource_id (not a file path)
        m_resourceManager->deleteResource(m_coverImagePath);
    }
    
    m_coverImagePath.clear();
    m_coverImageLabel->setText("No cover image");
    m_coverImageLabel->setPixmap(QPixmap());
    m_removeCoverButton->setEnabled(false);
    emit metadataChanged();
}

bool MetadataEditor::loadMetadataFromDatabase(const QString& cartridgePath) {
    // Open ResourceManager for this cartridge
    if (m_resourceManager) {
        m_resourceManager->openCartridge(cartridgePath);
    }
    
    common::database::CartridgeDBConnector connector;
    if (!connector.openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge for metadata load:" << cartridgePath;
        return false;
    }
    
    QSqlQuery query(connector.getDatabase());
    query.prepare("SELECT cartridge_guid, title, author, publisher, version, publication_year, tags_json, cover_image_path, schema_version FROM Metadata LIMIT 1");
    
    if (query.exec() && query.next()) {
        m_cartridgeGuid = query.value(0).toString();
        m_title = query.value(1).toString();
        m_author = query.value(2).toString();
        m_publisher = query.value(3).toString();
        m_version = query.value(4).toString();
        m_publicationYear = query.value(5).toString();
        QString tagsJson = query.value(6).toString();
        m_coverImagePath = query.value(7).toString();
        m_schemaVersion = query.value(8).toString();
        
        m_tags = parseTags(tagsJson);
        
        // Update UI
        m_titleEdit->setText(m_title);
        m_authorEdit->setText(m_author);
        m_publisherEdit->setText(m_publisher);
        m_versionEdit->setText(m_version);
        m_publicationYearEdit->setText(m_publicationYear);
        m_tagsEdit->setText(formatTags(m_tags));
        
        m_guidLabel->setText(QString("GUID: %1").arg(m_cartridgeGuid));
        m_schemaVersionLabel->setText(QString("Schema Version: %1").arg(m_schemaVersion));
        
        // Load cover image if available
        if (!m_coverImagePath.isEmpty()) {
            // Try to load from Resources table
            if (m_resourceManager) {
                ResourceInfo coverInfo = m_resourceManager->getResource(m_coverImagePath);
                if (coverInfo.isValid()) {
                    QPixmap pixmap;
                    if (pixmap.loadFromData(coverInfo.resourceData)) {
                        QPixmap scaled = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        m_coverImageLabel->setPixmap(scaled);
                        m_removeCoverButton->setEnabled(true);
                    } else {
                        m_coverImageLabel->setText(QString("Cover: %1 (load failed)").arg(m_coverImagePath));
                        m_removeCoverButton->setEnabled(true);
                    }
                } else {
                    // Fallback: might be a file path (legacy)
                    m_coverImageLabel->setText(QString("Cover: %1").arg(m_coverImagePath));
                    m_removeCoverButton->setEnabled(true);
                }
            } else {
                m_coverImageLabel->setText(QString("Cover: %1").arg(m_coverImagePath));
                m_removeCoverButton->setEnabled(true);
            }
        }
        
        connector.closeCartridge();
        return true;
    } else {
        qWarning() << "Failed to load metadata:" << query.lastError().text();
        connector.closeCartridge();
        return false;
    }
}

bool MetadataEditor::saveMetadataToDatabase(const QString& cartridgePath) {
    common::database::CartridgeDBConnector connector;
    if (!connector.openCartridge(cartridgePath)) {
        qWarning() << "Failed to open cartridge for metadata save:" << cartridgePath;
        return false;
    }
    
    // Check if metadata exists
    QSqlQuery checkQuery(connector.getDatabase());
    checkQuery.prepare("SELECT COUNT(*) FROM Metadata WHERE cartridge_guid = ?");
    checkQuery.addBindValue(m_cartridgeGuid);
    checkQuery.exec();
    checkQuery.next();
    bool exists = checkQuery.value(0).toInt() > 0;
    
    QString tagsJson = formatTags(m_tags);
    
    QSqlQuery query(connector.getDatabase());
    
    if (exists) {
        // Update existing metadata
        query.prepare(R"(
            UPDATE Metadata SET
                title = ?, author = ?, publisher = ?, version = ?,
                publication_year = ?, tags_json = ?, cover_image_path = ?, schema_version = ?
            WHERE cartridge_guid = ?
        )");
        query.addBindValue(m_title);
        query.addBindValue(m_author);
        query.addBindValue(m_publisher);
        query.addBindValue(m_version);
        query.addBindValue(m_publicationYear);
        query.addBindValue(tagsJson);
        query.addBindValue(m_coverImagePath);
        query.addBindValue(m_schemaVersion);
        query.addBindValue(m_cartridgeGuid);
    } else {
        // Insert new metadata
        if (m_cartridgeGuid.isEmpty()) {
            m_cartridgeGuid = generateGuid();
            m_guidLabel->setText(QString("GUID: %1").arg(m_cartridgeGuid));
        }
        
        query.prepare(R"(
            INSERT INTO Metadata (
                cartridge_guid, title, author, publisher, version,
                publication_year, tags_json, cover_image_path, schema_version
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");
        query.addBindValue(m_cartridgeGuid);
        query.addBindValue(m_title);
        query.addBindValue(m_author);
        query.addBindValue(m_publisher);
        query.addBindValue(m_version);
        query.addBindValue(m_publicationYear);
        query.addBindValue(tagsJson);
        query.addBindValue(m_coverImagePath);
        query.addBindValue(m_schemaVersion);
    }
    
    if (!query.exec()) {
        qWarning() << "Failed to save metadata:" << query.lastError().text();
        connector.closeCartridge();
        return false;
    }
    
    connector.closeCartridge();
    return true;
}

QStringList MetadataEditor::parseTags(const QString& tagsJson) const {
    if (tagsJson.isEmpty()) {
        return QStringList();
    }
    
    // Try parsing as JSON array first
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(tagsJson.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && doc.isArray()) {
        QJsonArray array = doc.array();
        QStringList tags;
        for (const QJsonValue& value : array) {
            tags.append(value.toString());
        }
        return tags;
    }
    
    // Fallback: parse as comma-separated string
    QStringList tags = tagsJson.split(',', Qt::SkipEmptyParts);
    for (QString& tag : tags) {
        tag = tag.trimmed();
    }
    return tags;
}

QString MetadataEditor::formatTags(const QStringList& tags) const {
    if (tags.isEmpty()) {
        return QString();
    }
    
    QJsonArray array;
    for (const QString& tag : tags) {
        array.append(tag);
    }
    QJsonDocument doc(array);
    return doc.toJson(QJsonDocument::Compact);
}

} // namespace creator
} // namespace smartbook
