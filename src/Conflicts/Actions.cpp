#include "Conflicts/Actions.h"
#include "Conflicts/Inspection.h"
#include "Conflicts/Model.h"
#include "Json.h"
#include "Mods/FileTree.h"
#include "Mods/Filters.h"
#include "Native/TreeMenu.h"
#include "Tools/Registry.h"
#include <QCoreApplication>
#include <format>

namespace Bench {
Json ConflictActions::Invoke(const Json& a_arguments) {
    const auto action = a_arguments.value("action", "inspect");
    if (action == "operationStatus") {
        return Status();
    }
    if (action == "inspect") {
        return Start(action, [this, arguments = Json(a_arguments)] {
            return WithUnfilteredModList([this, &arguments] { return InspectModConflicts(_organizer, arguments); });
        });
    }
    if (action == "list") {
        return ReadConflicts(a_arguments);
    }
    if (action == "directory") {
        return ReadModDirectory(a_arguments);
    }
    if (action == "hideFiles" || action == "unhide") {
        return Start(action, [arguments = Json(a_arguments)] { return ModFileVisibility(arguments); });
    }
    if (action != "hide" && action != "preview") {
        throw ToolError(400, std::format("Unknown conflicts action: {}", action));
    }
    return Start(action, [arguments = Json(a_arguments), hide = action == "hide"] {
        auto* tree = ConflictView(arguments);
        SelectConflictFiles(tree, arguments.at("paths"));
        const auto label = QCoreApplication::translate("ConflictsTab", hide ? "&Hide" : "&Preview");
        InvokeTreeMenu(tree, label);
        return ReadConflicts(arguments);
    });
}
}
