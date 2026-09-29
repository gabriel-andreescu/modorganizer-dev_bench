#include "Native/TreeMenu.h"
#include "Native/Menu.h"
#include "Tools/Registry.h"
#include <QItemSelectionModel>
#include <QMetaObject>
#include <QPoint>
#include <QTreeView>
#include <qnamespace.h>

namespace Bench {
void InvokeTreeMenu(QTreeView* a_tree, const QString& a_action) {
    const auto rows = a_tree->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        throw ToolError(400, "Select at least one file");
    }
    const auto index = rows.front();
    for (auto parent = index.parent(); parent.isValid(); parent = parent.parent()) {
        a_tree->expand(parent);
    }
    a_tree->scrollTo(index);
    InvokeNativeMenu("QMenu", a_action, [a_tree, index] {
        QMetaObject::invokeMethod(
            a_tree,
            "customContextMenuRequested",
            Qt::DirectConnection,
            Q_ARG(QPoint, a_tree->visualRect(index).center())
        );
    });
}
}
