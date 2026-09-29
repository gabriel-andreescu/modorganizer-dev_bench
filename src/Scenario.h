#pragma once
#include "EventBus.h"
#include "Tools/Registry.h"

namespace Bench {
Json Scenario(const ToolRegistry& a_registry, EventBus& a_events, const Json& a_args);
}
