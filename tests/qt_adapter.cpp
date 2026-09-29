#include "DevBenchQt.h"
#include "Harness.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>
#include <string>

namespace {
QJsonObject Echo(const QJsonObject& a_arguments) {
    if (a_arguments.contains("fail")) {
        throw std::runtime_error("requested failure");
    }
    return {{"echo", a_arguments["value"]}};
}

template <class Respond>
QJsonObject Call(const Respond& a_respond, const char* a_argsJson) {
    std::string written;
    const DevBenchAPI::WriteFn write = +[](void* a_sink, const char* a_json) {
        *static_cast<std::string*>(a_sink) = a_json;
    };
    a_respond(&Echo, a_argsJson, &written, write);
    return QJsonDocument::fromJson(QByteArray::fromStdString(written)).object();
}

int CheckTool() {
    const auto respond = [](auto a_handler, const char* a_args, void* a_sink, DevBenchAPI::WriteFn a_write) {
        DevBenchQt::RespondToTool(a_handler, a_args, a_sink, a_write);
    };
    if (Call(respond, R"({"value":3})")["echo"].toInt() != 3) {
        return 1;
    }
    const auto failed = Call(respond, R"({"fail":true})");
    if (!failed["isError"].toBool() || failed["content"][0]["text"].toString() != QStringLiteral("requested failure")) {
        return 2;
    }
    if (!Call(respond, "[1]")["isError"].toBool() || !Call(respond, "{")["isError"].toBool()) {
        return 3;
    }
    return 0;
}

int CheckExtension() {
    const auto respond = [](auto a_handler, const char* a_args, void* a_sink, DevBenchAPI::WriteFn a_write) {
        DevBenchQt::RespondToExtension(a_handler, a_args, a_sink, a_write);
    };
    if (Call(respond, R"({"fail":true})")["error"].toString() != QStringLiteral("requested failure")) {
        return 11;
    }
    return Call(respond, R"({"value":"x"})")["echo"].toString() == QStringLiteral("x") ? 0 : 12;
}
}

int main() {
    return Tests::Run([] { return Tests::First({CheckTool, CheckExtension}); });
}
