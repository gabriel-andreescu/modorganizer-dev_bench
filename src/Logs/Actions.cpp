#include "Logs/Actions.h"
#include "Json.h"
#include "Logs/Tail.h"
#include "Tools/Registry.h"
#include <QAbstractItemModel>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTreeView>
#include <QVariant>
#include <algorithm>
#include <cstdint>
#include <qnamespace.h>
#include <qobject.h>

namespace Bench {
namespace {
    Json ReadLive(std::uint64_t a_count) {
        const QAbstractItemModel* model = nullptr;
        for (auto* widget : QApplication::allWidgets()) {
            if (widget->objectName() == "logList") {
                if (const auto* view = qobject_cast<QTreeView*>(widget)) {
                    model = view->model();
                }
            }
        }
        if (model == nullptr) {
            throw ToolError(409, "MO2's live log is not ready");
        }
        const int total = model->rowCount();
        const int count = static_cast<int>(std::min(a_count, static_cast<std::uint64_t>(total)));
        Json entries = Json::array();
        for (int row = total - count; row < total; ++row) {
            entries.push_back({
                {"time", Utf8(model->data(model->index(row, 0), Qt::DisplayRole).toString())},
                {"message", Utf8(model->data(model->index(row, 2), Qt::DisplayRole).toString())},
            });
        }
        return {
            {"source", "live"},
            {"entries", entries},
            {"requested", a_count},
            {"returned", count},
            {"available", total},
            {"truncated", total > count},
        };
    }
}

Json Logs(const Json& a_args) {
    const QDir directory(qApp->property("dataPath").toString() + "/logs");
    const auto action = a_args.value("action", "list");
    if (action == "list") {
        Json files = Json::array();
        for (const auto& file : directory.entryInfoList(QDir::Files, QDir::Time)) {
            files.push_back({
                {"name", Utf8(file.fileName())},
                {"bytes", file.size()},
                {"modified", Utf8(file.lastModified().toUTC().toString(Qt::ISODateWithMs))},
            });
        }
        return {
            {"sources", {"live", "file"}},
            {"files", files},
            {"directory", Utf8(directory.absolutePath())},
            {"fileScope", "active MO2 instance directory. Files may include earlier or concurrent processes"},
        };
    }

    const auto count = a_args.value("lines", std::uint64_t {10});
    if (a_args.value("source", "live") == "live") {
        return ReadLive(count);
    }
    const auto name = Text(a_args.at("file"));
    if (name.isEmpty() || name.contains('/') || name.contains('\\') || name == "." || name == "..") {
        throw ToolError(400, "Use a log filename from logs list");
    }
    QFile file(directory.filePath(name));
    if (!file.open(QIODevice::ReadOnly)) {
        throw ToolError(404, "Cannot open the selected log: " + Utf8(file.errorString()));
    }
    auto result = ReadLogTail(file, count);
    result["source"] = "file";
    result["file"] = Utf8(name);
    return result;
}
}
