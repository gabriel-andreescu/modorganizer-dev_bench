#include "Transport/Server.h"
#include "EventBus.h"
#include "Json.h"
#include "MainThread.h"
#include "Tools/Registry.h"
#include "Transport/Runtime.h"
#include <cstdint>
#include <httplib.h>
#include <mcp_message.h>
#include <mcp_server.h>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <winsock2.h>

namespace Bench {
Server::Server(ToolRegistry& a_registry, EventBus& a_events, Runtime& a_runtime, MainThread& a_main)
    : _registry(a_registry)
    , _events(a_events)
    , _runtime(a_runtime)
    , _main(a_main) {}

Server::~Server() {
    Stop();
}

void Server::Start(int a_port) {
    mcp::server::configuration config;
    config.host = "127.0.0.1";
    config.port = a_port;
    config.name = "modorganizer-dev_bench";
    config.version = "0.1.1";
    config.max_sessions = 32;
    config.session_timeout = 300;
    int bound = a_port;
    if (bound < 1 || bound > 65535) {
        throw std::runtime_error("Port must be between 1 and 65535");
    }
    while (bound < 65536) {
        config.port = bound;
        _server = std::make_unique<mcp::server>(config);
        _server->http()->set_socket_options([](socket_t a_socket) {
            const int exclusive = 1;
            if (setsockopt(
                    a_socket,
                    SOL_SOCKET,
                    SO_EXCLUSIVEADDRUSE,
                    reinterpret_cast<const char*>(&exclusive),
                    sizeof(exclusive)
                )
                != 0) {
                throw std::runtime_error("Cannot reserve an exclusive loopback socket");
            }
        });
        if (_server->http()->bind_to_port("127.0.0.1", bound)) {
            break;
        }
        ++bound;
    }
    if (bound == 65536) {
        throw std::runtime_error("No loopback port available");
    }
    _server->set_capabilities({{"tools", {{"listChanged", true}}}, {"logging", Json::object()}});
    MountMcp();
    MountRest();
    _registry.OnChanged([this](const Json& a_tool) {
        _server->broadcast_notification(
            mcp::request::create_notification("notifications/tools/list_changed", Json::object())
        );
        _events.Publish("tools.changed", {{"name", a_tool.at("name")}});
    });
    _runtime.Publish(bound);
    if (!_server->start(false)) {
        throw std::runtime_error("Failed to start MCP/HTTP listener");
    }
    _notifications = std::jthread([this](const std::stop_token& a_stop) {
        std::uint64_t sequence = 0;
        while (!a_stop.stop_requested()) {
            const auto batch = _events.Wait(sequence, 1000);
            sequence = batch["headSeq"];
            for (const auto& event : batch["events"]) {
                _server->broadcast_notification(
                    mcp::request::create_notification(
                        "notifications/message",
                        {{"level", "info"}, {"logger", "modorganizer-dev_bench"}, {"data", event}}
                    )
                );
            }
        }
    });
}

void Server::Stop() {
    if (!_server) {
        return;
    }
    _registry.OnChanged({});
    _main.Stop();
    _notifications.request_stop();
    _events.Stop();
    if (_notifications.joinable()) {
        _notifications.join();
    }
    _runtime.Remove();
    _server->stop();
    _server.reset();
}
}
