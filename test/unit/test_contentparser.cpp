/**
 * @file test_contentparser.cpp
 * @brief Unit tests for ContentParser
 * 
 * Tests for parsing HTML content and detecting QML app markers.
 */

#include <QtTest>
#include "smartbook/reader/ContentParser.h"
#include <QCoreApplication>
#include <QString>

class TestContentParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Test QML marker detection
    void testDetectQmlAppMarkers();
    void testDetectMultipleQmlAppMarkers();
    void testDetectNoQmlAppMarkers();
    void testDetectQmlAppMarkersWithAttributes();
    
    // Test HTML cleaning
    void testCleanHtmlRemovesMarkers();
    void testCleanHtmlPreservesOtherContent();
    void testCleanHtmlWithNestedContent();
    
    // Test marker extraction
    void testExtractAppIds();
    void testExtractAppIdsWithInvalidMarkers();
    void testExtractAppIdsOrder();
    
    // Test marker positions
    void testGetMarkerPositions();
    void testGetMarkerPositionsWithMultipleApps();
    
    // Test form marker detection
    void testParseFormMarker();
    void testParseFormMarkerMultiple();
    void testParseFormMarkerSelfClosing();
    void testCleanHtmlRemovesFormMarkers();
    void testExtractFormIds();
};

void TestContentParser::initTestCase()
{
    // Create QCoreApplication if it doesn't exist
    if (!qApp) {
        int argc = 0;
        char** argv = nullptr;
        new QCoreApplication(argc, argv);
    }
}

void TestContentParser::cleanupTestCase()
{
}

void TestContentParser::testDetectQmlAppMarkers()
{
    QString html = R"(
        <p>Some content</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>More content</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::QmlAppMarker> markers = parser.parseContent(html);
    
    QCOMPARE(markers.size(), 1);
    QCOMPARE(markers[0].appId, QString("app_1"));
}

void TestContentParser::testDetectMultipleQmlAppMarkers()
{
    QString html = R"(
        <p>Content before</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>Middle content</p>
        <div data-smartbook-qml-app="app_2"></div>
        <p>Content after</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::QmlAppMarker> markers = parser.parseContent(html);
    
    QCOMPARE(markers.size(), 2);
    QCOMPARE(markers[0].appId, QString("app_1"));
    QCOMPARE(markers[1].appId, QString("app_2"));
}

void TestContentParser::testDetectNoQmlAppMarkers()
{
    QString html = R"(
        <p>Regular content</p>
        <div>No markers here</div>
        <p>More content</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::QmlAppMarker> markers = parser.parseContent(html);
    
    QCOMPARE(markers.size(), 0);
}

void TestContentParser::testDetectQmlAppMarkersWithAttributes()
{
    QString html = R"(
        <div data-smartbook-qml-app="app_1" class="embedded-app" style="width: 400px;"></div>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::QmlAppMarker> markers = parser.parseContent(html);
    
    QCOMPARE(markers.size(), 1);
    QCOMPARE(markers[0].appId, QString("app_1"));
}

void TestContentParser::testCleanHtmlRemovesMarkers()
{
    QString html = R"(
        <p>Content before</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>Content after</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QString cleaned = parser.cleanHtml(html);
    
    QVERIFY(!cleaned.contains("data-smartbook-qml-app"));
    QVERIFY(!cleaned.contains("app_1"));
    QVERIFY(cleaned.contains("Content before"));
    QVERIFY(cleaned.contains("Content after"));
}

void TestContentParser::testCleanHtmlPreservesOtherContent()
{
    QString html = R"(
        <h1>Title</h1>
        <p>Paragraph with <strong>bold</strong> text</p>
        <div data-smartbook-qml-app="app_1"></div>
        <ul>
            <li>Item 1</li>
            <li>Item 2</li>
        </ul>
    )";
    
    smartbook::reader::ContentParser parser;
    QString cleaned = parser.cleanHtml(html);
    
    QVERIFY(cleaned.contains("Title"));
    QVERIFY(cleaned.contains("bold"));
    QVERIFY(cleaned.contains("Item 1"));
    QVERIFY(cleaned.contains("Item 2"));
    QVERIFY(!cleaned.contains("data-smartbook-qml-app"));
}

void TestContentParser::testCleanHtmlWithNestedContent()
{
    QString html = R"(
        <div>
            <p>Nested content</p>
            <div data-smartbook-qml-app="app_1">
                <p>This should be removed</p>
            </div>
            <p>After marker</p>
        </div>
    )";
    
    smartbook::reader::ContentParser parser;
    QString cleaned = parser.cleanHtml(html);
    
    QVERIFY(cleaned.contains("Nested content"));
    QVERIFY(cleaned.contains("After marker"));
    QVERIFY(!cleaned.contains("data-smartbook-qml-app"));
    QVERIFY(!cleaned.contains("This should be removed"));
}

