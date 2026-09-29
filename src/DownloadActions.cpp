#include "DownloadActions.h"
#include "Json.h"
#include "VariantJson.h"
#include <QFileInfo>
#include <QSettings>
#include <uibase/idownloadmanager.h>
#include <uibase/imoinfo.h>

namespace Bench {
Json Downloads(const MOBase::IOrganizer* a_organizer, const Json& a_args) {
    const auto action = a_args.value("action", "list");
    if (action == "start") {
        return {{"downloadId", a_organizer->downloadManager()->startDownloadURLs(StringList(a_args.at("urls")))}};
    }
    if (action == "nexus") {
        return {
            {
                "downloadId",
                a_organizer->downloadManager()
                    ->startDownloadNexusFile(a_args.at("modId").get<int>(), a_args.at("fileId").get<int>()),
            },
        };
    }
    if (action == "path") {
        return {{"path", Utf8(a_organizer->downloadManager()->downloadPath(a_args.at("id").get<int>()))}};
    }
    Json result = Json::array();
    for (const auto& file : QDir(a_organizer->downloadsPath()).entryInfoList(QDir::Files, QDir::Name)) {
        if (file.suffix() == "meta") {
            continue;
        }
        const QSettings meta(file.absoluteFilePath() + ".meta", QSettings::IniFormat);
        Json metadata = Json::object();
        for (const auto& key : meta.allKeys()) {
            metadata[Utf8(key)] = VariantJson(meta.value(key));
        }
        result.push_back({
            {"name", Utf8(file.fileName())},
            {"path", Utf8(file.absoluteFilePath())},
            {"bytes", file.size()},
            {"metadata", metadata},
        });
    }
    return {{"downloads", result}};
}
}
