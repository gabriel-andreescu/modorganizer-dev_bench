#include "Json.h"
#include "Mods/Actions.h"
#include "Mods/ListModel.h"
#include "Tools/Registry.h"
#include <QAbstractItemModel>
#include <QApplication>
#include <QString>

namespace Bench {
Json ModActions::Remove(const QString& a_name) {
    if (QApplication::activeModalWidget() != nullptr) {
        throw ToolError(409, "Answer the current modal dialog first");
    }
    return Start("remove", [this, name = a_name] {
        const auto [model, index, view] = FindModListItem(_organizer->modList(), name);
        if (!model->removeRow(index.row())) {
            throw ToolError(409, "MO2 rejected removal");
        }
        return Json {{"name", Utf8(name)}, {"removed", _organizer->modList()->getMod(name) == nullptr}};
    });
}
}
