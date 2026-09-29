#include "Tools/Extensions.h"
#include "DevBenchAPI.h"
#include "EventBus.h"
#include "ExtensionApi.h"
#include "Harness.h"
#include "Tools/Registry.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
extern "C" void* DevBenchQueryInterface(unsigned int a_revision);

namespace {
constexpr auto kDescriptor = R"({"description":"Echo arguments","inputSchema":{"type":"object"}})";

void Echo(void* /*unused*/, const char* a_args, void* a_sink, DevBenchAPI::WriteFn a_write) {
    a_write(a_sink, a_args);
}

void Replacement(void* /*unused*/, const char* /*unused*/, void* a_sink, DevBenchAPI::WriteFn a_write) {
    a_write(a_sink, R"({"replaced":true})");
}

DevBenchAPI::IDevBenchInterface001* Api() {
    return static_cast<DevBenchAPI::IDevBenchInterface001*>(DevBenchQueryInterface(1));
}

int CheckQuery() {
    const auto* api = Api();
    if (api == nullptr || DevBenchQueryInterface(2) != nullptr) {
        return 1;
    }
    const auto build = (MOPK_VERSION_MAJOR * 10000U) + (MOPK_VERSION_MINOR * 100U) + MOPK_VERSION_PATCH;
    return Api()->GetBuildNumber() == build ? 0 : 2;
}

int CheckRegistration(Bench::ToolRegistry& a_registry, Bench::EventBus& a_events) {
    auto* api = Api();
    if (api->RegisterTool("fixture.echo", kDescriptor, Echo, nullptr)) {
        return 3;
    }
    const auto dispatched = std::make_shared<int>(0);
    Bench::SetExtensionServices(&a_registry, &a_events, [dispatched](const std::function<Bench::Json()>& a_work) {
        ++*dispatched;
        return a_work();
    });
    if (!api->RegisterTool("fixture.echo", kDescriptor, Echo, nullptr)
        || a_registry.Invoke("fixture.echo", {{"value", 42}})["value"] != 42
        || *dispatched != 1) {
        return 4;
    }
    if (api->RegisterTool("fixture.echo", kDescriptor, Replacement, nullptr)
        || a_registry.Invoke("fixture.echo", Bench::Json::object())["replaced"] != true) {
        return 5;
    }
    api->EmitEvent("fixture.ready", R"({"ready":true})");
    return a_events.Since(0)["events"][0]["data"]["ready"] == true ? 0 : 6;
}

int CheckExtensions() {
    auto* api = Api();
    std::vector<std::string> changes;
    Bench::ToolExtensions::SetChangeListener([&changes](const std::string& a_baseTool) {
        changes.push_back(a_baseTool);
    });
    if (!api->RegisterToolExtension("capture", "Fixture", kDescriptor, Echo, nullptr)
        || Bench::ToolExtensions::Keys("capture") != std::vector<std::string> {"Fixture"}) {
        return 7;
    }
    const auto entry = Bench::ToolExtensions::Find("CAPTURE", "fixture");
    if (!entry
        || entry->descriptor["description"] != "Echo arguments"
        || entry->handler({{"value", 7}})["value"] != 7) {
        return 8;
    }
    if (api->RegisterToolExtension("capture", "fixture", kDescriptor, Replacement, nullptr)
        || Bench::ToolExtensions::Find("capture", "fixture")->handler(Bench::Json::object())["replaced"] != true) {
        return 9;
    }
    if (!api->RegisterMenuHandler("fixture.menu", kDescriptor, Echo, nullptr)
        || api->RegisterToolExtension("capture", "", kDescriptor, Echo, nullptr)) {
        return 10;
    }
    Bench::ToolExtensions::SetChangeListener({});
    return changes == std::vector<std::string> {"capture", "capture", "menu"} ? 0 : 11;
}
}

int main() {
    return Tests::Run([] {
        Bench::ToolRegistry registry;
        Bench::EventBus events;
        return Tests::First({
            [&] { return CheckQuery(); },
            [&] { return CheckRegistration(registry, events); },
            [&] { return CheckExtensions(); },
            [&] {
                Bench::SetExtensionServices(nullptr, nullptr, {});
                Api()->EmitEvent("fixture.ignored", "{}");
                return events.Since(0)["events"].size() == 1 ? 0 : 12;
            },
        });
    });
}
