#include "EventBus.h"
#include "Json.h"
#include "Tools/Registry.h"
#include "Transport/Server.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <httplib.h>

namespace Bench {
namespace {
    void Reply(httplib::Response& a_response, int a_status, const Json& a_body) {
        a_response.status = a_status;
        a_response.set_content(a_body.dump(), "application/json");
    }

    void StreamEvents(EventBus& a_events, httplib::Response& a_response, std::uint64_t a_since) {
        a_response.set_chunked_content_provider(
            "text/event-stream",
            [&a_events, since = a_since](size_t, httplib::DataSink& a_sink) mutable {
                const auto batch = a_events.Wait(since, 1000);
                if (batch["stopped"].get<bool>()) {
                    a_sink.done();
                    return false;
                }
                since = batch["headSeq"].get<std::uint64_t>();
                const auto message = "data: " + batch.dump() + "\n\n";
                return a_sink.write(message.data(), message.size());
            }
        );
    }
}

void Server::MountRest() {
    auto* http = _server->http();
    http->Get("/api/health", [this](const auto&, auto& a_response) {
        auto data = _runtime.Identity();
        data.update(_main.Health());
        data["ok"] = true;
        Reply(a_response, 200, data);
    });
    http->Get("/api/tools", [this](const auto&, auto& a_response) {
        Reply(a_response, 200, {{"tools", _registry.List()}, {"mcp_bridge", _runtime.Bridge()}});
    });
    http->Post(R"(/api/tool/([^/]+))", [this](const auto& a_request, auto& a_response) {
        const auto session = a_request.get_header_value("X-Dev-Bench-Session");
        if (!session.empty() && session != _runtime.Identity()["session"].template get<std::string>()) {
            Reply(a_response, 409, {{"error", "Process session changed. Rediscover before calling"}});
            return;
        }
        try {
            const auto arguments = a_request.body.empty() ? Json::object() : Json::parse(a_request.body);
            Reply(a_response, 200, _registry.Invoke(a_request.matches[1], arguments));
        } catch (const ToolError& error) {
            Reply(a_response, error.Code(), {{"error", error.what()}, {"code", error.Code()}});
        } catch (const Json::exception& error) {
            Reply(a_response, 400, {{"error", error.what()}, {"code", 400}});
        } catch (const std::exception& error) {
            Reply(a_response, 500, {{"error", error.what()}, {"code", 500}});
        }
    });
    http->Get("/api/events", [this](const auto& a_request, auto& a_response) {
        try {
            const auto since = a_request.has_param("since") ? std::stoull(a_request.get_param_value("since")) : 0;
            if (a_request.get_header_value("Accept").find("text/event-stream") != std::string::npos) {
                StreamEvents(_events, a_response, since);
            } else {
                Reply(a_response, 200, _events.Since(since));
            }
        } catch (const ToolError& error) {
            Reply(a_response, error.Code(), {{"error", error.what()}, {"code", error.Code()}});
        } catch (const std::exception& error) {
            Reply(a_response, 400, {{"error", error.what()}});
        }
    });
}
}
