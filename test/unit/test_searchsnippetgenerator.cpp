#include <QtTest>
#include "smartbook/common/utils/SearchSnippetGenerator.h"
#include <QCoreApplication>

using namespace smartbook::common::utils;

class TestSearchSnippetGenerator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testStripHtmlTags();
    void testGenerateSnippets();
    void testHighlightSearchTerm();
    void testContextSnippetGeneration();
    void testCaseSensitiveSearch();
    void testMultipleMatches();
    void testEmptyInputs();
    void testLongSnippetTruncation();
};

void TestSearchSnippetGenerator::initTestCase()
{
    // No setup needed for utility class tests
}

void TestSearchSnippetGenerator::cleanupTestCase()
{
    // No cleanup needed
}

void TestSearchSnippetGenerator::testStripHtmlTags()
{
    QString html = "<p>This is a <strong>test</strong> with <em>HTML</em> tags.</p>";
    QString expected = "This is a test with HTML tags.";
    QString result = SearchSnippetGenerator::stripHtmlTags(html);
    QCOMPARE(result, expected);
}

void TestSearchSnippetGenerator::testGenerateSnippets()
{
    QString html = R"(<p>This is a test document. The character sheet includes ability scores, 
        skills, and equipment. Players can create characters using the character creation rules.</p>)";
    
    QString searchTerm = "character";
    QList<SearchResult> results = SearchSnippetGenerator::generateSnippets(html, searchTerm, false, 30);
    
    QVERIFY(results.size() >= 3); // Should find at least 3 matches
    
    // Check first result
    QVERIFY(!results[0].contextSnippet.isEmpty());
    QVERIFY(results[0].contextSnippet.contains("<mark>"));
    QVERIFY(results[0].matchIndex == 1);
    QVERIFY(results[0].totalMatches == results.size());
}

void TestSearchSnippetGenerator::testHighlightSearchTerm()
{
    QString text = "The character sheet includes character creation rules.";
    QString searchTerm = "character";
    QString result = SearchSnippetGenerator::highlightSearchTerm(text, searchTerm, false);
    
    QVERIFY(result.contains("<mark>character</mark>"));
    QVERIFY(result.count("<mark>") == 2); // Two occurrences
}

void TestSearchSnippetGenerator::testContextSnippetGeneration()
{
    QString plainText = "This is a long text that contains the search term multiple times. "
                       "The search term appears here and also later in the text. "
                       "Finally, the search term appears at the end.";
    
    QString searchTerm = "search term";
    int matchPosition = plainText.indexOf(searchTerm);
    int matchLength = searchTerm.length();
    
    QString snippet = SearchSnippetGenerator::generateContextSnippet(
        plainText, matchPosition, matchLength, searchTerm, 30, false
    );
    
    QVERIFY(!snippet.isEmpty());
    QVERIFY(snippet.contains("<mark>"));
    QVERIFY(snippet.length() <= SearchSnippetGenerator::MAX_SNIPPET_LENGTH + 10); // Allow some margin
}

void TestSearchSnippetGenerator::testLongSnippetTruncation()
{
    // Test that long snippets are truncated properly
    QString longHtml = "<p>" + QString("A").repeated(500) + "search term" + QString("B").repeated(500) + "</p>";
    QString searchTerm = "search term";
    
    QList<SearchResult> results = SearchSnippetGenerator::generateSnippets(longHtml, searchTerm, false, 100);
    
    QVERIFY(!results.isEmpty());
    // Snippet should be truncated if it exceeds MAX_SNIPPET_LENGTH
    QVERIFY(results[0].contextSnippet.length() <= SearchSnippetGenerator::MAX_SNIPPET_LENGTH + 50); // Allow some margin
}

void TestSearchSnippetGenerator::testCaseSensitiveSearch()
{
    QString html = "<p>The Character sheet includes character creation rules.</p>";
    QString searchTerm = "Character";
    
    // Case-sensitive search should find only one match
    QList<SearchResult> results = SearchSnippetGenerator::generateSnippets(html, searchTerm, true, 30);
    QCOMPARE(results.size(), 1);
    
    // Case-insensitive search should find two matches
    results = SearchSnippetGenerator::generateSnippets(html, searchTerm, false, 30);
    QCOMPARE(results.size(), 2);
}

void TestSearchSnippetGenerator::testMultipleMatches()
{
    QString html = "<p>Character one. Character two. Character three.</p>";
    QString searchTerm = "Character";
    
    QList<SearchResult> results = SearchSnippetGenerator::generateSnippets(html, searchTerm, false, 20);
    
    QCOMPARE(results.size(), 3);
    QCOMPARE(results[0].matchIndex, 1);
    QCOMPARE(results[0].totalMatches, 3);
    QCOMPARE(results[1].matchIndex, 2);
    QCOMPARE(results[2].matchIndex, 3);
}

void TestSearchSnippetGenerator::testEmptyInputs()
{
    // Test with empty HTML
    QList<SearchResult> results = SearchSnippetGenerator::generateSnippets("", "test", false);
    QCOMPARE(results.size(), 0);
    
    // Test with empty search term
    results = SearchSnippetGenerator::generateSnippets("<p>Some content</p>", "", false);
    QCOMPARE(results.size(), 0);
    
    // Test stripHtmlTags with empty input
    QString result = SearchSnippetGenerator::stripHtmlTags("");
    QCOMPARE(result, QString());
}

QTEST_MAIN(TestSearchSnippetGenerator)
#include "test_searchsnippetgenerator.moc"

