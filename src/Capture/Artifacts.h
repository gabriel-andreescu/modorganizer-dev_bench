#pragma once
#include "Json.h"
#include <QString>
#include <cstdint>
#include <string_view>

namespace Bench::Capture {
struct Settings {
    // Relative golden paths resolve against this directory, the MO2 install.
    QString root;
    // Relative outDir values resolve against this directory.
    QString captureBase;
    int timeoutMs = 8000;
};

// <outDir or captureBase>/<recording>/<variant>, created on demand.
QString CaptureDir(const Settings& a_settings, const Json& a_arguments);
// checkpointId, suffixed with __r<repeat> when repeat is present. Throws 400 without a valid checkpointId.
QString CheckpointStem(const Json& a_arguments);
QString TemporaryPath(std::string_view a_kind, std::uint64_t a_sequence);
// Copies with retries while another process still holds the source. Throws 500 on failure.
void PublishFile(const QString& a_source, const QString& a_destination);
}
