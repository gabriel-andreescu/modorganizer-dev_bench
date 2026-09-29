#pragma once
#include "Capture/Artifacts.h"
#include "Json.h"
#include <QString>
#include <string>

namespace Bench {
class EventBus;
}

namespace Bench::Capture {
// Adds correlation fields, the inconclusive verdict and golden scores, then writes the JSON sidecar and
// publishes capture.saved.
void CompleteCapture(
    Json& a_result,
    const Json& a_arguments,
    const QString& a_path,
    const Settings& a_settings,
    EventBus* a_events
);
void PublishAbandoned(
    EventBus* a_events,
    const std::string& a_requestId,
    const QString& a_path,
    const Json& a_arguments,
    const std::string& a_reason
);
// An MCP result carrying the capture metadata and the image itself.
Json ImageResult(const Json& a_result, const QString& a_path);
}
