#pragma once
#include "Json.h"
#include "Tools/Registry.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Keyed extensions registered by other plugins. Base tools that opt in route requests to them by key.
namespace Bench::ToolExtensions {
struct Entry {
    Json descriptor;
    ToolRegistry::Handler handler;
};

// Returns false when the registration replaced an existing entry.
bool Register(const std::string& a_baseTool, std::string a_key, Json a_descriptor, ToolRegistry::Handler a_handler);
using ChangeListener = std::function<void(const std::string& a_baseTool)>;
void SetChangeListener(ChangeListener a_listener);
std::optional<Entry> Find(std::string_view a_baseTool, std::string_view a_key);
std::vector<std::string> Keys(std::string_view a_baseTool);
}
