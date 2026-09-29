#include "Tools/Registry.h"
#include "Json.h"
#include <format>
#include <functional>
#include <json.hpp>
#include <mutex>
#include <string>
#include <utility>

namespace Bench {
bool ToolRegistry::Add(Json a_descriptor, Handler a_handler) {
    std::function<void(const Json&)> changed;
    bool added = false;
    {
        const std::scoped_lock lock(_mutex);
        const auto name = a_descriptor.at("name").get<std::string>();
        added = !_entries.contains(name);
        _entries[name] = Entry {.descriptor = a_descriptor, .handler = std::move(a_handler)};
        changed = _changed;
    }
    if (changed) {
        changed(a_descriptor);
    }
    return added;
}

Json ToolRegistry::List() const {
    const std::scoped_lock lock(_mutex);
    Json result = Json::array();
    for (const auto& [name, entry] : _entries) {
        result.push_back(entry.descriptor);
    }
    return result;
}

Json ToolRegistry::Invoke(const std::string& a_name, const Json& a_arguments) const {
    Entry entry;
    {
        const std::scoped_lock lock(_mutex);
        const auto found = _entries.find(a_name);
        if (found == _entries.end()) {
            throw ToolError(404, std::format("Unknown tool: {}. List /api/tools", a_name));
        }
        entry = found->second;
    }
    Validate(entry.descriptor.at("inputSchema"), a_arguments);
    try {
        return entry.handler(a_arguments);
    } catch (const nlohmann::json::exception& error) {
        throw ToolError(400, error.what());
    }
}

void ToolRegistry::OnChanged(std::function<void(const Json&)> a_callback) {
    const std::scoped_lock lock(_mutex);
    _changed = std::move(a_callback);
}
}
