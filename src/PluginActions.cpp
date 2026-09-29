#include "PluginActions.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <algorithm>
#include <format>
#include <uibase/imoinfo.h>
#include <uibase/ipluginlist.h>

namespace Bench {
Json Plugins(const MOBase::IOrganizer* a_organizer, const Json& a_args) {
    auto* list = a_organizer->pluginList();
    auto names = list->pluginNames();
    const auto action = a_args.value("action", "list");
    if (action == "setLoadOrder") {
        const auto order = StringList(a_args.at("names"));
        auto expected = names;
        auto actual = order;
        expected.sort();
        actual.sort();
        if (expected != actual) {
            throw ToolError(
                400,
                "names must contain every plugin exactly once, including disabled plugins. Use plugins action=list"
            );
        }
        list->setLoadOrder(order);
    } else if (action != "list") {
        const auto name = Text(a_args.at("name"));
        if (!names.contains(name)) {
            throw ToolError(404, std::format("Unknown plugin: {}. Use plugins action=list", Utf8(name)));
        }
        if (action == "setEnabled") {
            list->setState(
                name,
                a_args.at("enabled").get<bool>() ? MOBase::IPluginList::STATE_ACTIVE
                                                 : MOBase::IPluginList::STATE_INACTIVE
            );
        } else if (action == "setPriority") {
            if (!list->setPriority(name, a_args.at("priority").get<int>())) {
                throw ToolError(409, "MO2 rejected plugin priority");
            }
        } else {
            throw ToolError(400, std::format("Unknown plugins action: {}", action));
        }
    }
    std::ranges::sort(names, [list](const auto& a_left, const auto& a_right) {
        return list->priority(a_left) < list->priority(a_right);
    });
    Json result = Json::array();
    for (const auto& name : names) {
        result.push_back({
            {"name", Utf8(name)},
            {"enabled", list->state(name) == MOBase::IPluginList::STATE_ACTIVE},
            {"state", static_cast<int>(list->state(name))},
            {"priority", list->priority(name)},
            {"loadOrder", list->loadOrder(name)},
            {"origin", Utf8(list->origin(name))},
            {"masters", Strings(list->masters(name))},
            {"master", list->isMasterFlagged(name)},
            {"light", list->isLightFlagged(name)},
            {"medium", list->isMediumFlagged(name)},
            {"noRecords", list->hasNoRecords(name)},
        });
    }
    return {{"plugins", result}};
}
}
