#include "smartbook/common/utils/SearchSnippetGenerator.h"
#include <QRegularExpression>
#include <QTextDocument>
#include <QDebug>

namespace smartbook {
namespace common {
namespace utils {

QList<SearchResult> SearchSnippetGenerator::generateSnippets(
    const QString& htmlContent,
    const QString& searchTerm,
    bool caseSensitive,
    int contextSize)
{
    QList<SearchResult> results;
    
    if (searchTerm.isEmpty() || htmlContent.isEmpty()) {
        return results;
    }
    
    // Strip HTML tags to get plain text
    QString plainText = stripHtmlTags(htmlContent);
    
    // Find all matches
    QList<int> matchPositions = findMatches(plainText, searchTerm, caseSensitive);
    
    if (matchPositions.isEmpty()) {
        return results;
    }
    
    // Generate snippet for each match
    int matchLength = caseSensitive ? searchTerm.length() : searchTerm.toLower().length();
    for (int i = 0; i < matchPositions.size(); ++i) {
        int position = matchPositions[i];
        
        SearchResult result;
        result.matchPosition = position;
        result.matchIndex = i + 1;
        result.totalMatches = matchPositions.size();
        
        // Extract match text
        result.matchText = plainText.mid(position, matchLength);
        
        // Generate context snippet
        result.contextSnippet = generateContextSnippet(
            plainText,
            position,
            matchLength,
            searchTerm,
            contextSize,
            caseSensitive
        );
        
        results.append(result);
    }
    
    return results;
}

QString SearchSnippetGenerator::stripHtmlTags(const QString& htmlContent)
{
    // Use QTextDocument to strip HTML tags and decode entities
    QTextDocument doc;
    doc.setHtml(htmlContent);
    return doc.toPlainText();
}

QString SearchSnippetGenerator::generateContextSnippet(
    const QString& plainText,
    int matchPosition,
    int matchLength,
    const QString& searchTerm,
    int contextSize,
    bool caseSensitive)
{
    if (plainText.isEmpty() || matchPosition < 0 || matchPosition >= plainText.length()) {
        return QString();
    }
    
    // Calculate start and end positions for context
    int startPos = qMax(0, matchPosition - contextSize);
    int endPos = qMin(plainText.length(), matchPosition + matchLength + contextSize);
    
    // Extract context
    QString context = plainText.mid(startPos, endPos - startPos);
    
    // Add ellipsis if we truncated at the beginning
    if (startPos > 0) {
        context = "..." + context;
    }
    
    // Add ellipsis if we truncated at the end
    if (endPos < plainText.length()) {
        context = context + "...";
    }
    
    // Highlight the search term in the context using highlightSearchTerm
    QString highlightedContext = highlightSearchTerm(context, searchTerm, caseSensitive);
    
    // Truncate if necessary
    return truncateWithEllipsis(highlightedContext);
}

QString SearchSnippetGenerator::highlightSearchTerm(
    const QString& text,
    const QString& searchTerm,
    bool caseSensitive)
{
    if (text.isEmpty() || searchTerm.isEmpty()) {
        return text;
    }
    
    QString result = text;
    Qt::CaseSensitivity sensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    
    // Find all occurrences and replace with highlighted version
    // Need to work backwards to avoid position shifts from replacements
    QList<int> positions;
    int pos = 0;
    while ((pos = result.indexOf(searchTerm, pos, sensitivity)) != -1) {
        positions.append(pos);
        pos += searchTerm.length();
    }
    
    // Replace from end to beginning to preserve positions
    for (int i = positions.size() - 1; i >= 0; --i) {
        int matchPos = positions[i];
        QString match = result.mid(matchPos, searchTerm.length());
        QString highlighted = QString("<mark>%1</mark>").arg(match);
        result.replace(matchPos, searchTerm.length(), highlighted);
    }
    
    return result;
}

QList<int> SearchSnippetGenerator::findMatches(
    const QString& text,
    const QString& searchTerm,
    bool caseSensitive)
{
    QList<int> positions;
    
    if (text.isEmpty() || searchTerm.isEmpty()) {
        return positions;
    }
    
    Qt::CaseSensitivity sensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int pos = 0;
    
    while ((pos = text.indexOf(searchTerm, pos, sensitivity)) != -1) {
        positions.append(pos);
        pos += searchTerm.length();
    }
    
    return positions;
}

QString SearchSnippetGenerator::truncateWithEllipsis(const QString& text, int maxLength)
{
    if (text.length() <= maxLength) {
        return text;
    }
    
    // If text is longer, truncate and add ellipsis
    // Try to truncate at word boundary if possible
    QString truncated = text.left(maxLength - 3); // Reserve space for "..."
    
    // Find last space before truncation point
    int lastSpace = truncated.lastIndexOf(' ');
    if (lastSpace > maxLength * 0.7) { // Only use word boundary if it's not too early
        truncated = truncated.left(lastSpace);
    }
    
    return truncated + "...";
}

} // namespace utils
} // namespace common
} // namespace smartbook

