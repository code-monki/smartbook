/**
 * @file ContentParser.cpp
 * @brief Implementation of ContentParser for detecting embedded component markers
 * 
 * This file implements the ContentParser class, which analyzes HTML content to find
 * markers for embedded components (QML apps and forms) and provides utilities to
 * clean HTML by removing these markers before rendering.
 * 
 * @section Marker Detection
 * 
 * **QML App Markers:**
 * - Pattern: `<div data-smartbook-qml-app="app_id"></div>`
 * - Also supports self-closing: `<div data-smartbook-qml-app="app_id" />`
 * - Uses QRegularExpression with case-insensitive matching
 * 
 * **Form Markers:**
 * - Pattern: `<div data-smartbook-form="form_id"></div>`
 * - Also supports self-closing: `<div data-smartbook-form="form_id" />`
 * - Uses QRegularExpression with case-insensitive matching
 * 
 * @section Implementation Details
 * 
 * **Regex Patterns:**
 * - Uses `QRegularExpression::DotMatchesEverythingOption` to match across newlines
 * - Uses `QRegularExpression::CaseInsensitiveOption` for case-insensitive matching
 * - Captures app/form ID from attribute value
 * - Handles both opening/closing tags and self-closing tags
 * 
 * **Performance:**
 * - All methods are const (stateless)
 * - Regex patterns are compiled on each call (acceptable for typical content sizes)
 * - For large content, consider caching compiled regex patterns
 * 
 * @see ContentParser.h
 * @see ReaderView
 */

#include "smartbook/reader/ContentParser.h"
#include <QRegularExpression>
#include <QDebug>

namespace smartbook {
namespace reader {

// ============================================================================
// Public Methods - QML App Marker Parsing
// ============================================================================

QList<ContentParser::QmlAppMarker> ContentParser::parseContent(const QString& html) const
{
    QList<QmlAppMarker> markers;
    int startPos = 0;
    
    while (startPos < html.length()) {
        QmlAppMarker marker = findNextMarker(html, startPos);
        if (marker.appId.isEmpty()) {
            // No more markers found
            break;
        }
        
        markers.append(marker);
        startPos = marker.position + marker.length;
    }
    
    return markers;
}

QString ContentParser::cleanHtml(const QString& html) const
{
    /**
     * @brief Remove all embedded component markers from HTML
     * 
     * Removes both QML app markers and form markers from HTML content
     * before rendering. This ensures markers don't appear in the rendered
     * content (they are replaced with actual widgets).
     * 
     * @param html Original HTML content with markers
     * @return HTML content with markers removed
     */
    
    QString cleaned = html;
    
    // Remove QML app markers
    // Pattern: <div data-smartbook-qml-app="app_id"[^>]*>.*?</div>
    QString markerPattern = QStringLiteral("<div\\s+data-smartbook-qml-app=\"[^\"]*\"[^>]*>.*?</div>");
    QRegularExpression markerRegex(
        markerPattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );
    
    // Also match self-closing tags: <div data-smartbook-qml-app="app_id" />
    QString selfClosingPattern = QStringLiteral("<div\\s+data-smartbook-qml-app=\"[^\"]*\"[^>]*\\s*/>");
    QRegularExpression selfClosingRegex(
        selfClosingPattern,
        QRegularExpression::CaseInsensitiveOption
    );
    
    // Remove all markers
    cleaned.remove(markerRegex);
    cleaned.remove(selfClosingRegex);
    
    // Also remove form markers
    QString formMarkerPattern = QStringLiteral("<div\\s+data-smartbook-form=\"[^\"]*\"[^>]*>.*?</div>");
    QRegularExpression formMarkerRegex(
        formMarkerPattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );
    
    QString formSelfClosingPattern = QStringLiteral("<div\\s+data-smartbook-form=\"[^\"]*\"[^>]*\\s*/>");
    QRegularExpression formSelfClosingRegex(
        formSelfClosingPattern,
        QRegularExpression::CaseInsensitiveOption
    );
    
    cleaned.remove(formMarkerRegex);
    cleaned.remove(formSelfClosingRegex);
    
    return cleaned;
}

QStringList ContentParser::extractAppIds(const QString& html) const
{
    QStringList appIds;
    QList<QmlAppMarker> markers = parseContent(html);
    
    for (const QmlAppMarker& marker : markers) {
        if (!marker.appId.isEmpty()) {
            appIds.append(marker.appId);
        }
    }
    
    return appIds;
}

// ============================================================================
// Private Methods - Marker Detection
// ============================================================================

ContentParser::QmlAppMarker ContentParser::findNextMarker(const QString& html, int startPos) const
{
    /**
     * @brief Find next QML app marker in HTML
     * 
     * Searches for QML app marker starting from startPos. Returns marker
     * with appId, position, and length if found, or invalid marker if not found.
     * 
     * @param html HTML content to search
     * @param startPos Starting position for search
     * @return QmlAppMarker if found, or invalid marker (empty appId) if not found
     */
    
    QmlAppMarker marker;
    marker.position = -1;
    marker.length = 0;
    
    // Pattern to match: <div data-smartbook-qml-app="app_id">
    // Also handle self-closing: <div data-smartbook-qml-app="app_id" />
    // Use QString to avoid raw string literal issues with ?: in regex
    QString pattern = QStringLiteral("<div\\s+data-smartbook-qml-app=\"([^\"]+)\"[^>]*(?:>.*?</div>|/>)");
    QRegularExpression regex(
        pattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );
    
    QRegularExpressionMatch match = regex.match(html, startPos);
    if (match.hasMatch()) {
        marker.appId = match.captured(1);
        marker.position = match.capturedStart(0);
        marker.length = match.capturedLength(0);
    }
    
    return marker;
}

QList<ContentParser::FormMarker> ContentParser::parseFormMarkers(const QString& html) const
{
    QList<FormMarker> markers;
    int startPos = 0;
    
    while (startPos < html.length()) {
        FormMarker marker = findNextFormMarker(html, startPos);
        if (marker.formId.isEmpty()) {
            // No more markers found
            break;
        }
        
        markers.append(marker);
        startPos = marker.position + marker.length;
    }
    
    return markers;
}

QStringList ContentParser::extractFormIds(const QString& html) const
{
    QStringList formIds;
    QList<FormMarker> markers = parseFormMarkers(html);
    
    for (const FormMarker& marker : markers) {
        if (!marker.formId.isEmpty()) {
            formIds.append(marker.formId);
        }
    }
    
    return formIds;
}

ContentParser::FormMarker ContentParser::findNextFormMarker(const QString& html, int startPos) const
{
    FormMarker marker;
    marker.position = -1;
    marker.length = 0;
    
    // Pattern to match: <div data-smartbook-form="form_id">
    // Also handle self-closing: <div data-smartbook-form="form_id" />
    QString pattern = QStringLiteral("<div\\s+data-smartbook-form=\"([^\"]+)\"[^>]*(?:>.*?</div>|/>)");
    QRegularExpression regex(
        pattern,
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );
    
    QRegularExpressionMatch match = regex.match(html, startPos);
    if (match.hasMatch()) {
        marker.formId = match.captured(1);
        marker.position = match.capturedStart(0);
        marker.length = match.capturedLength(0);
    }
    
    return marker;
}

} // namespace reader
} // namespace smartbook

