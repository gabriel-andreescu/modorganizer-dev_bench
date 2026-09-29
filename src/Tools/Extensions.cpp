#include "Tools/Extensions.h"
#include <algorithm>
#include <cctype>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace Bench::ToolExtensions {
namespace {
    std::string Lower(std::string a_value) {
        std::ranges::transform(a_value, a_value.begin(), [](unsigned char a_character) {
            return static_cast<char>(std::tolower(a_character));
        });
        return a_value;
    }

    struct Bucket {
        std::unordered_map<std::string, Entry> byKey;
        std::unordered_map<std::string, std::string> displayKey;
    };

    struct ExtensionState {
        std::mutex mutex;
        std::unordered_map<std::string, Bucket> tools;
        ChangeListener onChange;
    };

    ExtensionState& GetState() {
        static ExtensionState state;
        return state;
    }
}

void SetChangeListener(ChangeListener a_listener) {
    auto& state = GetState();
    const std::scoped_lock lock(state.mutex);
    state.onChange = std::move(a_listener);
}

bool Register(const std::string& a_baseTool, std::string a_key, Json a_descriptor, ToolRegistry::Handler a_handler) {
    const auto tool = Lower(a_baseTool);
    const auto key = Lower(a_key);
    ChangeListener onChange;
    bool replaced = false;
    {
        auto& state = GetState();
        const std::scoped_lock lock(state.mutex);
        auto& bucket = state.tools[tool];
        replaced = bucket.byKey.contains(key);
        bucket.byKey[key] = Entry {.descriptor = std::move(a_descriptor), .handler = std::move(a_handler)};
        bucket.displayKey[key] = std::move(a_key);
        onChange = state.onChange;
    }
    if (onChange) {
        onChange(a_baseTool);
    }
    return !replaced;
}

std::optional<Entry> Find(std::string_view a_baseTool, std::string_view a_key) {
    const auto tool = Lower(std::string(a_baseTool));
    const auto key = Lower(std::string(a_key));
    auto& state = GetState();
    const std::scoped_lock lock(state.mutex);
    const auto toolEntry = state.tools.find(tool);
    if (toolEntry == state.tools.end()) {
        return std::nullopt;
    }
    const auto extension = toolEntry->second.byKey.find(key);
    if (extension == toolEntry->second.byKey.end()) {
        return std::nullopt;
    }
    return extension->second;
}

std::vector<std::string> Keys(std::string_view a_baseTool) {
    const auto tool = Lower(std::string(a_baseTool));
    auto& state = GetState();
    const std::scoped_lock lock(state.mutex);
    std::vector<std::string> keys;
    const auto toolEntry = state.tools.find(tool);
    if (toolEntry != state.tools.end()) {
        keys.reserve(toolEntry->second.displayKey.size());
        for (const auto& [key, display] : toolEntry->second.displayKey) {
            keys.push_back(display);
        }
        std::ranges::sort(keys);
    }
    return keys;
}
}
