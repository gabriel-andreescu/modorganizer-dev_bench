#include "Plugins/ListModel.h"
#include "Tools/Registry.h"
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QTreeView>
#include <QWidget>

namespace Bench {
// Both supported releases order the source model's rows like IPluginList::pluginNames.
QAbstractItemModel* FindPluginListModel() {
    for (auto* widget : QApplication::allWidgets()) {
        const auto* view = qobject_cast<const QTreeView*>(widget);
        if ((view == nullptr) || view->objectName() != "espList") {
            continue;
        }
        auto* model = view->model();
        while (const auto* proxy = qobject_cast<QAbstractProxyModel*>(model)) {
            model = proxy->sourceModel();
        }
        return model;
    }
    throw ToolError(409, "Plugin list is not ready");
}
}
