#pragma once
#include "Capture/Artifacts.h"
#include "Json.h"

namespace Bench {
class EventBus;
class MainThread;
}

namespace Bench::Capture {
// Captures one MO2 window, the main window unless a window id is given. MO2 windows always include UI.
Json Native(const Json& a_arguments, const Settings& a_settings, EventBus* a_events, MainThread& a_main);
Json ListWindows();
}
