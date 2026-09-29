#pragma once
#include "Json.h"
#include <vector>
#include <windows.h>

namespace Bench {
// Visible top-level windows of this MO2 process, including dialogs outside the main window.
std::vector<HWND> VisibleWindows();
Json DescribeWindow(HWND a_window);
// Throws 404 when the id is not a visible window of this process.
HWND FindVisibleWindow(std::uintptr_t a_id);
}
