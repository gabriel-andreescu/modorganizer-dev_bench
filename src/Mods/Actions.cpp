#include "Mods/Actions.h"
#include "Json.h"
#include "Mods/ConflictFlags.h"
#include "Mods/Editor.h"
#include "Mods/Filters.h"
#include "Mods/ListModel.h"
#include "Tools/Registry.h"
#include <QColor>
#include <algorithm>
#include <format>
#include <tuple>
#include <uibase/guessedvalue.h>
#include <uibase/imodinterface.h>
#include <utility>

namespace Bench {
MOBase::IModInterface* ModActions::Require(const QString& a_name) const {
    auto* mod = _organizer->modList()->getMod(a_name);
    if (mod == nullptr) {
        throw ToolError(404, std::format("Unknown mod: {}. Use mods action=list", Utf8(a_name)));
    }
    return mod;
}

Json ModActions::Describe(const QString& a_name) const {
    const auto* mod = Require(a_name);
    const auto* list = _organizer->modList();
    Json files = Json::array();
    for (const auto& [modId, fileId] : mod->installedFiles()) {
        files.push_back({{"modId", modId}, {"fileId", fileId}});
    }
    return {
        {"name", Utf8(a_name)},
        {"displayName", Utf8(list->displayName(a_name))},
        {"path", Utf8(mod->absolutePath())},
        {"enabled", list->state(a_name).testFlag(MOBase::IModList::STATE_ACTIVE)},
        {"state", static_cast<int>(list->state(a_name))},
        {"conflicts", ReadModConflictFlags(FindModListItem(list, a_name).index)},
        {"priority", list->priority(a_name)},
        {"separator", mod->isSeparator()},
        {"overwrite", mod->isOverwrite()},
        {"foreign", mod->isForeign()},
        {"backup", mod->isBackup()},
        {"comments", Utf8(mod->comments())},
        {"notes", Utf8(mod->notes())},
        {"game", Utf8(mod->gameName())},
        {"repository", Utf8(mod->repository())},
        {"nexusId", mod->nexusId()},
        {"version", Utf8(mod->version().canonicalString())},
        {"newestVersion", Utf8(mod->newestVersion().canonicalString())},
        {"ignoredVersion", Utf8(mod->ignoredVersion().canonicalString())},
        {"installationFile", Utf8(mod->installationFile())},
        {"installedFiles", files},
        {"categories", Strings(mod->categories())},
        {"primaryCategory", mod->primaryCategory()},
        {"url", Utf8(mod->url())},
        {"color", Utf8(mod->color().name(QColor::HexArgb))},
        {"validated", mod->validated()},
        {"converted", mod->converted()},
        {"tracked", static_cast<int>(mod->trackedState())},
        {"endorsed", static_cast<int>(mod->endorsedState())},
    };
}

Json ModActions::List(bool a_separatorsOnly) const {
    Json mods = Json::array();
    QString separator;
    for (const auto& name : _organizer->modList()->allModsByProfilePriority()) {
        auto data = Describe(name);
        if (data["separator"] == true) {
            separator = name;
        } else if (a_separatorsOnly) {
            continue;
        }
        data["parentSeparator"] = Utf8(separator);
        mods.push_back(std::move(data));
    }
    return {{"mods", mods}};
}

void ModActions::Move(const QString& a_name, int a_priority) const {
    std::ignore = Require(a_name);
    auto* list = _organizer->modList();
    if (list->priority(a_name) == a_priority) {
        return;
    }
    if (!list->setPriority(a_name, a_priority)) {
        throw ToolError(409, "MO2 rejected mod priority");
    }
    _organizer->refresh();
}

void ModActions::UnderSeparator(const QString& a_name, const QString& a_separator) const {
    if (!Require(a_separator)->isSeparator()) {
        throw ToolError(400, "Target is not a separator");
    }
    if (a_name == a_separator) {
        throw ToolError(400, "A separator cannot contain itself");
    }
    const auto* list = _organizer->modList();
    int target = list->priority(a_separator) + 1;
    if (list->priority(a_name) < target) {
        --target;
    }
    Move(a_name, target);
}

Json ModActions::Create(const QString& a_requested, bool a_separator) {
    auto name = a_requested;
    if (a_separator && !name.endsWith("_separator")) {
        name += "_separator";
    }
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains('\\')) {
        throw ToolError(400, "Use a nonempty mod name, without path components");
    }
    if (_organizer->modList()->getMod(name) != nullptr) {
        throw ToolError(409, "Mod already exists");
    }
    MOBase::GuessedValue<QString> guessed(name);
    auto* mod = _organizer->createMod(guessed);
    if (mod == nullptr) {
        throw ToolError(409, "MO2 did not create the mod");
    }
    const auto createdName = mod->name();
    const auto createdPath = mod->absolutePath();
    _organizer->modDataChanged(mod);
    return {{"name", Utf8(createdName)}, {"path", Utf8(createdPath)}, {"refreshPending", true}};
}

Json ModActions::Invoke(const Json& a_args) {
    const auto action = a_args.value("action", "list");
    if (action == "operationStatus") {
        return Status();
    }
    if (action == "editorState" || action == "editorUpdate" || action == "closeEditor") {
        return ModEditor(a_args);
    }
    if (action == "filters" || action == "setFilters" || action == "clearFilters") {
        return ModFilters(a_args);
    }
    if (action == "list") {
        auto result = List(false);
        if (a_args.contains("conflict")) {
            auto& mods = result.at("mods");
            std::erase_if(mods.get_ref<Json::array_t&>(), [&a_args](const Json& a_mod) {
                const auto& flags = a_mod.at("conflicts").at("flags");
                return std::ranges::find(flags, a_args.at("conflict")) == flags.end();
            });
        }
        return result;
    }
    if (action == "separators") {
        return List(true);
    }
    if (action == "create" || action == "createSeparator") {
        return Create(Text(a_args.at("name")), action == "createSeparator");
    }
    if (action == "metadata") {
        return Metadata(a_args);
    }
    return InvokeMod(a_args);
}

Json ModActions::InvokeMod(const Json& a_args) {
    const auto action = a_args.at("action").get<std::string>();
    const auto name = Text(a_args.at("name"));
    auto* mod = Require(name);
    auto* list = _organizer->modList();
    if (action == "manage" || action == "setIgnoreUpdate") {
        return MenuOperation(a_args);
    }
    if (action == "get") {
        return Describe(name);
    }
    if (action == "setEnabled") {
        if (!list->setActive(name, a_args.at("enabled").get<bool>())) {
            throw ToolError(409, "MO2 rejected mod state change");
        }
    } else if (action == "setPriority") {
        Move(name, a_args.at("priority").get<int>());
    } else if (action == "move") {
        UnderSeparator(name, Text(a_args.at("separator")));
    } else if (action == "rename") {
        const auto* renamed = list->renameMod(mod, Text(a_args.at("newName")));
        if (renamed == nullptr) {
            throw ToolError(409, "MO2 rejected rename");
        }
        return Describe(renamed->name());
    } else if (action == "remove") {
        return Remove(name);
    } else if (action == "addCategory") {
        mod->addCategory(Text(a_args.at("category")));
    } else if (action == "removeCategory") {
        if (!mod->removeCategory(Text(a_args.at("category")))) {
            throw ToolError(404, "Category not assigned");
        }
    } else {
        throw ToolError(400, std::format("Unknown mods action: {}", action));
    }
    return Describe(name);
}
}
