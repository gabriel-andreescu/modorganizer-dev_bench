#include "Mods/ListModel.h"
#include "Tools/Registry.h"
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QObject>
#include <QTreeView>
#include <QWidget>
#include <format>
#include <uibase/imodlist.h>

namespace Bench {
ModListItem FindModListItem(const MOBase::IModList* a_list, const QString& a_name) {
    for (auto* widget : QApplication::allWidgets()) {
        auto* view = qobject_cast<QTreeView*>(widget);
        if ((view == nullptr) || view->objectName() != "modList") {
            continue;
        }
        auto* model = view->model();
        while (const auto* proxy = qobject_cast<QAbstractProxyModel*>(model)) {
            model = proxy->sourceModel();
        }
        // Both supported releases expose allMods in the source model's row order.
        const auto row = a_list->allMods().indexOf(a_name);
        if (row < 0) {
            throw ToolError(404, std::format("Unknown mod: {}", a_name.toStdString()));
        }
        return {.model = model, .index = model->index(static_cast<int>(row), 0), .view = view};
    }
    throw ToolError(409, "Mod list is not ready");
}
}
