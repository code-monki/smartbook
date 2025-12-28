#ifndef SMARTBOOK_READER_CONTENTPARSER_H
#define SMARTBOOK_READER_CONTENTPARSER_H

#include <QString>
#include <QStringList>
#include <QList>

namespace smartbook {
namespace reader {

/**
 * @brief Parser for HTML content to detect and extract embedded component markers
 * 
 * ContentParser analyzes HTML content to find markers for embedded components:
 * - QML embedded applications: `<div data-smartbook-qml-app="app_id"></div>`
 * - Form widgets: `<div data-smartbook-form="form_id"></div>`
 * 
 * The parser extracts marker information (app/form IDs, positions) and provides
 * utilities to clean HTML by removing markers before rendering.
 * 
 * @section Marker Format
 * 
 * **QML App Markers:**
 * @code
 * <div data-smartbook-qml-app="my_app_id"></div>
 * @endcode
 * 
 * **Form Markers:**
 * @code
 * <div data-smartbook-form="contact_form"></div>
 * @endcode
 * 
 * Markers can be self-closing or have closing tags. The parser handles both formats.
 * 
 * @section Usage
 * 
 * @code
 * ContentParser parser;
 * QList<ContentParser::QmlAppMarker> markers = parser.parseContent(html);
 * QString cleanHtml = parser.cleanHtml(html); // HTML with markers removed
 * @endcode
 * 
 * @note This class is stateless and all methods are const. Multiple instances
 * can be used concurrently without issues.
 * 
 * @see ReaderView
 * @see QmlEmbeddedAppWidget
 * @see FormEmbeddedWidget
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
     * @brief Structure representing a form marker found in HTML
     */
    struct FormMarker {
        QString formId;     // Form ID from marker
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
    
    /**
     * @brief Parse HTML content and extract form markers
     * @param html HTML content to parse
     * @return List of form markers found in the content
     */
    QList<FormMarker> parseFormMarkers(const QString& html) const;
    
    /**
     * @brief Extract form IDs from HTML content
     * @param html HTML content to parse
     * @return List of form IDs found in form markers (preserves order)
     */
    QStringList extractFormIds(const QString& html) const;

private:
    /**
     * @brief Find QML app marker in HTML string
     * @param html HTML content
     * @param startPos Starting position to search from
     * @return QmlAppMarker if found, or invalid marker if not found
     */
    QmlAppMarker findNextMarker(const QString& html, int startPos = 0) const;
    
    /**
     * @brief Find form marker in HTML string
     * @param html HTML content
     * @param startPos Starting position to search from
     * @return FormMarker if found, or invalid marker if not found
     */
    FormMarker findNextFormMarker(const QString& html, int startPos = 0) const;
};

} // namespace reader
} // namespace smartbook

#endif // SMARTBOOK_READER_CONTENTPARSER_H

