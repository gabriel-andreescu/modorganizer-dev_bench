#include "Capture/Artifacts.h"
#include "Tools/Registry.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <chrono>
#include <format>
#include <string>
#include <thread>

namespace Bench::Capture {
namespace {
    void ValidatePathSegment(std::string_view a_field, const std::string& a_value) {
        if (a_value.empty() || a_value == "." || a_value == ".." || a_value.contains('/') || a_value.contains('\\')) {
            throw ToolError(
                400,
                std::format(
                    "Invalid '{}' value '{}': must be a single path segment (no '/', '\\', '.', or '..')",
                    a_field,
                    a_value
                )
            );
        }
    }

    QString Segment(const Json& a_arguments, const char* a_field, const char* a_default) {
        const auto value = a_arguments.value(a_field, std::string(a_default));
        ValidatePathSegment(a_field, value);
        return QString::fromStdString(value);
    }
}

QString CaptureDir(const Settings& a_settings, const Json& a_arguments) {
    auto base = QString::fromStdString(a_arguments.value("outDir", std::string()));
    if (base.isEmpty()) {
        base = a_settings.captureBase;
    } else if (QDir::isRelativePath(base)) {
        base = QDir(a_settings.captureBase).filePath(base);
    }
    const auto directory = QDir(base).filePath(
        Segment(a_arguments, "recording", "adhoc") + "/" + Segment(a_arguments, "variant", "default")
    );
    if (!QDir().mkpath(directory)) {
        throw ToolError(500, "Cannot create capture directory " + directory.toStdString());
    }
    return directory;
}

QString CheckpointStem(const Json& a_arguments) {
    const auto checkpointId = a_arguments.value("checkpointId", std::string());
    if (checkpointId.empty()) {
        throw ToolError(400, "Capture requires 'checkpointId'");
    }
    ValidatePathSegment("checkpointId", checkpointId);
    auto stem = QString::fromStdString(checkpointId);
    if (a_arguments.contains("repeat")) {
        stem += QStringLiteral("__r%1").arg(a_arguments.value("repeat", 0));
    }
    return stem;
}

QString TemporaryPath(std::string_view a_kind, const std::uint64_t a_sequence) {
    const auto directory = QDir(QDir::tempPath()).filePath("devbench-mo2");
    QDir().mkpath(directory);
    const auto name = std::format("capture-{}-{}-{}.png", a_kind, QCoreApplication::applicationPid(), a_sequence);
    const auto path = QDir(directory).filePath(QString::fromStdString(name));
    QFile::remove(path);
    return path;
}

void PublishFile(const QString& a_source, const QString& a_destination) {
    QString error;
    for (unsigned int attempt = 0; attempt < 5U; ++attempt) {
        QFile::remove(a_destination);
        QFile source(a_source);
        if (source.copy(a_destination)) {
            return;
        }
        error = source.errorString();
        std::this_thread::sleep_for(std::chrono::milliseconds(50U * (1U << attempt)));
    }
    throw ToolError(
        500,
        std::format(
            "Failed to publish capture {} to {}: {}",
            a_source.toStdString(),
            a_destination.toStdString(),
            error.toStdString()
        )
    );
}
}
