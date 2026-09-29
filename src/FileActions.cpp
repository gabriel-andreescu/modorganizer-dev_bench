#include "FileActions.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <format>
#include <uibase/imoinfo.h>

namespace Bench {
Json Files(const MOBase::IOrganizer* a_organizer, const Json& a_args) {
    const auto path = Text(a_args.value("path", Json("")));
    const auto action = a_args.value("action", "find");
    if (action == "resolve") {
        return {
            {"path", Utf8(a_organizer->resolvePath(path))},
            {"origins", Strings(a_organizer->getFileOrigins(path))},
        };
    }
    if (action == "directories") {
        return {{"directories", Strings(a_organizer->listDirectories(path))}};
    }
    if (action == "info") {
        const auto filters = StringList(a_args.value("filters", Json::array({"*"})));
        const auto infos = a_organizer->findFileInfos(path, [&filters](const MOBase::IOrganizer::FileInfo& a_info) {
            return QDir::match(filters, QFileInfo(a_info.filePath).fileName());
        });
        const auto offset = a_args.value("offset", 0);
        const auto limit = a_args.value("limit", 100);
        const auto total = static_cast<int>(infos.size());
        const auto end = offset >= total ? total : offset + std::min(limit, total - offset);
        Json files = Json::array();
        for (int index = offset; index < end; ++index) {
            const auto& info = infos[index];
            files.push_back({
                {"path", Utf8(QDir::cleanPath(QDir(path).filePath(QFileInfo(info.filePath).fileName())))},
                {"resolvedPath", Utf8(info.filePath)},
                {"archive", Utf8(info.archive)},
                {"origins", Strings(info.origins)},
            });
        }
        return {
            {"files", files},
            {"total", total},
            {"offset", offset},
            {"nextOffset", end < total ? Json(end) : Json(nullptr)},
        };
    }
    if (action == "find") {
        return {
            {"files", Strings(a_organizer->findFiles(path, StringList(a_args.value("filters", Json::array({"*"})))))},
        };
    }
    throw ToolError(400, std::format("Unknown files action: {}", action));
}
}
