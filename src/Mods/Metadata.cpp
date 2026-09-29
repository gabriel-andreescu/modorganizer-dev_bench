#include "Json.h"
#include "Mods/Actions.h"
#include "Mods/ListModel.h"
#include "Tools/Registry.h"
#include <QAbstractItemModel>
#include <qnamespace.h>
#include <uibase/imodinterface.h>

namespace Bench {
namespace {
    void SetComments(const MOBase::IModList* a_list, const QString& a_name, const QString& a_comments) {
        const auto [model, index, view] = FindModListItem(a_list, a_name);
        const auto notes = index.siblingAtColumn(model->columnCount() - 1);
        if (!model->flags(notes).testFlag(Qt::ItemIsEditable)) {
            throw ToolError(409, "Mod comments are not editable");
        }
        if (!model->setData(notes, a_comments, Qt::EditRole)) {
            throw ToolError(409, "MO2 rejected mod comments");
        }
    }
}

Json ModActions::Metadata(const Json& a_arguments) {
    const auto name = Text(a_arguments.at("name"));
    auto* mod = Require(name);
    const auto& values = a_arguments.at("values");
    for (const auto& [key, value] : values.items()) {
        if (key == "comments") {
            SetComments(_organizer->modList(), name, Text(value));
        } else if (key == "version") {
            mod->setVersion(MOBase::VersionInfo(Text(value)));
        } else if (key == "newestVersion") {
            mod->setNewestVersion(MOBase::VersionInfo(Text(value)));
        } else if (key == "nexusId") {
            mod->setNexusID(value.get<int>());
        } else if (key == "game") {
            mod->setGameName(Text(value));
        } else if (key == "url") {
            mod->setUrl(Text(value));
        } else if (key == "installationFile") {
            mod->setInstallationFile(Text(value));
        } else {
            throw ToolError(400, "Unknown writable metadata field: " + key);
        }
    }
    return Describe(name);
}

}
