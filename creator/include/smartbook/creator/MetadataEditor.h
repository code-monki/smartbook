#ifndef SMARTBOOK_CREATOR_METADATAEDITOR_H
#define SMARTBOOK_CREATOR_METADATAEDITOR_H

#include <QWidget>
#include <QString>
#include <QStringList>

namespace smartbook {
namespace creator {
    class ResourceManager;

/**
 * @brief Metadata editor widget
 * 
 * Provides interface for managing document-level metadata.
 * Implements FR-CT-3.20, FR-CT-3.21, FR-CT-3.22, FR-CT-3.23
 */
class MetadataEditor : public QWidget {
    Q_OBJECT

public:
    explicit MetadataEditor(QWidget* parent = nullptr);
    ~MetadataEditor();

    /**
     * @brief Load metadata from cartridge
     * @param cartridgePath Path to cartridge SQLite file
     * @return true if loaded successfully
     */
    bool loadMetadata(const QString& cartridgePath);

    /**
     * @brief Save metadata to cartridge
     * @param cartridgePath Path to cartridge SQLite file
     * @return true if saved successfully
     */
    bool saveMetadata(const QString& cartridgePath);

    /**
     * @brief Get cartridge GUID
     * @return Cartridge GUID (UUID v4)
     */
    QString getCartridgeGuid() const { return m_cartridgeGuid; }

    /**
     * @brief Set cartridge GUID (only for new cartridges)
     * @param guid UUID v4 string
     */
    void setCartridgeGuid(const QString& guid);

    /**
     * @brief Generate new UUID v4 GUID
     * @return New GUID
     */
    static QString generateGuid();

signals:
    void metadataChanged();

private slots:
    void onFieldChanged();
    void onImportCoverImage();
    void onRemoveCoverImage();

private:
    void setupUI();
    bool loadMetadataFromDatabase(const QString& cartridgePath);
    bool saveMetadataToDatabase(const QString& cartridgePath);
    QStringList parseTags(const QString& tagsJson) const;
    QString formatTags(const QStringList& tags) const;
    
    QString m_cartridgeGuid;
    QString m_title;
    QString m_author;
    QString m_publisher;
    QString m_version;
    QString m_publicationYear;
    QStringList m_tags;
    QString m_coverImagePath;  // Stores resource_id for cover image
    QString m_schemaVersion;
    ResourceManager* m_resourceManager;
    
    class QLineEdit* m_titleEdit;
    class QLineEdit* m_authorEdit;
    class QLineEdit* m_publisherEdit;
    class QLineEdit* m_versionEdit;
    class QLineEdit* m_publicationYearEdit;
    class QLineEdit* m_tagsEdit;
    class QLabel* m_coverImageLabel;
    class QPushButton* m_importCoverButton;
    class QPushButton* m_removeCoverButton;
    class QLabel* m_guidLabel;
    class QLabel* m_schemaVersionLabel;
};

} // namespace creator
} // namespace smartbook

#endif // SMARTBOOK_CREATOR_METADATAEDITOR_H
