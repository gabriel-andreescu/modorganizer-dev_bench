#include "Mods/ConflictFlags.h"
#include "Json.h"
#include <QCoreApplication>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <qnamespace.h>

namespace Bench {
Json ReadModConflictFlags(const QModelIndex& a_index) {
    const auto tooltip = a_index.siblingAtColumn(1).data(Qt::ToolTipRole).toString();
    const auto labels = tooltip.split("<br>", Qt::SkipEmptyParts);
    Json flags = Json::array();
    bool redundant = false;

    for (const auto& [name, label] : kModConflictFlags) {
        if (labels.contains(QCoreApplication::translate("ModList", label))) {
            flags.push_back(name);
            if (QString(name) == "redundant") {
                redundant = true;
            }
        }
    }

    return {{"flags", flags}, {"redundant", redundant}, {"tooltip", Utf8(tooltip)}};
}
}
