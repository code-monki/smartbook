#include "smartbook/reader/ContentParser.h"
#include <QRegularExpression>
#include <QDebug>

namespace smartbook {
namespace reader {

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
    QString cleaned = html;
    
    // Use regex to find and remove QML app marker divs
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

ContentParser::QmlAppMarker ContentParser::findNextMarker(const QString& html, int startPos) const
{
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

} // namespace reader
} // namespace smartbook

