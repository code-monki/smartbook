#ifndef SMARTBOOK_COMMON_UTILS_PLATFORMUTILS_H
#define SMARTBOOK_COMMON_UTILS_PLATFORMUTILS_H

#include <QString>

namespace smartbook {
namespace common {
namespace utils {

/**
 * @brief Platform detection and utility functions
 */
class PlatformUtils {
public:
    /**
     * @brief Get the current platform name
     * @return "linux", "macos", or "windows"
     */
    static QString getPlatform();

    /**
     * @brief Get the current architecture
     * @return "arm64" or "x86_64"
     */
    static QString getArchitecture();

    /**
     * @brief Get application data directory
     * @return Platform-specific application data directory path
     */
    static QString getApplicationDataDirectory();

    /**
     * @brief Get cache directory
     * @return Platform-specific cache directory path
     */
    static QString getCacheDirectory();

    /**
     * @brief Get log directory
     * @return Platform-specific log directory path
     */
    static QString getLogDirectory();

    /**
     * @brief Get backup directory
     * @return Platform-specific backup directory path
     */
    static QString getBackupDirectory();
};

} // namespace utils
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_UTILS_PLATFORMUTILS_H
