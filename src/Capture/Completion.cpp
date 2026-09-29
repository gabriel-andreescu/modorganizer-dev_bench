#include "Capture/Completion.h"
#include "Capture/Ssim.h"
#include "EventBus.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <utility>

namespace Bench::Capture {
namespace {
    void AddCorrelationFields(Json& a_result, const Json& a_arguments) {
        for (const char* field : {"runId", "repeat", "atMs", "resolvedAtMs", "resolvedIndex"}) {
            if (a_arguments.contains(field)) {
                a_result[field] = a_arguments[field];
            }
        }
    }

    void ComputeInconclusive(Json& a_result) {
        std::string reason;
        if (a_result.value("sceneMismatch", false)) {
            reason = "sceneMismatch";
        } else if (const auto degraded = a_result.value("degraded", Json::array()); !degraded.empty()) {
            std::string list;
            for (const auto& item : degraded) {
                list += (list.empty() ? "" : ", ") + item.get<std::string>();
            }
            reason = "degraded: " + list;
        }
        a_result["inconclusive"] = !reason.empty();
        if (!reason.empty()) {
            a_result["inconclusiveReason"] = reason;
        }
    }

    void MaybeScoreAgainstGolden(
        Json& a_result,
        const Json& a_arguments,
        const QString& a_path,
        const Settings& a_settings
    ) {
        const auto golden = QString::fromStdString(a_arguments.value("golden", std::string()));
        if (golden.isEmpty()) {
            return;
        }
        const auto goldenPath = QDir::isRelativePath(golden) ? QDir(a_settings.root).filePath(golden) : golden;
        const auto score = Ssim::ScoreAgainstGolden(a_path, goldenPath, a_arguments);
        if (!score.ok) {
            a_result["goldenError"] = score.error;
            return;
        }
        a_result["ssim"] = score.score;
        a_result["threshold"] = score.threshold;
        a_result["passed"] = score.passed;
        if (!score.regions.empty()) {
            Json regions = Json::array();
            for (const auto& region : score.regions) {
                regions.push_back({
                    {"name", region.name},
                    {"ssim", region.score},
                    {"threshold", region.threshold},
                    {"passed", region.passed},
                });
            }
            a_result["regions"] = std::move(regions);
        }
    }

    void WriteSidecar(const QString& a_imagePath, const Json& a_result) {
        const QFileInfo image(a_imagePath);
        QSaveFile sidecar(image.dir().filePath(image.completeBaseName() + ".json"));
        if (sidecar.open(QIODevice::WriteOnly)) {
            sidecar.write(QByteArray::fromStdString(a_result.dump(2) + "\n"));
            sidecar.commit();
        } else {
            qWarning("Dev Bench could not write capture sidecar %s", qUtf8Printable(sidecar.fileName()));
        }
    }
}

void CompleteCapture(
    Json& a_result,
    const Json& a_arguments,
    const QString& a_path,
    const Settings& a_settings,
    EventBus* a_events
) {
    AddCorrelationFields(a_result, a_arguments);
    ComputeInconclusive(a_result);
    MaybeScoreAgainstGolden(a_result, a_arguments, a_path, a_settings);
    WriteSidecar(a_path, a_result);
    if (a_events != nullptr) {
        a_events->Publish("capture.saved", a_result);
    }
}

void PublishAbandoned(
    EventBus* a_events,
    const std::string& a_requestId,
    const QString& a_path,
    const Json& a_arguments,
    const std::string& a_reason
) {
    if (a_events == nullptr) {
        return;
    }
    a_events->Publish(
        "capture.abandoned",
        {
            {"requestId", a_requestId},
            {"path", Utf8(a_path)},
            {"checkpointId", a_arguments.value("checkpointId", std::string())},
            {"runId", a_arguments.value("runId", Json(nullptr))},
            {"reason", a_reason},
        }
    );
}

Json ImageResult(const Json& a_result, const QString& a_path) {
    Json content = Json::array({{{"type", "text"}, {"text", a_result.dump()}}});
    QFile image(a_path);
    if (image.open(QIODevice::ReadOnly)) {
        content.push_back(
            {{"type", "image"}, {"mimeType", "image/png"}, {"data", image.readAll().toBase64().toStdString()}}
        );
    }
    return {{"content", content}, {"structuredContent", a_result}};
}
}