void TestContentParser::testExtractAppIds()
{
    QString html = R"(
        <div data-smartbook-qml-app="app_1"></div>
        <div data-smartbook-qml-app="app_2"></div>
        <div data-smartbook-qml-app="app_3"></div>
    )";
    
    smartbook::reader::ContentParser parser;
    QStringList appIds = parser.extractAppIds(html);
    
    QCOMPARE(appIds.size(), 3);
    QVERIFY(appIds.contains("app_1"));
    QVERIFY(appIds.contains("app_2"));
    QVERIFY(appIds.contains("app_3"));
}

void TestContentParser::testExtractAppIdsWithInvalidMarkers()
{
    QString html = R"(
        <div data-smartbook-qml-app="app_1"></div>
        <div data-smartbook-qml-app=""></div>
        <div data-smartbook-qml-app="app_2"></div>
    )";
    
    smartbook::reader::ContentParser parser;
    QStringList appIds = parser.extractAppIds(html);
    
    // Should only extract valid (non-empty) app IDs
    QCOMPARE(appIds.size(), 2);
    QVERIFY(appIds.contains("app_1"));
    QVERIFY(appIds.contains("app_2"));
}

void TestContentParser::testExtractAppIdsOrder()
{
    QString html = R"(
        <div data-smartbook-qml-app="first"></div>
        <p>Content</p>
        <div data-smartbook-qml-app="second"></div>
        <p>More content</p>
        <div data-smartbook-qml-app="third"></div>
    )";
    
    smartbook::reader::ContentParser parser;
    QStringList appIds = parser.extractAppIds(html);
    
    // Should preserve order
    QCOMPARE(appIds.size(), 3);
    QCOMPARE(appIds[0], QString("first"));
    QCOMPARE(appIds[1], QString("second"));
    QCOMPARE(appIds[2], QString("third"));
}

void TestContentParser::testGetMarkerPositions()
{
    QString html = R"(
        <p>Before</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>After</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::QmlAppMarker> markers = parser.parseContent(html);
    
    QCOMPARE(markers.size(), 1);
    // Position should be after "Before" content
    QVERIFY(markers[0].position > 0);
}

void TestContentParser::testGetMarkerPositionsWithMultipleApps()
{
    QString html = R"(
        <p>Start</p>
        <div data-smartbook-qml-app="app_1"></div>
        <p>Middle</p>
        <div data-smartbook-qml-app="app_2"></div>
        <p>End</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::QmlAppMarker> markers = parser.parseContent(html);
    
    QCOMPARE(markers.size(), 2);
    // Positions should be in order
    QVERIFY(markers[0].position < markers[1].position);
}

// Test form marker detection
void TestContentParser::testParseFormMarker()
{
    QString html = R"(
        <p>Some content</p>
        <div data-smartbook-form="form_1"></div>
        <p>More content</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::FormMarker> markers = parser.parseFormMarkers(html);
    
    QCOMPARE(markers.size(), 1);
    QCOMPARE(markers[0].formId, QString("form_1"));
}

void TestContentParser::testParseFormMarkerMultiple()
{
    QString html = R"(
        <p>Content before</p>
        <div data-smartbook-form="form_1"></div>
        <p>Middle content</p>
        <div data-smartbook-form="form_2"></div>
        <p>Content after</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::FormMarker> markers = parser.parseFormMarkers(html);
    
    QCOMPARE(markers.size(), 2);
    QCOMPARE(markers[0].formId, QString("form_1"));
    QCOMPARE(markers[1].formId, QString("form_2"));
}

void TestContentParser::testParseFormMarkerSelfClosing()
{
    QString html = R"(
        <p>Content</p>
        <div data-smartbook-form="form_1" />
        <p>More content</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QList<smartbook::reader::ContentParser::FormMarker> markers = parser.parseFormMarkers(html);
    
    QCOMPARE(markers.size(), 1);
    QCOMPARE(markers[0].formId, QString("form_1"));
}

void TestContentParser::testCleanHtmlRemovesFormMarkers()
{
    QString html = R"(
        <p>Content before</p>
        <div data-smartbook-form="form_1"></div>
        <p>Content after</p>
    )";
    
    smartbook::reader::ContentParser parser;
    QString cleaned = parser.cleanHtml(html);
    
    QVERIFY(!cleaned.contains("data-smartbook-form"));
    QVERIFY(!cleaned.contains("form_1"));
    QVERIFY(cleaned.contains("Content before"));
    QVERIFY(cleaned.contains("Content after"));
}

void TestContentParser::testExtractFormIds()
{
    QString html = R"(
        <div data-smartbook-form="form_1"></div>
        <div data-smartbook-form="form_2"></div>
        <div data-smartbook-form="form_3"></div>
    )";
    
    smartbook::reader::ContentParser parser;
    QStringList formIds = parser.extractFormIds(html);
    
    QCOMPARE(formIds.size(), 3);
    QVERIFY(formIds.contains("form_1"));
    QVERIFY(formIds.contains("form_2"));
    QVERIFY(formIds.contains("form_3"));
}

QTEST_MAIN(TestContentParser)
#include "test_contentparser.moc"

