#include "Conflicts/Inspection.h"
#include "Conflicts/Model.h"
#include "Json.h"
#include "Mods/ContextMenu.h"
#include "Native/DialogSnapshot.h"
#include "Tools/Registry.h"
#include <QDialog>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <uibase/imodinterface.h>
#include <uibase/imodlist.h>
#include <uibase/imoinfo.h>
#include <utility>

namespace Bench {
namespace {
    Json SnapshotConflicts(const Json& a_arguments) {
        Json initialRequest = {{"view", a_arguments.value("view", "advanced")}, {"limit", 0}};
        const auto initial = ReadConflicts(initialRequest);
        Json request = a_arguments;
        request["filter"] = a_arguments.value("filter", "");
        request["options"] = a_arguments.value("options", Json {{"allProviders", true}, {"includeUnique", false}});
        auto result = ReadConflicts(request);

        initialRequest["filter"] = initial.at("filter");
        initialRequest["options"] = initial.at("options");
        ConflictView(initialRequest);
        return result;
    }

    QStringList RequestedMods(const MOBase::IModList* a_list, const Json& a_arguments) {
        const auto names = a_arguments.contains("mods") ? StringList(a_arguments.at("mods"))
                                                        : a_list->allModsByProfilePriority();
        const auto filter = Text(a_arguments.value("modFilter", Json("")));
        QStringList result;
        for (const auto& name : names) {
            const auto* mod = a_list->getMod(name);
            if (mod == nullptr) {
                throw ToolError(404, "Unknown mod: " + Utf8(name));
            }
            if (!mod->isSeparator() && name.contains(filter, Qt::CaseInsensitive) && !result.contains(name)) {
                result.push_back(name);
            }
        }
        return result;
    }
}

Json InspectModConflicts(const MOBase::IOrganizer* a_organizer, const Json& a_arguments) {
    const auto* list = a_organizer->modList();
    const auto names = RequestedMods(list, a_arguments);
    const auto offset = a_arguments.value("modOffset", 0);
    const auto limit = a_arguments.value("modLimit", 10);
    const auto total = static_cast<int>(names.size());
    const auto end = offset >= total ? total : offset + std::min(limit, total - offset);
    Json mods = Json::array();
    for (int index = offset; index < end; ++index) {
        const auto& name = names[index];
        try {
            auto snapshot = SnapshotNativeDialog(
                "ModInfoDialog",
                [list, name] { InvokeModMenu(list, name, "Information..."); },
                [&a_arguments](QDialog*) { return SnapshotConflicts(a_arguments); }
            );
            for (auto& file : snapshot.at("files")) {
                file["providers"] = Strings(a_organizer->getFileOrigins(Text(file.at("path"))));
            }
            mods.push_back(std::move(snapshot));
        } catch (const ToolError& error) {
            mods.push_back({{"name", Utf8(name)}, {"error", error.what()}});
        }
    }
    return {
        {"mods", mods},
        {"totalMods", total},
        {"modOffset", offset},
        {"nextModOffset", end < total ? Json(end) : Json(nullptr)},
    };
}
}
