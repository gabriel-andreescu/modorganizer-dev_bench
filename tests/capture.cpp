#include "Capture/Providers.h"
#include "EventBus.h"
#include "Harness.h"
#include "Tools/Extensions.h"
#include "Tools/Registry.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

namespace {
using Bench::Json;

QImage Pattern() {
    QImage image(32, 32, QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixel(x, y, ((x / 4) + (y / 4)) % 2 == 0 ? qRgb(220, 180, 40) : qRgb(20, 40, 90));
        }
    }
    return image;
}

void RegisterProvider(Bench::EventBus& a_events) {
    Bench::ToolExtensions::Register(
        "capture",
        "fixture",
        {{"description", "Writes a fixed pattern"}},
        [&a_events](const Json& a_arguments) {
            Pattern().save(QString::fromStdString(a_arguments["outputPath"].get<std::string>()), "PNG");
            a_events.Publish(
                "capture.ready",
                {{"requestId", a_arguments["requestId"]},
                    {"ok", true},
                    {"uiExcluded", true},
                    {"width", 32},
                    {"height", 32}}
            );
            return Json {{"queued", true}};
        }
    );
    Bench::ToolExtensions::Register("capture", "broken", Json::object(), [](const Json&) {
        return Json {{"error", "renderer unavailable"}};
    });
}

int ExpectStatus(const std::function<void()>& a_call, int a_status, int a_failure) {
    try {
        a_call();
    } catch (const Bench::ToolError& error) {
        return error.Code() == a_status ? 0 : a_failure;
    }
    return a_failure;
}

int CheckDelivery(const QTemporaryDir& a_directory, Bench::EventBus& a_events) {
    const Bench::Capture::Settings settings {.root = a_directory.path(), .captureBase = a_directory.path()};
    const auto golden = QDir(a_directory.path()).filePath("golden.png");
    Pattern().save(golden, "PNG");

    const auto result = Bench::Capture::DispatchToProvider(
        "fixture",
        {{"checkpointId", "pattern"}, {"recording", "suite"}, {"golden", "golden.png"}},
        settings,
        &a_events
    )["structuredContent"];
    const auto published = QDir(a_directory.path()).filePath("suite/default/pattern.png");
    if (result["path"] != Bench::Utf8(published)
        || !QFileInfo::exists(published)
        || !QFileInfo::exists(QDir(a_directory.path()).filePath("suite/default/pattern.json"))) {
        return 1;
    }
    if (result["readyBy"] != "event"
        || result["uiExcluded"] != true
        || result["inconclusive"] != false
        || result["width"] != 32) {
        return 2;
    }
    return result["passed"] == true && result["ssim"].get<double>() > 0.99 ? 0 : 3;
}

int CheckFailures(const QTemporaryDir& a_directory, Bench::EventBus& a_events) {
    const Bench::Capture::Settings settings {.root = a_directory.path(), .captureBase = a_directory.path()};
    const Json arguments = {{"checkpointId", "pattern"}};
    return Tests::First({
        [&] {
            return ExpectStatus(
                [&] { Bench::Capture::DispatchToProvider("missing", arguments, settings, &a_events); },
                404,
                4
            );
        },
        [&] {
            return ExpectStatus(
                [&] { Bench::Capture::DispatchToProvider("broken", arguments, settings, &a_events); },
                502,
                5
            );
        },
        [&] {
            return ExpectStatus(
                [&] { Bench::Capture::DispatchToProvider("fixture", Json::object(), settings, &a_events); },
                400,
                6
            );
        },
    });
}
}

int main() {
    return Tests::Run([] {
        const QTemporaryDir directory;
        Bench::EventBus events;
        RegisterProvider(events);
        return Tests::First({
            [&] { return CheckDelivery(directory, events); },
            [&] { return CheckFailures(directory, events); },
        });
    });
}
