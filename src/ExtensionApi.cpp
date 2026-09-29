#include "ExtensionApi.h"
#include "DevBenchAPI.h"
#include "EventBus.h"
#include "Json.h"
#include "Tools/Extensions.h"
#include "Tools/Registry.h"
#include <mutex>
#include <string>
#include <utility>

namespace {
struct Services {
    std::mutex mutex;
    Bench::ToolRegistry* registry = nullptr;
    Bench::EventBus* events = nullptr;
    Bench::ExtensionDispatch dispatch;
};

// The interface passes no host context, so extensions reach the host through process-wide state.
Services& Current() {
    static Services services;
    return services;
}

Bench::Json ParseObject(const char* a_text) {
    if (a_text == nullptr) {
        return Bench::Json::object();
    }
    auto value = Bench::Json::parse(a_text, nullptr, false);
    return value.is_object() ? std::move(value) : Bench::Json::object();
}

Bench::Json ParsePayload(const char* a_text) {
    if (a_text == nullptr) {
        return Bench::Json::object();
    }
    auto value = Bench::Json::parse(a_text, nullptr, false);
    return value.is_discarded() ? Bench::Json::object() : std::move(value);
}

Bench::Json Descriptor(const std::string& a_name, const char* a_descriptorJson) {
    const auto descriptor = ParseObject(a_descriptorJson);
    return {
        {"name", a_name},
        {"description", descriptor.value("description", std::string())},
        {"inputSchema", descriptor.value("inputSchema", Bench::Json {{"type", "object"}})},
        {"readOnly", descriptor.value("readOnly", false)},
    };
}

Bench::Json Invoke(DevBenchAPI::ToolFn a_handler, void* a_ctx, const std::string& a_arguments) {
    std::string result;
    const DevBenchAPI::WriteFn write = +[](void* a_sink, const char* a_json) {
        *static_cast<std::string*>(a_sink) = a_json != nullptr ? a_json : "";
    };
    a_handler(a_ctx, a_arguments.c_str(), &result, write);
    if (result.empty()) {
        return Bench::Json::object();
    }
    auto parsed = Bench::Json::parse(result, nullptr, false);
    return parsed.is_discarded() ? Bench::Json {{"raw", result}} : std::move(parsed);
}

Bench::ToolRegistry::Handler MakeHandler(
    DevBenchAPI::ToolFn a_handler,
    void* a_ctx,
    Bench::ExtensionDispatch a_dispatch
) {
    return [a_handler, a_ctx, dispatch = std::move(a_dispatch)](const Bench::Json& a_arguments) {
        // The dispatch can time out while the handler still runs, so the handler owns its input.
        return dispatch([a_handler, a_ctx, arguments = a_arguments.dump()] {
            return Invoke(a_handler, a_ctx, arguments);
        });
    };
}

struct Interface final : DevBenchAPI::IDevBenchInterface001 {
    unsigned int GetBuildNumber() override {
        return (MOPK_VERSION_MAJOR * 10000U) + (MOPK_VERSION_MINOR * 100U) + MOPK_VERSION_PATCH;
    }

    bool RegisterTool(
        const char* a_name,
        const char* a_descriptorJson,
        DevBenchAPI::ToolFn a_handler,
        void* a_ctx
    ) override {
        if ((a_name == nullptr) || ((*a_name) == 0) || (a_handler == nullptr)) {
            return false;
        }
        auto& services = Current();
        const std::scoped_lock lock(services.mutex);
        if (services.registry == nullptr) {
            return false;
        }
        try {
            return services.registry->Add(
                Descriptor(a_name, a_descriptorJson),
                MakeHandler(a_handler, a_ctx, services.dispatch)
            );
        } catch (...) {
            return false;
        }
    }

    void EmitEvent(const char* a_topic, const char* a_payloadJson) override {
        if (a_topic == nullptr) {
            return;
        }
        auto& services = Current();
        const std::scoped_lock lock(services.mutex);
        if (services.events != nullptr) {
            services.events->Publish(a_topic, ParsePayload(a_payloadJson));
        }
    }

    bool RegisterMenuHandler(
        const char* a_menuName,
        const char* a_descriptorJson,
        DevBenchAPI::ToolFn a_handler,
        void* a_ctx
    ) override {
        return RegisterToolExtension("menu", a_menuName, a_descriptorJson, a_handler, a_ctx);
    }

    bool RegisterToolExtension(
        const char* a_baseTool,
        const char* a_key,
        const char* a_descriptorJson,
        DevBenchAPI::ToolFn a_handler,
        void* a_ctx
    ) override {
        if ((a_baseTool == nullptr)
            || ((*a_baseTool) == 0)
            || (a_key == nullptr)
            || ((*a_key) == 0)
            || (a_handler == nullptr)) {
            return false;
        }
        Bench::ExtensionDispatch dispatch;
        {
            auto& services = Current();
            const std::scoped_lock lock(services.mutex);
            if (services.registry == nullptr) {
                return false;
            }
            dispatch = services.dispatch;
        }
        try {
            // The change listener refreshes the base tool's descriptor, so no services lock is held here.
            return Bench::ToolExtensions::Register(
                a_baseTool,
                a_key,
                ParseObject(a_descriptorJson),
                MakeHandler(a_handler, a_ctx, std::move(dispatch))
            );
        } catch (...) {
            return false;
        }
    }
};

Interface& GetInterface() {
    static Interface interface;
    return interface;
}
}

namespace Bench {
void SetExtensionServices(ToolRegistry* a_registry, EventBus* a_events, ExtensionDispatch a_dispatch) {
    auto& services = Current();
    const std::scoped_lock lock(services.mutex);
    services.registry = a_registry;
    services.events = a_events;
    services.dispatch = std::move(a_dispatch);
}
}

extern "C" __declspec(dllexport) void* DevBenchQueryInterface(unsigned int a_revision) {
    return a_revision == 1 ? static_cast<DevBenchAPI::IDevBenchInterface001*>(&GetInterface()) : nullptr;
}
