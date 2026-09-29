#include "Capture/Providers.h"
#include "Capture/Completion.h"
#include "EventBus.h"
#include "Tools/Extensions.h"
#include "Tools/Registry.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <atomic>
#include <chrono>
#include <ctime>
#include <format>
#include <optional>
#include <thread>
#include <windows.h>

namespace Bench::Capture {
namespace {
    using Clock = std::chrono::steady_clock;

    std::atomic<std::uint64_t>& RequestSequence() {
        static std::atomic<std::uint64_t> sequence {0};
        return sequence;
    }

    struct Ready {
        bool ok = false;
        std::string readyBy;
        std::string error;
        qint64 bytes = 0;
        std::optional<bool> uiExcluded;
        std::optional<int> width;
        std::optional<int> height;
    };

    // The writer has finished once the file can be opened without sharing.
    bool IsWriterDone(const QString& a_path) {
        auto* const handle = CreateFileW(
            reinterpret_cast<const wchar_t*>(a_path.utf16()),
            GENERIC_READ,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (handle == INVALID_HANDLE_VALUE) {
            return false;
        }
        CloseHandle(handle);
        return true;
    }

    std::optional<Ready> MatchReadyEvent(const Json& a_event, const std::string& a_requestId, const QString& a_path) {
        const auto& payload = a_event["data"];
        if (a_event.value("topic", std::string()) != "capture.ready"
            || payload.value("requestId", std::string()) != a_requestId) {
            return std::nullopt;
        }
        if (!payload.value("ok", false)) {
            return Ready {
                .readyBy = "event",
                .error = payload.value("error", std::string("Provider reported failure")),
            };
        }
        const auto size = QFileInfo(a_path).size();
        if (size == 0) {
            return std::nullopt;
        }
        Ready ready {.ok = true, .readyBy = "event", .bytes = size};
        if (payload.contains("uiExcluded")) {
            ready.uiExcluded = payload.value("uiExcluded", false);
        }
        if (payload.contains("width")) {
            ready.width = payload.value("width", 0);
        }
        if (payload.contains("height")) {
            ready.height = payload.value("height", 0);
        }
        return ready;
    }

    std::optional<Ready> FindReadyEvent(
        const EventBus* a_events,
        std::uint64_t& a_since,
        const std::string& a_requestId,
        const QString& a_path
    ) {
        if (a_events == nullptr) {
            return std::nullopt;
        }
        for (const auto& event : a_events->Since(a_since)["events"]) {
            a_since = event["seq"].get<std::uint64_t>();
            if (auto ready = MatchReadyEvent(event, a_requestId, a_path)) {
                return ready;
            }
        }
        return std::nullopt;
    }

    std::optional<Ready> PollArtifact(const QString& a_path, const int a_pollMs) {
        if (!QFileInfo::exists(a_path) || !IsWriterDone(a_path)) {
            return std::nullopt;
        }
        const auto firstSize = QFileInfo(a_path).size();
        std::this_thread::sleep_for(std::chrono::milliseconds(a_pollMs));
        const auto secondSize = QFileInfo(a_path).size();
        if (firstSize == 0 || firstSize != secondSize) {
            return std::nullopt;
        }
        return Ready {.ok = true, .readyBy = "poll", .bytes = secondSize};
    }

    Ready AwaitArtifact(
        const EventBus* a_events,
        std::uint64_t a_since,
        const std::string& a_requestId,
        const QString& a_path,
        const int a_timeoutMs,
        const int a_pollMs
    ) {
        const auto deadline = Clock::now() + std::chrono::milliseconds(a_timeoutMs);
        while (Clock::now() < deadline) {
            if (auto ready = FindReadyEvent(a_events, a_since, a_requestId, a_path)) {
                return *ready;
            }
            if (auto ready = PollArtifact(a_path, a_pollMs)) {
                return *ready;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(a_pollMs));
        }
        return {.readyBy = "poll", .error = "Timed out waiting for the capture provider"};
    }

    Json ProviderResult(
        const std::string& a_providerKey,
        const Json& a_arguments,
        const QString& a_path,
        const Ready& a_ready,
        const std::string& a_requestId,
        const Clock::time_point a_start
    ) {
        Json degraded = Json::array();
        if (a_arguments.value("excludeUi", true)) {
            if (!a_ready.uiExcluded.has_value()) {
                degraded.push_back("uiExclusionUnverified");
            } else if (!*a_ready.uiExcluded) {
                degraded.push_back("uiIncluded");
            }
        }
        Json result {
            {"ok", true},
            {"provider", a_providerKey},
            {"kind", "screenshot"},
            {"path", Utf8(QDir::fromNativeSeparators(a_path))},
            {"file", Utf8(QFileInfo(a_path).fileName())},
            {"bytes", a_ready.bytes},
            {"checkpointId", a_arguments.value("checkpointId", std::string())},
            {"recording", a_arguments.value("recording", std::string("adhoc"))},
            {"variant", a_arguments.value("variant", std::string("default"))},
            {"epoch", static_cast<long long>(std::time(nullptr))},
            {"sceneMismatch", a_arguments.value("sceneMismatch", false)},
            {"uiExcluded", a_ready.uiExcluded.value_or(false)},
            {"readyBy", a_ready.readyBy},
            {"captureElapsedMs", std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - a_start).count()},
            {"requestId", a_requestId},
            {"degraded", degraded},
        };
        if (a_ready.width) {
            result["width"] = *a_ready.width;
        }
        if (a_ready.height) {
            result["height"] = *a_ready.height;
        }
        return result;
    }
}

Json DispatchToProvider(
    const std::string& a_providerKey,
    const Json& a_arguments,
    const Settings& a_settings,
    EventBus* a_events
) {
    const auto start = Clock::now();
    const auto provider = ToolExtensions::Find("capture", a_providerKey);
    if (!provider) {
        throw ToolError(404, std::format("No capture provider registered under '{}'", a_providerKey));
    }
    const auto stem = CheckpointStem(a_arguments);
    const auto destination = QDir(CaptureDir(a_settings, a_arguments)).filePath(stem + ".png");

    const auto sequence = RequestSequence().fetch_add(1, std::memory_order_relaxed);
    const auto temporary = TemporaryPath("provider", sequence);
    const auto requestId = std::format(
        "{}#{}#{}",
        a_arguments.value("checkpointId", std::string()),
        a_arguments.value("runId", Json(0)).dump(),
        sequence
    );

    auto providerArguments = a_arguments;
    providerArguments["outputPath"] = Utf8(QDir::fromNativeSeparators(temporary));
    providerArguments["requestId"] = requestId;
    const auto since = a_events != nullptr ? a_events->HeadSequence() : 0;
    if (const auto queued = provider->handler(providerArguments); queued.contains("error")) {
        QFile::remove(temporary);
        throw ToolError(502, queued["error"].is_string() ? queued["error"].get<std::string>() : queued["error"].dump());
    }

    const auto ready = AwaitArtifact(
        a_events,
        since,
        requestId,
        temporary,
        a_arguments.value("timeoutMs", a_settings.timeoutMs),
        a_arguments.value("pollMs", 100)
    );
    if (!ready.ok) {
        PublishAbandoned(
            a_events,
            requestId,
            temporary,
            a_arguments,
            ready.readyBy == "event" ? "providerFailure" : "timeout"
        );
        QFile::remove(temporary);
        throw ToolError(504, std::format("Capture provider '{}' did not deliver: {}", a_providerKey, ready.error));
    }
    try {
        PublishFile(temporary, destination);
    } catch (const ToolError&) {
        PublishAbandoned(a_events, requestId, temporary, a_arguments, "publishFailed");
        throw;
    }
    QFile::remove(temporary);

    auto result = ProviderResult(a_providerKey, a_arguments, destination, ready, requestId, start);
    CompleteCapture(result, a_arguments, destination, a_settings, a_events);
    return ImageResult(result, destination);
}
}
