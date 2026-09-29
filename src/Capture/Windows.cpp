#include "Capture/Windows.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QString>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <processthreadsapi.h>
#include <winuser.h>

namespace Bench {
namespace {
    BOOL CALLBACK CollectWindow(HWND a_window, LPARAM a_context) {
        DWORD process = 0;
        GetWindowThreadProcessId(a_window, &process);
        if (process == GetCurrentProcessId() && (IsWindowVisible(a_window) != 0)) {
            auto& windows = *std::bit_cast<std::vector<HWND>*>(a_context);
            windows.push_back(a_window);
        }
        return TRUE;
    }
}

std::vector<HWND> VisibleWindows() {
    std::vector<HWND> windows;
    EnumWindows(CollectWindow, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

Json DescribeWindow(HWND a_window) {
    RECT bounds {};
    GetWindowRect(a_window, &bounds);
    std::array<wchar_t, 1024> title {};
    GetWindowTextW(a_window, title.data(), static_cast<int>(title.size()));
    return {
        {"id", reinterpret_cast<std::uintptr_t>(a_window)},
        {"owner", reinterpret_cast<std::uintptr_t>(GetWindow(a_window, GW_OWNER))},
        {"title", Utf8(QString::fromWCharArray(title.data()))},
        {"foreground", a_window == GetForegroundWindow()},
        {"minimized", IsIconic(a_window) != FALSE},
        {
            "bounds",
            {
                {"x", bounds.left},
                {"y", bounds.top},
                {"width", bounds.right - bounds.left},
                {"height", bounds.bottom - bounds.top},
            },
        },
    };
}

HWND FindVisibleWindow(const std::uintptr_t a_id) {
    const auto windows = VisibleWindows();
    const auto found = std::ranges::find_if(windows, [a_id](HWND a_window) {
        return reinterpret_cast<std::uintptr_t>(a_window) == a_id;
    });
    if (found == windows.end()) {
        throw ToolError(404, "Window is not visible in this MO2 process. List capture windows again");
    }
    return *found;
}
}
