#pragma once
#include "Json.h"
#include <functional>

namespace Bench {
class ToolRegistry;
class EventBus;
// Runs an extension callback on MO2's UI thread and returns its result.
using ExtensionDispatch = std::function<Json(std::function<Json()>)>;
void SetExtensionServices(ToolRegistry* a_registry, EventBus* a_events, ExtensionDispatch a_dispatch);
}
