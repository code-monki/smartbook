#include "smartbook/common/utils/PlatformUtils.h"
#include <QStandardPaths>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <TargetConditionals.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#endif

namespace smartbook {
namespace common {
namespace utils {

QString PlatformUtils::getPlatform() {
#ifdef Q_OS_WIN
    return "windows";
#elif defined(Q_OS_MACOS)
    return "macos";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    return "unknown";
#endif
}

QString PlatformUtils::getArchitecture() {
#ifdef Q_PROCESSOR_ARM_64
    return "arm64";
#elif defined(Q_PROCESSOR_X86_64)
    return "x86_64";
#else
    return "unknown";
#endif
}

QString PlatformUtils::getApplicationDataDirectory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    return path;
}

QString PlatformUtils::getCacheDirectory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(path);
    return path;
}

QString PlatformUtils::getLogDirectory() {
    QString path = getApplicationDataDirectory() + "/logs";
    QDir().mkpath(path);
    return path;
}

QString PlatformUtils::getBackupDirectory() {
    QString path = getApplicationDataDirectory() + "/backups";
    QDir().mkpath(path);
    return path;
}

} // namespace utils
} // namespace common
} // namespace smartbook
