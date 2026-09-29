#include "Capture/Native.h"
#include "Capture/Completion.h"
#include "Capture/WindowCapture.h"
#include "Capture/Windows.h"
#include "MainThread.h"
#include "Tools/Registry.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <processthreadsapi.h>
#include <winuser.h>

namespace Bench::Capture {
namespace {
    HWND SelectWindow(const Json& a_arguments) {
        if (a_arguments.contains("window")) {
            return FindVisibleWindow(a_arguments["window"].get<std::uintptr_t>());
        }
        const auto windows = VisibleWindows();
        const auto main = std::ranges::find_if(windows, [](HWND a_window) {
            return GetWindow(a_window, GW_OWNER) == nullptr;
        });
        if (main == windows.end()) {
            throw ToolError(404, "MO2 has no visible main window");
        }
        return *main;
    }

    Json Degradation(const Json& a_arguments) {
        Json degraded = Json::array();
        if (a_arguments.value("excludeUi", true)) {
            degraded.push_back("uiIncluded");
        }
        if (a_arguments.contains("subrect")) {
            degraded.push_back("subrectUnsupported");
        }
        return degraded;
    }
}

Json Native(const Json& a_arguments, const Settings& a_settings, EventBus* a_events, MainThread& a_main) {
    const auto start = std::chrono::steady_clock::now();
    const auto stem = CheckpointStem(a_arguments);
    const auto destination = QDir(CaptureDir(a_settings, a_arguments)).filePath(stem + ".png");

    const auto window = a_main.Run([&a_arguments, destination] {
        auto* const target = SelectWindow(a_arguments);
        if (!CaptureWindow(target).save(destination, "PNG")) {
            throw ToolError(500, "Could not write window capture " + destination.toStdString());
        }
        return DescribeWindow(target);
    });
    const auto elapsed = std::chrono::steady_clock::now() - start;

    Json result {
        {"ok", true},
        {"provider", "native"},
        {"kind", "screenshot"},
        {"path", Utf8(QDir::fromNativeSeparators(destination))},
        {"file", Utf8(QFileInfo(destination).fileName())},
        {"bytes", QFileInfo(destination).size()},
        {"checkpointId", a_arguments.value("checkpointId", std::string())},
        {"recording", a_arguments.value("recording", std::string("adhoc"))},
        {"variant", a_arguments.value("variant", std::string("default"))},
        {"window", window},
        {"epoch", static_cast<long long>(std::time(nullptr))},
        {"sceneMismatch", a_arguments.value("sceneMismatch", false)},
        {"uiExcluded", false},
        {"readyBy", "present"},
        {"captureElapsedMs", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()},
        {"degraded", Degradation(a_arguments)},
    };
    CompleteCapture(result, a_arguments, destination, a_settings, a_events);
    return ImageResult(result, destination);
}

Json ListWindows() {
    Json windows = Json::array();
    for (auto* const window : VisibleWindows()) {
        windows.push_back(DescribeWindow(window));
    }
    return {{"processId", GetCurrentProcessId()}, {"windows", windows}};
}
}
