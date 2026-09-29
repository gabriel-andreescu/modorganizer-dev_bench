#include "Mods/ContextMenu.h"
#include "Json.h"
#include "Mods/Actions.h"
#include "Mods/ListModel.h"
#include "Native/Menu.h"
#include "Tools/Registry.h"
#include <QAbstractProxyModel>
#include <QApplication>
#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QMetaObject>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QTreeView>
#include <qnamespace.h>
#include <ranges>
#include <vector>

namespace Bench {
namespace {
    QModelIndex ViewIndex(QAbstractItemModel* a_model, QModelIndex a_index) {
        std::vector<QAbstractProxyModel*> proxies;
        while (auto* proxy = qobject_cast<QAbstractProxyModel*>(a_model)) {
            proxies.push_back(proxy);
            a_model = proxy->sourceModel();
        }
        for (const auto* proxy : proxies | std::views::reverse) {
            a_index = proxy->mapFromSource(a_index);
        }
        return a_index;
    }
}

void InvokeModMenu(const MOBase::IModList* a_list, const QString& a_name, const char* a_action) {
    if (QApplication::activeModalWidget() != nullptr) {
        throw ToolError(409, "Answer the current modal dialog first");
    }
    const auto item = FindModListItem(a_list, a_name);
    auto* view = item.view;
    const auto index = ViewIndex(view->model(), item.index);
    if (!index.isValid()) {
        throw ToolError(409, "Mod is excluded by the current list filters. Clear those filters first");
    }
    for (auto parent = index.parent(); parent.isValid(); parent = parent.parent()) {
        view->expand(parent);
    }
    view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    view->scrollTo(index);
    InvokeNativeMenu("ModListContextMenu", QCoreApplication::translate("ModListContextMenu", a_action), [view, index] {
        QMetaObject::invokeMethod(
            view,
            "customContextMenuRequested",
            Qt::DirectConnection,
            Q_ARG(QPoint, view->visualRect(index).center())
        );
    });
}

Json ModActions::MenuOperation(const Json& a_arguments) {
    const auto name = Text(a_arguments.at("name"));
    const auto& action = a_arguments.at("action");
    const char* menuAction = "Information...";
    if (action == "setIgnoreUpdate") {
        menuAction = a_arguments.at("enabled").get<bool>() ? "Ignore update" : "Un-ignore update";
    }
    return Start(action, [this, name, menuAction] {
        InvokeModMenu(_organizer->modList(), name, menuAction);
        return Describe(name);
    });
}

}
