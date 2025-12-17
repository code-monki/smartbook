#ifndef SMARTBOOK_COMMON_UTILS_PATHUTILS_H
#define SMARTBOOK_COMMON_UTILS_PATHUTILS_H

#include <QString>

namespace smartbook {
namespace common {
namespace utils {

/**
 * @brief Path utility functions
 */
class PathUtils {
public:
    /**
     * @brief Normalize a file path
     * @param path Input path
     * @return Normalized path
     */
    static QString normalizePath(const QString& path);

    /**
     * @brief Check if path is within a base directory (prevent path traversal)
     * @param path Path to check
     * @param baseDir Base directory
     * @return true if path is within base directory, false otherwise
     */
    static bool isPathWithinBase(const QString& path, const QString& baseDir);

    /**
     * @brief Get sandbox directory path for an embedded app
     * @param cartridgeGuid Cartridge GUID
     * @param appId Application ID
     * @return Sandbox directory path
     */
    static QString getSandboxPath(const QString& cartridgeGuid, const QString& appId);
};

} // namespace utils
} // namespace common
} // namespace smartbook

#endif // SMARTBOOK_COMMON_UTILS_PATHUTILS_H
