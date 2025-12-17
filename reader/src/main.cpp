#include <QApplication>
#include <QStyleFactory>
#include "smartbook/reader/LibraryManager.h"
#include "smartbook/common/utils/PlatformUtils.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    app.setApplicationName("SmartBook Reader");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SmartBook");
    app.setOrganizationDomain("smartbook.org");

    // Apply Qt Fusion style for uniform appearance
    app.setStyle(QStyleFactory::create("Fusion"));

    // Initialize platform-specific settings
    smartbook::common::utils::PlatformUtils::getApplicationDataDirectory();

    // Create and show Library Manager
    smartbook::reader::LibraryManager libraryManager;
    libraryManager.show();

    return app.exec();
}
