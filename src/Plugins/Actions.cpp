#include "Plugins/Actions.h"
#include "Json.h"
#include "MainThread.h"
#include "Plugins/ListModel.h"
#include "Tools/Registry.h"
#include <QAbstractItemModel>
#include <algorithm>
#include <chrono>
#include <format>
#include <numeric>
#include <qnamespace.h>
#include <uibase/imoinfo.h>
#include <uibase/ipluginlist.h>
#include <vector>

namespace Bench {
namespace {
    // MO2 saves the plugin list on a 200 ms coarse timer, accurate to 5%. A refresh that finishes before the save
    // reloads the old list, so changes return after a later UI timer.
    constexpr std::chrono::milliseconds kPluginListSaveDelay {300};

    bool Forced(const QAbstractItemModel* a_model, int a_row) {
        return !a_model->flags(a_model->index(a_row, 0)).testFlag(Qt::ItemIsUserCheckable);
    }

    // IPluginList::setState and setLoadOrder skip that save. The checkbox model path and setPriority schedule it,
    // as MO2's Plugins tab does.
    void SetEnabled(const MOBase::IPluginList* a_list, const QString& a_name, bool a_enabled) {
        auto* model = FindPluginListModel();
        const auto row = static_cast<int>(a_list->pluginNames().indexOf(a_name));
        if (Forced(model, row)) {
            throw ToolError(
                409,
                std::format(
                    "MO2 forces the state of {}. Use plugins action=list to find plugins with forced false",
                    Utf8(a_name)
                )
            );
        }
        model->setData(model->index(row, 0), a_enabled ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }

    void SetPriority(MOBase::IPluginList* a_list, const QString& a_name, int a_priority) {
        const auto count = a_list->pluginNames().size();
        if (a_priority >= count) {
            throw ToolError(
                400,
                std::format("priority must be below {}, the number of plugins. Use plugins action=list", count)
            );
        }
        a_list->setPriority(a_name, a_priority);
    }

    void SetLoadOrder(MOBase::IPluginList* a_list, const QStringList& a_order) {
        auto expected = a_list->pluginNames();
        auto actual = a_order;
        expected.sort();
        actual.sort();
        if (expected != actual) {
            throw ToolError(
                400,
                "names must contain every plugin exactly once, including disabled plugins. Use plugins action=list"
            );
        }
        for (int priority = 0; priority < a_order.size(); ++priority) {
            if (a_list->priority(a_order[priority]) != priority) {
                a_list->setPriority(a_order[priority], priority);
            }
        }
    }

    void Change(MOBase::IPluginList* a_list, const Json& a_args) {
        const auto action = a_args.at("action").get<std::string>();
        if (action == "setLoadOrder") {
            SetLoadOrder(a_list, StringList(a_args.at("names")));
            return;
        }
        const auto name = Text(a_args.at("name"));
        if (!a_list->pluginNames().contains(name)) {
            throw ToolError(404, std::format("Unknown plugin: {}. Use plugins action=list", Utf8(name)));
        }
        if (action == "setEnabled") {
            SetEnabled(a_list, name, a_args.at("enabled").get<bool>());
        } else if (action == "setPriority") {
            SetPriority(a_list, name, a_args.at("priority").get<int>());
        } else {
            throw ToolError(400, std::format("Unknown plugins action: {}", action));
        }
    }

    Json Describe(const MOBase::IPluginList* a_list) {
        const auto* model = FindPluginListModel();
        const auto names = a_list->pluginNames();
        std::vector<int> rows(names.size());
        std::ranges::iota(rows, 0);
        std::ranges::sort(rows, {}, [a_list, &names](int a_row) { return a_list->priority(names[a_row]); });
        Json result = Json::array();
        for (const auto row : rows) {
            const auto& name = names[row];
            result.push_back({
                {"name", Utf8(name)},
                {"enabled", a_list->state(name) == MOBase::IPluginList::STATE_ACTIVE},
                {"forced", Forced(model, row)},
                {"state", static_cast<int>(a_list->state(name))},
                {"priority", a_list->priority(name)},
                {"loadOrder", a_list->loadOrder(name)},
                {"origin", Utf8(a_list->origin(name))},
                {"masters", Strings(a_list->masters(name))},
                {"master", a_list->isMasterFlagged(name)},
                {"light", a_list->isLightFlagged(name)},
                {"medium", a_list->isMediumFlagged(name)},
                {"noRecords", a_list->hasNoRecords(name)},
            });
        }
        return {{"plugins", result}};
    }
}

Json Plugins(const MOBase::IOrganizer* a_organizer, MainThread& a_main, const Json& a_args) {
    if (a_args.value("action", "list") != "list") {
        a_main.Run([a_organizer, arguments = Json(a_args)] {
            Change(a_organizer->pluginList(), arguments);
            return Json();
        });
        a_main.AwaitTimer(kPluginListSaveDelay);
    }
    return a_main.Run([a_organizer] { return Describe(a_organizer->pluginList()); });
}
}
