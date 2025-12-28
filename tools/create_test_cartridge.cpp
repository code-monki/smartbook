/**
 * @file create_test_cartridge.cpp
 * @brief Utility to create a test cartridge for manual testing and development
 * 
 * This utility creates a complete test cartridge with sample content for manual
 * testing of the Reader application. It is useful for:
 * - Manual testing of Reader features
 * - Development and debugging
 * - Creating demo cartridges
 * - Testing cartridge import workflow
 * 
 * The created cartridge includes:
 * - Sample HTML content pages
 * - QML embedded application example
 * - Form definition example (contact form)
 * - Author-defined settings
 * - Complete database schema (Metadata, Content_Pages, Embedded_Apps, etc.)
 * 
 * Usage:
 *   ./create_test_cartridge <output_path>
 * 
 * Example:
 *   ./create_test_cartridge ~/Desktop/test_cartridge.sqlite
 *   ./create_test_cartridge /tmp/my_test_cartridge.sqlite
 * 
 * Note:
 *   This is a standalone utility for manual testing. Automated tests use
 *   TestHelpers (test/unit/test_helpers.h) for programmatic cartridge creation.
 * 
 * See Documentation/testing-guide.adoc for testing information.
 */

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>
#include <QFile>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    if (argc < 2) {
        qWarning() << "Usage: create_test_cartridge <output_path>";
        qWarning() << "Example: create_test_cartridge ~/Desktop/test_cartridge.sqlite";
        return 1;
    }
    
    QString outputPath = argv[1];
    
    // Expand user home directory
    if (outputPath.startsWith("~/")) {
        outputPath = QDir::homePath() + outputPath.mid(1);
    }
    
    // Ensure directory exists
    QDir dir = QFileInfo(outputPath).absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCritical() << "Failed to create directory:" << dir.absolutePath();
            return 1;
        }
    }
    
    // Remove existing file if it exists
    if (QFile::exists(outputPath)) {
        QFile::remove(outputPath);
    }
    
    QString guid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString connectionName = "TestCartridge_" + guid;
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(outputPath);
    
    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return 1;
    }
    
    QSqlQuery query(db);
    
    // Create Metadata table
    query.exec(R"(
        CREATE TABLE Metadata (
            cartridge_guid TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            publication_year TEXT NOT NULL,
            version TEXT NOT NULL DEFAULT '1.0',
            schema_version TEXT NOT NULL DEFAULT '1.0',
            publisher TEXT
        )
    )");
    
    query.prepare("INSERT INTO Metadata (cartridge_guid, title, author, publication_year, version, schema_version) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue("Test Cartridge for Manual Testing");
    query.addBindValue("Test Author");
    query.addBindValue("2025");
    query.addBindValue("1.0");
    query.addBindValue("1.0");
    if (!query.exec()) {
        qCritical() << "Failed to insert metadata:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return 1;
    }
    
    // Create Content_Pages table
    query.exec(R"(
        CREATE TABLE Content_Pages (
            page_id INTEGER PRIMARY KEY,
            page_order INTEGER NOT NULL UNIQUE,
            chapter_title TEXT,
            html_content TEXT NOT NULL,
            associated_css TEXT
        )
    )");
    
    // Insert sample content pages
    QString htmlContent1 = R"(
        <h1>Welcome to the Test Cartridge</h1>
        <p>This is a test cartridge for manual testing of the SmartBook reader.</p>
        
        <h2>Features to Test</h2>
        <ul>
            <li><strong>Theme Changes:</strong> Try switching between light, dark, and sepia themes</li>
            <li><strong>Content Rendering:</strong> This HTML should render correctly</li>
            <li><strong>QML Apps:</strong> See the embedded QML app below</li>
            <li><strong>Forms:</strong> Try the form on the next page</li>
        </ul>
        
        <h2>QML Embedded App</h2>
        <p>Below is an embedded QML application:</p>
        <div data-smartbook-qml-app="test_app_1"></div>
        
        <p>This app should display a blue rectangle with text.</p>
    )";
    
    query.prepare("INSERT INTO Content_Pages (page_id, page_order, chapter_title, html_content) VALUES (?, ?, ?, ?)");
    query.addBindValue(1);
    query.addBindValue(1);
    query.addBindValue("Introduction");
    query.addBindValue(htmlContent1);
    if (!query.exec()) {
        qCritical() << "Failed to insert page 1:" << query.lastError().text();
    }
    
    QString htmlContent2 = R"(
        <h1>Form Testing Page</h1>
        <p>This page contains an embedded form for testing form functionality.</p>
        
        <h2>Contact Form</h2>
        <p>Fill out the form below:</p>
        <div data-smartbook-form="contact_form"></div>
        
        <h2>More Content</h2>
        <p>You can continue reading after the form. The form data should persist when you navigate away and come back.</p>
    )";
    
    query.prepare("INSERT INTO Content_Pages (page_id, page_order, chapter_title, html_content) VALUES (?, ?, ?, ?)");
    query.addBindValue(2);
    query.addBindValue(2);
    query.addBindValue("Forms");
    query.addBindValue(htmlContent2);
    if (!query.exec()) {
        qCritical() << "Failed to insert page 2:" << query.lastError().text();
    }
    
    // Create Embedded_Apps table
    query.exec(R"(
        CREATE TABLE Embedded_Apps (
            app_id TEXT PRIMARY KEY,
            app_name TEXT NOT NULL,
            qml_code TEXT,
            javascript_code TEXT,
            app_config_json TEXT
        )
    )");
    
    // Insert QML app
    QString qmlCode = R"(
        import QtQuick 2.15
        import QtQuick.Controls 2.15
        import SmartBook 1.0
        
        Rectangle {
            id: root
            width: 300
            height: 150
            color: "lightblue"
            border.color: "darkblue"
            border.width: 2
            radius: 10
            
            SmartbookBridge {
                id: bridge
            }
            
            Column {
                anchors.centerIn: parent
                spacing: 10
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Test QML App"
                    font.pixelSize: 18
                    font.bold: true
                }
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "This is an embedded QML application"
                    font.pixelSize: 12
                    color: "darkblue"
                }
                
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Click Me!"
                    onClicked: {
                        console.log("Button clicked!");
                    }
                }
            }
        }
    )";
    
    query.prepare("INSERT INTO Embedded_Apps (app_id, app_name, qml_code) VALUES (?, ?, ?)");
    query.addBindValue("test_app_1");
    query.addBindValue("Test QML App");
    query.addBindValue(qmlCode);
    if (!query.exec()) {
        qCritical() << "Failed to insert QML app:" << query.lastError().text();
    }
    
    // Create Form_Definitions table
    query.exec(R"(
        CREATE TABLE Form_Definitions (
            form_id TEXT PRIMARY KEY,
            form_title TEXT NOT NULL,
            form_schema_json TEXT NOT NULL
        )
    )");
    
    // Insert contact form schema
    QString formSchema = R"({
        "type": "object",
        "title": "Contact Form",
        "properties": {
            "name": {
                "type": "string",
                "title": "Name",
                "defaultValue": ""
            },
            "email": {
                "type": "string",
                "title": "Email",
                "defaultValue": ""
            },
            "message": {
                "type": "string",
                "title": "Message",
                "defaultValue": ""
            }
        },
        "required": ["name", "email"]
    })";
    
    query.prepare("INSERT INTO Form_Definitions (form_id, form_title, form_schema_json) VALUES (?, ?, ?)");
    query.addBindValue("contact_form");
    query.addBindValue("Contact Form");
    query.addBindValue(formSchema);
    if (!query.exec()) {
        qCritical() << "Failed to insert form:" << query.lastError().text();
    }
    
    // Create User_Data table for form data persistence
    query.exec(R"(
        CREATE TABLE User_Data (
            form_key TEXT PRIMARY KEY,
            serialized_data TEXT NOT NULL,
            timestamp TEXT NOT NULL
        )
    )");
    
    // Create Settings table
    query.exec(R"(
        CREATE TABLE Settings (
            setting_key TEXT PRIMARY KEY,
            setting_value TEXT NOT NULL,
            setting_type TEXT NOT NULL DEFAULT 'author'
        )
    )");
    
    // Insert some default settings
    query.prepare("INSERT INTO Settings (setting_key, setting_value, setting_type) VALUES (?, ?, ?)");
    query.addBindValue("default_theme");
    query.addBindValue("light");
    query.addBindValue("author");
    query.exec();
    
    query.prepare("INSERT INTO Settings (setting_key, setting_value, setting_type) VALUES (?, ?, ?)");
    query.addBindValue("default_font_size");
    query.addBindValue("12");
    query.addBindValue("author");
    query.exec();
    
    // Create Cartridge_Security table (required for validation)
    query.exec(R"(
        CREATE TABLE Cartridge_Security (
            cartridge_guid TEXT PRIMARY KEY,
            h1_hash BLOB,
            signature_data BLOB,
            certificate_data BLOB
        )
    )");
    
    query.prepare("INSERT INTO Cartridge_Security (cartridge_guid, h1_hash, signature_data, certificate_data) VALUES (?, ?, ?, ?)");
    query.addBindValue(guid);
    query.addBindValue(QByteArray()); // Empty hash for unsigned cartridge
    query.addBindValue(QByteArray());
    query.addBindValue(QByteArray());
    query.exec();
    
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    
    qInfo() << "Test cartridge created successfully!";
    qInfo() << "Path:" << outputPath;
    qInfo() << "GUID:" << guid;
    qInfo() << "";
    qInfo() << "You can now import this cartridge into SmartBook for testing.";
    
    return 0;
}

