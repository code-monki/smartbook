#include <QApplication>
#include <QStyleFactory>
#include "smartbook/creator/CreatorMainWindow.h"
#include "smartbook/common/utils/PlatformUtils.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    app.setApplicationName("SmartBook Creator");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SmartBook");
    app.setOrganizationDomain("smartbook.org");

    // Apply Qt Fusion style for uniform appearance
    app.setStyle(QStyleFactory::create("Fusion"));

    // Initialize platform-specific settings
    smartbook::common::utils::PlatformUtils::getApplicationDataDirectory();

    // Create and show Creator Main Window
    smartbook::creator::CreatorMainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
