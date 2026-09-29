#pragma once
#include "EventBus.h"
#include "MainThread.h"
#include "Tools/Registry.h"
#include "Transport/Runtime.h"
#include <mcp_server.h>
#include <thread>

namespace Bench {
class Server {
public:
    Server(ToolRegistry& a_registry, EventBus& a_events, Runtime& a_runtime, MainThread& a_main);
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;
    ~Server();
    void Start(int a_port);
    void Stop();

private:
    void MountRest();
    void MountMcp();
    ToolRegistry& _registry;
    EventBus& _events;
    Runtime& _runtime;
    MainThread& _main;
    std::unique_ptr<mcp::server> _server;
    std::jthread _notifications;
};
}
