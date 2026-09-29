#include "Dialogs/Choices.h"
#include "Json.h"
#include <QString>
#include <cstddef>
#include <cstdint>
#include <map>
#include <qnamespace.h>

#include <ranges>
namespace Bench {
Json DialogChoices(const Json& a_dialog, const Json& a_args) {
    if (!a_dialog.value("open", false)) {
        return a_dialog;
    }
    auto result = a_dialog;
    result.erase("controls");
    result["groups"] = Json::array();
    result["navigation"] = Json::array();
    result["optionCount"] = 0;
    std::map<std::uint64_t, Json> groups;
    const auto filter = Text(a_args.value("filter", ""));
    for (const auto& control : a_dialog["controls"]) {
        if (control["class"] == "QGroupBox") {
            auto group = control;
            group["options"] = Json::array();
            group["optionCount"] = 0;
            groups[control["id"].get<std::uint64_t>()] = group;
        }
    }

    std::size_t count = 0;
    for (const auto& control : a_dialog["controls"]) {
        if (control["class"] == "QPushButton") {
            result["navigation"].push_back(control);
        }
        if (!control.value("checkable", false)) {
            continue;
        }
        ++count;
        auto& group = groups[control.at("parent").get<std::uint64_t>()];
        if (!group.contains("options")) {
            group = {{"id", control["parent"]}, {"options", Json::array()}, {"optionCount", 0}};
        }
        group["optionCount"] = group["optionCount"].get<int>() + 1;
        const auto searchable = Text(control.value("text", ""))
                                + " "
                                + Text(control.value("description", ""))
                                + " "
                                + Text(group.value("text", ""));
        if (filter.isEmpty() || searchable.contains(filter, Qt::CaseInsensitive)) {
            group["options"].push_back(control);
        }
    }
    result["optionCount"] = count;
    for (const auto& group : groups | std::views::values) {
        if (!group["options"].empty()) {
            result["groups"].push_back(group);
        }
    }
    return result;
}
}
