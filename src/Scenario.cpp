#include "Scenario.h"
#include "EventBus.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <chrono>
#include <cstdint>
#include <exception>
#include <string>

namespace Bench {
namespace {
    Json WaitFor(EventBus& a_events, std::uint64_t& a_sequence, const Json& a_step) {
        const auto topic = a_step.at("waitFor").get<std::string>();
        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(a_step.value("timeoutMs", 30000));
        do {
            const auto batch = a_events.Wait(a_sequence, 250);
            a_sequence = batch["headSeq"];
            for (const auto& event : batch["events"]) {
                if (event["topic"] == topic) {
                    return {{"ok", true}, {"event", event}};
                }
            }
        } while (std::chrono::steady_clock::now() < deadline);
        throw ToolError(504, "Timed out waiting for " + topic);
    }

    Json RunStep(const ToolRegistry& a_registry, EventBus& a_events, std::uint64_t& a_sequence, const Json& a_step) {
        if (!a_step.contains("tool")) {
            return WaitFor(a_events, a_sequence, a_step);
        }
        const auto name = a_step.at("tool").get<std::string>();
        if (name == "scenario") {
            throw ToolError(400, "Nested scenarios are not supported");
        }
        return {{"ok", true}, {"value", a_registry.Invoke(name, a_step.value("args", Json::object()))}};
    }
}

Json Scenario(const ToolRegistry& a_registry, EventBus& a_events, const Json& a_args) {
    Json results = Json::array();
    bool succeeded = true;
    auto sequence = a_events.Since(0)["headSeq"].get<std::uint64_t>();
    for (const auto& step : a_args.at("steps")) {
        const auto start = std::chrono::steady_clock::now();
        Json result;
        try {
            result = RunStep(a_registry, a_events, sequence, step);
        } catch (const std::exception& error) {
            succeeded = false;
            result = {{"ok", false}, {"error", error.what()}};
        }
        result["elapsedMs"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        )
                                  .count();
        results.push_back(result);
        if (result["ok"] == false && !a_args.value("continueOnError", false)) {
            break;
        }
    }
    return {{"ok", succeeded}, {"results", results}};
}
}
