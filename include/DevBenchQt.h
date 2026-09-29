// SPDX-License-Identifier: MIT
//
// Qt adapter for DevBenchAPI handlers. MIT-licensed like DevBenchAPI.h, so any MO2
// plugin may vendor it or consume the devbench-api package.
#pragma once

#include "DevBenchAPI.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <exception>

// Each Respond function parses the request into a QJsonObject, calls a handler of the
// form QJsonObject(const QJsonObject&) and writes its result. Malformed arguments and
// exceptions the handler throws become the error result.
namespace DevBenchQt {
namespace Detail {
    template <class Handler, class ErrorResult>
    QJsonObject Result(const Handler& a_handler, const char* a_argsJson, const ErrorResult& a_error) {
        QJsonParseError error {};
        const auto arguments = QJsonDocument::fromJson(QByteArray(a_argsJson), &error);
        if (error.error != QJsonParseError::NoError) {
            return a_error(QStringLiteral("Invalid arguments JSON: %1").arg(error.errorString()));
        }
        if (!arguments.isObject()) {
            return a_error(QStringLiteral("Arguments must be a JSON object"));
        }
        try {
            return a_handler(arguments.object());
        } catch (const std::exception& exception) {
            return a_error(QString::fromUtf8(exception.what()));
        } catch (...) {
            return a_error(QStringLiteral("The handler failed with an unknown exception"));
        }
    }

    inline void Write(const QJsonObject& a_result, void* a_sink, const DevBenchAPI::WriteFn a_write) {
        a_write(a_sink, QJsonDocument(a_result).toJson(QJsonDocument::Compact).constData());
    }
}

// For RegisterTool handlers. Errors become an MCP tool result with isError set.
template <class Handler>
void RespondToTool(const Handler& a_handler, const char* a_argsJson, void* a_sink, const DevBenchAPI::WriteFn a_write) {
    const auto error = [](const QString& a_message) {
        return QJsonObject {
            {"content", QJsonArray {QJsonObject {{"type", "text"}, {"text", a_message}}}},
            {"isError", true},
        };
    };
    Detail::Write(Detail::Result(a_handler, a_argsJson, error), a_sink, a_write);
}

// For RegisterToolExtension handlers. Errors become {"error": message}, which base
// tools such as capture report as the extension's failure.
template <class Handler>
void RespondToExtension(
    const Handler& a_handler,
    const char* a_argsJson,
    void* a_sink,
    const DevBenchAPI::WriteFn a_write
) {
    const auto error = [](const QString& a_message) { return QJsonObject {{"error", a_message}}; };
    Detail::Write(Detail::Result(a_handler, a_argsJson, error), a_sink, a_write);
}
}
