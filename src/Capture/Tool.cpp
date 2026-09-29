#include "Capture/Tool.h"
#include "Capture/Native.h"
#include "Capture/Providers.h"
#include "MainThread.h"
#include "Tools/Extensions.h"
#include "Tools/Registry.h"
#include <string>
#include <utility>

namespace Bench::Capture {
namespace {
    Json Providers() {
        Json keys = Json::array();
        for (const auto& key : ToolExtensions::Keys("capture")) {
            keys.push_back(key);
        }
        return {{"providers", keys}};
    }

    Json Extensions() {
        Json extensions = Json::array();
        for (const auto& key : ToolExtensions::Keys("capture")) {
            Json extension {{"kind", key}};
            if (const auto entry = ToolExtensions::Find("capture", key)) {
                extension["descriptor"] = entry->descriptor;
            }
            extensions.push_back(std::move(extension));
        }
        return {{"extensions", extensions}};
    }
}

Tool::Tool(Settings a_settings, EventBus& a_events, MainThread& a_main)
    : _settings(std::move(a_settings))
    , _events(&a_events)
    , _main(&a_main) {}

Json Tool::Invoke(const Json& a_arguments) const {
    const auto kind = a_arguments.value("kind", std::string("auto"));
    if (kind == "native") {
        return Native(a_arguments, _settings, _events, *_main);
    }
    if (kind == "windows") {
        return _main->Run([] { return ListWindows(); });
    }
    if (kind == "providers") {
        return Providers();
    }
    if (kind == "extensions") {
        return Extensions();
    }
    if (kind != "auto") {
        return DispatchToProvider(kind, a_arguments, _settings, _events);
    }

    const auto keys = ToolExtensions::Keys("capture");
    if (keys.size() == 1) {
        return DispatchToProvider(keys.front(), a_arguments, _settings, _events);
    }
    if (keys.empty()) {
        if (a_arguments.value("allowNative", false)) {
            return Native(a_arguments, _settings, _events, *_main);
        }
        throw ToolError(
            404,
            "No capture provider registered. Pass kind='native' or allowNative:true to capture an MO2 window"
        );
    }
    std::string names;
    for (const auto& key : keys) {
        names += (names.empty() ? "" : ", ") + key;
    }
    throw ToolError(400, "Multiple capture providers registered (" + names + "). Pass an explicit kind");
}
}
