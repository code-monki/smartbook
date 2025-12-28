#ifndef SMARTBOOK_COMMON_UTILS_THEMEMANAGER_H
#define SMARTBOOK_COMMON_UTILS_THEMEMANAGER_H

#include <QString>
#include <QObject>

namespace smartbook {
namespace common {
namespace utils {

/**
 * @brief Global theme manager for all SmartBook applications
 * 
 * ThemeManager is a singleton utility that manages theme preferences shared across
 * all SmartBook applications (Library Manager, Reader, Creator Tool). It provides
 * a centralized way to get, set, and resolve theme preferences with persistence
 * across application launches.
 * 
 * @section Themes
 * 
 * **Supported Themes:**
 * - "auto" - Automatically detects system appearance (dark/light)
 * - "light" - Light theme (white background, black text)
 * - "dark" - Dark theme (dark background, light text)
 * - "sepia" - Sepia theme (sepia background, dark brown text)
 * 
 * **Theme Resolution:**
 * - "auto" theme is resolved to "dark" or "light" based on system appearance
 * - Other themes are returned as-is
 * - System appearance detection uses platform-specific APIs
 * 
 * @section Persistence
 * 
 * **Storage:**
 * - Theme preference is stored in QSettings with key "global_theme"
 * - Uses organization name "SmartBook" to ensure all apps share the same preference
 * - Persists across application launches
 * 
 * **Default:**
 * - Default theme is "auto" (system appearance)
 * - If no preference is stored, "auto" is returned
 * 
 * @section Signals
 * 
 * **themeChanged:**
 * - Emitted when theme preference changes
 * - All applications should listen to this signal and update their UI
 * - Signal is emitted synchronously when setTheme() is called
 * 
 * @section Usage
 * 
 * @code
 * // Get singleton instance
 * ThemeManager& themeManager = ThemeManager::getInstance();
 * 
 * // Get current theme
 * QString theme = themeManager.getTheme();
 * 
 * // Set theme (emits themeChanged signal)
 * themeManager.setTheme("dark");
 * 
 * // Resolve theme (if "auto", returns system theme)
 * QString resolved = themeManager.resolveTheme(theme);
 * 
 * // Listen for theme changes
 * connect(&themeManager, &ThemeManager::themeChanged,
 *         this, [](const QString& theme) {
 *     // Update UI to match new theme
 * });
 * @endcode
 * 
 * @section Thread Safety
 * 
 * ThemeManager is not thread-safe. All methods should be called from the main
 * (GUI) thread. QSettings operations are thread-safe, but signal emissions
 * must occur on the correct thread.
 * 
 * @note This is a singleton class. The constructor and destructor are private,
 * and the class cannot be copied or moved.
 * 
 * @see ReaderView
 * @see LibraryManager
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     */
    static ThemeManager& getInstance();

    /**
     * @brief Get the current theme preference
     * @return Theme name: "auto", "light", "dark", or "sepia"
     */
    QString getTheme() const;

    /**
     * @brief Set the theme preference
     * @param theme Theme name: "auto", "light", "dark", or "sepia"
     */
    void setTheme(const QString& theme);

    /**
     * @brief Detect system theme (dark or light)
     * @return "dark" or "light"
     */
    static QString detectSystemTheme();

    /**
     * @brief Resolve theme (if "auto", returns detected system theme)
     * @param theme Theme preference
     * @return Resolved theme: "light", "dark", or "sepia"
     */
    QString resolveTheme(const QString& theme) const;

signals:
    /**
     * @brief Emitted when theme preference changes
     * @param theme New theme preference
     */
    void themeChanged(const QString& theme);

private:
    ThemeManager() = default;
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    static constexpr const char* SETTINGS_KEY = "global_theme";
    static constexpr const char* DEFAULT_THEME = "auto";
};

} // namespace utils
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_UTILS_THEMEMANAGER_H

