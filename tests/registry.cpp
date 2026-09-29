#include "Tools/Registry.h"
#include "EventBus.h"
#include "Harness.h"
#include <future>

int main() {
    using namespace Bench;
    return Tests::Run([] {
        ToolRegistry registry;
        int calls = 0;
        registry.Add(
            {
                {"name", "double"},
                {
                    "inputSchema",
                    {
                        {"type", "object"},
                        {"required", Json::array({"value"})},
                        {"properties", {{"value", {{"type", "integer"}}}}},
                        {"additionalProperties", false},
                    },
                },
            },
            [&calls](const Json& a_args) {
                ++calls;
                return a_args.at("value").get<int>() * 2;
            }
        );
        if (registry.Invoke("double", {{"value", 3}}) != 6) {
            return 1;
        }
        for (const auto& bad : Json::array({Json::object(), {{"value", "3"}}, {{"value", 3}, {"extra", true}}})) {
            bool rejected = false;
            try {
                registry.Invoke("double", bad);
            } catch (const ToolError& error) {
                rejected = error.Code() == 400;
            }
            if (!rejected) {
                return 2;
            }
        }
        if (calls != 1) {
            return 6;
        }
        EventBus events;
        auto waiting = std::async(std::launch::async, [&] { return events.Wait(0, 1000); });
        events.Publish("mods.state", {{"enabled", true}});
        if (waiting.get()["events"].size() != 1) {
            return 3;
        }
        for (int i = 0; i < 1025; ++i) {
            events.Publish("test", i);
        }
        const auto batch = events.Since(0);
        if (batch["gap"] != true || batch["events"].size() != 1024) {
            return 4;
        }
        return 0;
    });
}
