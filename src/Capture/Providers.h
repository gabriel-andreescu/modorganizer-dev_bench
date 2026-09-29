#pragma once
#include "Capture/Artifacts.h"
#include "Json.h"
#include <string>

namespace Bench {
class EventBus;
}

namespace Bench::Capture {
// Asks the provider to write a PNG to a temporary outputPath, waits for it, then publishes it to the
// capture directory. Must not run on MO2's UI thread, where provider handlers are queued.
Json DispatchToProvider(
    const std::string& a_providerKey,
    const Json& a_arguments,
    const Settings& a_settings,
    EventBus* a_events
);
}
