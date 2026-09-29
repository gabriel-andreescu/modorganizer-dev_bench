#include "Capture/WindowCapture.h"
#include "Tools/Registry.h"
#include <QImage>
#include <memory>
#include <minwindef.h>
#include <type_traits>
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>

namespace Bench {
QImage CaptureWindow(HWND a_window) {
    RECT bounds {};
    if ((GetWindowRect(a_window, &bounds) == 0) || (IsIconic(a_window) != 0)) {
        throw ToolError(409, "Restore the MO2 window before capturing it");
    }
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    const std::unique_ptr<std::remove_pointer_t<HDC>, decltype(&DeleteDC)> context(
        CreateCompatibleDC(nullptr),
        DeleteDC
    );
    BITMAPINFO info {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    const std::unique_ptr<std::remove_pointer_t<HBITMAP>, decltype(&DeleteObject)> bitmap(
        CreateDIBSection(context.get(), &info, DIB_RGB_COLORS, &pixels, nullptr, 0),
        DeleteObject
    );
    if (!context || !bitmap) {
        throw ToolError(500, "Windows could not allocate the window capture");
    }

    auto* const previous = SelectObject(context.get(), bitmap.get());
    const bool captured = PrintWindow(a_window, context.get(), PW_RENDERFULLCONTENT) != FALSE;
    SelectObject(context.get(), previous);
    if (!captured) {
        throw ToolError(500, "Windows could not render the MO2 window");
    }
    return QImage(static_cast<const unsigned char*>(pixels), width, height, QImage::Format_RGB32).copy();
}
}
