#pragma once
#include "Capture/Artifacts.h"
#include "Json.h"

namespace Bench {
class EventBus;
class MainThread;
}

namespace Bench::Capture {
// Routes the capture tool by kind. Runs on a listener worker: native captures queue their window access
// on the UI thread and providers queue their own handlers there.
class Tool final {
public:
    Tool(Settings a_settings, EventBus& a_events, MainThread& a_main);
    [[nodiscard]] Json Invoke(const Json& a_arguments) const;

private:
    Settings _settings;
    EventBus* _events;
    MainThread* _main;
};
}
