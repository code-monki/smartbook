#include "smartbook/common/utils/PathUtils.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace smartbook {
namespace common {
namespace utils {

QString PathUtils::normalizePath(const QString& path) {
    return QDir::cleanPath(path);
}

bool PathUtils::isPathWithinBase(const QString& path, const QString& baseDir) {
    QString normalizedPath = QDir::cleanPath(path);
    QString normalizedBase = QDir::cleanPath(baseDir);

    // Check if path starts with base directory
    return normalizedPath.startsWith(normalizedBase + "/") || normalizedPath == normalizedBase;
}

QString PathUtils::getSandboxPath(const QString& cartridgeGuid, const QString& appId) {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString sandboxPath = QString("%1/sandbox/%2/%3").arg(dataDir, cartridgeGuid, appId);
    QDir().mkpath(sandboxPath);
    return sandboxPath;
}

} // namespace utils
} // namespace common
} // namespace smartbook
