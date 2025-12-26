#ifndef SMARTBOOK_COMMON_UTILS_SEARCHSNIPPETGENERATOR_H
#define SMARTBOOK_COMMON_UTILS_SEARCHSNIPPETGENERATOR_H

#include <QString>
#include <QStringList>

namespace smartbook {
namespace common {
namespace utils {

/**
 * @brief Search result information structure
 */
struct SearchResult {
    int pageId;                    // Page ID where match was found
    int pageOrder;                 // Page order for sorting
    QString chapterTitle;          // Chapter title (if available)
    QString matchText;             // The matched text
    QString contextSnippet;        // Context snippet with highlighted match
    int matchPosition;             // Character position of match in page content
    int matchIndex;                 // Match index (1-based) for "Match 1 of N" display
    int totalMatches;              // Total matches in this page
};

/**
 * @brief Utility class for generating context snippets from search matches
 * 
 * Implements context snippet generation as specified in DDD Section "Search Functionality Implementation Specification".
 * Provides functionality to:
 * - Strip HTML tags to extract plain text
 * - Find search term matches in text
 * - Generate context snippets with configurable context size
 * - Highlight search terms in snippets
 * - Handle truncation with ellipsis
 */
class SearchSnippetGenerator {
public:
    /**
     * @brief Default context size (characters before and after match)
     */
    static const int DEFAULT_CONTEXT_SIZE = 50;
    
    /**
     * @brief Maximum snippet length before truncation
     */
    static const int MAX_SNIPPET_LENGTH = 200;

    /**
     * @brief Generate context snippet from HTML content and search term
     * @param htmlContent HTML content to search
     * @param searchTerm Search term to find
     * @param caseSensitive Whether search should be case-sensitive
     * @param contextSize Number of characters to include before and after match (default: 50)
     * @return List of search results with context snippets
     */
    static QList<SearchResult> generateSnippets(
        const QString& htmlContent,
        const QString& searchTerm,
        bool caseSensitive = false,
        int contextSize = DEFAULT_CONTEXT_SIZE
    );

    /**
     * @brief Strip HTML tags from content to get plain text
     * @param htmlContent HTML content
     * @return Plain text with HTML tags removed
     */
    static QString stripHtmlTags(const QString& htmlContent);

    /**
     * @brief Generate a single context snippet for a match
     * @param plainText Plain text content
     * @param matchPosition Character position of match
     * @param matchLength Length of matched text
     * @param searchTerm Original search term (for highlighting)
     * @param contextSize Number of characters before and after match
     * @param caseSensitive Whether highlighting should be case-sensitive
     * @return Context snippet with highlighted match
     */
    static QString generateContextSnippet(
        const QString& plainText,
        int matchPosition,
        int matchLength,
        const QString& searchTerm,
        int contextSize = DEFAULT_CONTEXT_SIZE,
        bool caseSensitive = false
    );

    /**
     * @brief Highlight search term in text
     * @param text Text containing the search term
     * @param searchTerm Term to highlight
     * @param caseSensitive Whether matching should be case-sensitive
     * @return Text with search term wrapped in <mark> tags
     */
    static QString highlightSearchTerm(
        const QString& text,
        const QString& searchTerm,
        bool caseSensitive = false
    );

private:
    /**
     * @brief Find all occurrences of search term in text
     * @param text Text to search
     * @param searchTerm Term to find
     * @param caseSensitive Whether search should be case-sensitive
     * @return List of match positions (character offsets)
     */
    static QList<int> findMatches(
        const QString& text,
        const QString& searchTerm,
        bool caseSensitive = false
    );

    /**
     * @brief Truncate text with ellipsis if it exceeds maximum length
     * @param text Text to truncate
     * @param maxLength Maximum length
     * @return Truncated text with ellipsis if needed
     */
    static QString truncateWithEllipsis(const QString& text, int maxLength = MAX_SNIPPET_LENGTH);
};

} // namespace utils
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_UTILS_SEARCHSNIPPETGENERATOR_H

