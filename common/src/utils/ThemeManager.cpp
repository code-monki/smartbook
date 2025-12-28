/**
 * @file ThemeManager.cpp
 * @brief Implementation of ThemeManager for global theme management
 * 
 * This file implements the ThemeManager singleton class, which manages theme
 * preferences shared across all SmartBook applications (Library Manager, Reader, Creator Tool).
 * 
 * @section Theme Storage
 * 
 * Theme preference is stored in QSettings with:
 * - Organization: "SmartBook"
 * - Application: "SmartBook" (shared across all apps)
 * - Key: "global_theme"
 * - Default: "auto" (system appearance)
 * 
 * @section System Theme Detection
 * 
 * System theme detection uses platform-specific APIs:
 * - Qt 6.5+: QStyleHints::colorScheme() (preferred)
 * - Fallback: Calculate luminance from system palette window color
 * 
 * @section Thread Safety
 * 
 * ThemeManager is not thread-safe. All methods should be called from the
 * main (GUI) thread. QSettings operations are thread-safe, but signal
 * emissions must occur on the correct thread.
 * 
 * @see ThemeManager.h
 */

#include "smartbook/common/utils/ThemeManager.h"
#include <QSettings>
#include <QGuiApplication>
#include <QStyleHints>
#include <QPalette>
#include <QColor>
#include <QDebug>

namespace smartbook {
namespace common {
namespace utils {

// ============================================================================
// Singleton Instance
// ============================================================================

ThemeManager& ThemeManager::getInstance() {
    static ThemeManager instance;
    return instance;
}

QString ThemeManager::getTheme() const {
    // Use QSettings with organization name only (shared across all apps)
    QSettings settings("SmartBook", "SmartBook");
    return settings.value(SETTINGS_KEY, DEFAULT_THEME).toString();
}

void ThemeManager::setTheme(const QString& theme) {
    QString oldTheme = getTheme();
    
    if (oldTheme == theme) {
        return; // No change
    }
    
    // Use QSettings with organization name only (shared across all apps)
    QSettings settings("SmartBook", "SmartBook");
    settings.setValue(SETTINGS_KEY, theme);
    settings.sync();
    
    qDebug() << "ThemeManager::setTheme: Saved theme" << theme;
    
    emit themeChanged(theme);
}

// ============================================================================
// Public Methods
// ============================================================================

QString ThemeManager::detectSystemTheme() {
    /**
     * @brief Detect system theme (dark or light)
     * 
     * Detects system appearance using platform-specific APIs:
     * 1. Qt 6.5+: QStyleHints::colorScheme() (preferred)
     * 2. Fallback: Calculate luminance from system palette window color
     * 
     * @return "dark" or "light"
     */
    
    // Try Qt 6.5+ colorScheme API first
    QGuiApplication* app = qobject_cast<QGuiApplication*>(QGuiApplication::instance());
    if (app && app->styleHints()) {
        // Qt 6.5+ has colorScheme() method
        #if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        Qt::ColorScheme scheme = app->styleHints()->colorScheme();
        if (scheme == Qt::ColorScheme::Dark) {
            return "dark";
        } else if (scheme == Qt::ColorScheme::Light) {
            return "light";
        }
        #endif
    }
    
    // Fallback: Check system palette brightness (if QApplication is available)
    // Note: This requires QApplication, so we'll default to light if not available
    QGuiApplication* guiApp = qobject_cast<QGuiApplication*>(QGuiApplication::instance());
    if (guiApp) {
        // Try to get palette from QGuiApplication
        // For a more robust solution, we could check window color from primary screen
        QColor windowColor = guiApp->palette().color(QPalette::Window);
        
        // Calculate luminance (perceived brightness)
        // Using relative luminance formula: 0.299*R + 0.587*G + 0.114*B
        double luminance = 0.299 * windowColor.red() + 0.587 * windowColor.green() + 0.114 * windowColor.blue();
        
        // If window color is dark (luminance < 128), system is in dark mode
        if (luminance < 128) {
            return "dark";
        }
    }
    
    return "light";
}

QString ThemeManager::resolveTheme(const QString& theme) const {
    if (theme == "auto") {
        return detectSystemTheme();
    }
    return theme;
}

} // namespace utils
} // namespace common
} // namespace smartbook

