#ifndef SMARTBOOK_COMMON_METADATA_METADATAEXTRACTOR_H
#define SMARTBOOK_COMMON_METADATA_METADATAEXTRACTOR_H

#include <QString>
#include <QByteArray>
#include <QHash>

namespace smartbook {
namespace common {
namespace metadata {

/**
 * @brief Extracted metadata from a cartridge
 */
struct CartridgeMetadata {
    QString cartridgeGuid;
    QString title;
    QString author;
    QString publisher;
    QString version;
    QString publicationYear;
    QString seriesName;
    QString editionName;
    int seriesOrder = 0;
    QByteArray coverImageData;
    QString schemaVersion;
};

/**
 * @brief Extracts metadata from cartridge files
 */
class MetadataExtractor {
public:
    /**
     * @brief Extract metadata from a cartridge file
     * @param cartridgePath Path to the cartridge file
     * @return CartridgeMetadata structure with extracted data
     */
    static CartridgeMetadata extractMetadata(const QString& cartridgePath);

    /**
     * @brief Calculate content hash (H2) for a cartridge
     * @param cartridgePath Path to the cartridge file
     * @return H2 hash as QByteArray
     */
    static QByteArray calculateContentHash(const QString& cartridgePath);
};

} // namespace metadata
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_METADATA_METADATAEXTRACTOR_H
