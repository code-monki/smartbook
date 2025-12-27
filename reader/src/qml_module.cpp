#include "smartbook/reader/QmlAppBridge.h"
#include <QtQml>

namespace smartbook {
namespace reader {

/**
 * @brief Register QML types for SmartBook embedded applications
 * 
 * Registers QmlAppBridge as "SmartbookBridge" in QML module "SmartBook 1.0"
 * This allows QML embedded applications to access the bridge via:
 * 
 * import SmartBook 1.0
 * 
 * Item {
 *     property var bridge: SmartbookBridge
 *     ...
 * }
 */
void registerQmlTypes()
{
    // Register QmlAppBridge as "SmartbookBridge" in QML module "SmartBook 1.0"
    qmlRegisterType<QmlAppBridge>("SmartBook", 1, 0, "SmartbookBridge");
}

} // namespace reader
} // namespace smartbook

