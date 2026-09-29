#pragma once
#include <QModelIndex>
#include <QString>

class QAbstractItemModel;
class QTreeView;

namespace MOBase {
class IModList;
}

namespace Bench {
struct ModListItem {
    QAbstractItemModel* model {};
    QModelIndex index;
    QTreeView* view {};
};

ModListItem FindModListItem(const MOBase::IModList* a_list, const QString& a_name);
}
