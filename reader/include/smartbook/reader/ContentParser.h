#ifndef SMARTBOOK_READER_CONTENTPARSER_H
#define SMARTBOOK_READER_CONTENTPARSER_H

#include <QString>
#include <QStringList>
#include <QList>

namespace smartbook {
namespace reader {

/**
 * @brief Parser for HTML content to detect and extract QML app markers
 * 
 * Parses HTML content to find QML embedded application markers
 * (e.g., `<div data-smartbook-qml-app="app_id"></div>`) and provides
 * utilities to clean HTML and extract app information.
 */
class ContentParser {
public:
    /**
     * @brief Structure representing a QML app marker found in HTML
     */
    struct QmlAppMarker {
        QString appId;      // Application ID from marker
        int position;       // Character position in original HTML
        int length;         // Length of marker element in HTML
    };

    /**
     * @brief Parse HTML content and extract QML app markers
     * @param html HTML content to parse
     * @return List of QML app markers found in the content
     */
    QList<QmlAppMarker> parseContent(const QString& html) const;
    
    /**
     * @brief Clean HTML by removing QML app markers
     * @param html Original HTML content
     * @return HTML content with QML app markers removed
     */
    QString cleanHtml(const QString& html) const;
    
    /**
     * @brief Extract app IDs from HTML content
     * @param html HTML content to parse
     * @return List of app IDs found in QML app markers (preserves order)
     */
    QStringList extractAppIds(const QString& html) const;

private:
    /**
     * @brief Find QML app marker in HTML string
     * @param html HTML content
     * @param startPos Starting position to search from
     * @return QmlAppMarker if found, or invalid marker if not found
     */
    QmlAppMarker findNextMarker(const QString& html, int startPos = 0) const;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_CONTENTPARSER_H

