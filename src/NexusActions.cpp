#include "NexusActions.h"
#include "EventBus.h"
#include "Json.h"
#include "Mods/Actions.h"
#include "Tools/Registry.h"
#include "VariantJson.h"
#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QUuid>
#include <QVariant>
#include <format>
#include <qnamespace.h>
#include <uibase/imodinterface.h>
#include <uibase/imoinfo.h>
#include <uibase/iplugingame.h>

#include <ranges>
namespace Bench {
NexusActions::NexusActions(
    MOBase::IOrganizer* a_organizer,
    const ModActions& a_mods,
    EventBus& a_events,
    QObject* a_parent
)
    : QObject(a_parent)
    , _organizer(a_organizer)
    , _mods(a_mods)
    , _bridge(a_organizer->createNexusBridge())
    , _events(a_events) {
    _bridge->setParent(this);
    ConnectResponses();
}

void NexusActions::ConnectResponses() {
    connect(
        _bridge,
        &MOBase::IModRepositoryBridge::descriptionAvailable,
        this,
        [this](const QString&, int, const QVariant& a_id, const QVariant& a_data) { Complete(a_id, a_data); }
    );
    connect(
        _bridge,
        &MOBase::IModRepositoryBridge::fileInfoAvailable,
        this,
        [this](const QString&, int, int, const QVariant& a_id, const QVariant& a_data) { Complete(a_id, a_data); }
    );
    connect(
        _bridge,
        &MOBase::IModRepositoryBridge::downloadURLsAvailable,
        this,
        [this](const QString&, int, int, const QVariant& a_id, const QVariant& a_data) { Complete(a_id, a_data); }
    );
    connect(
        _bridge,
        &MOBase::IModRepositoryBridge::requestFailed,
        this,
        [this](const QString&, int, int, const QVariant& a_id, int a_code, const QString& a_message) {
            auto& request = _requests.at(Utf8(a_id.toString()));
            request["state"] = "failed";
            request["error"] = {{"code", a_code}, {"message", Utf8(a_message)}};
            _events.Publish("nexus", {{"requestId", request["requestId"]}, {"state", "failed"}});
        }
    );
}

void NexusActions::Complete(const QVariant& a_id, const QVariant& a_data) {
    auto& request = _requests.at(Utf8(a_id.toString()));
    request["state"] = "completed";
    request["result"] = VariantJson(a_data);
    request["receivedAt"] = Utf8(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    _events.Publish("nexus", {{"requestId", request["requestId"]}, {"state", "completed"}});
}

Json NexusActions::Start(const Json& a_args) {
    const auto action = a_args.at("action").get<std::string>();
    auto game = _organizer->managedGame()->gameShortName();
    int modId = a_args.value("modId", 0);
    if (a_args.contains("name")) {
        const auto* mod = _organizer->modList()->getMod(Text(a_args.at("name")));
        if (mod == nullptr) {
            throw ToolError(404, std::format("Unknown installed mod: {}", Utf8(Text(a_args.at("name")))));
        }
        game = mod->gameName();
        modId = mod->nexusId();
    }
    if (a_args.contains("game")) {
        game = Text(a_args.at("game"));
    }
    if (modId <= 0) {
        throw ToolError(400, "Specify a positive Nexus modId or an installed mod with a Nexus ID");
    }
    if (QString(MOPK_MO2_VERSION) == "2.5.2"
        && game != _organizer->managedGame()->gameShortName()
        && game != _organizer->managedGame()->gameNexusName()) {
        throw ToolError(
            400,
            "MO2 2.5.2's Nexus bridge only targets the managed game. Use the 2.5.3beta12 host for another game"
        );
    }
    const int fileId = action == "description" ? 0 : a_args.at("fileId").get<int>();
    if (action != "description" && fileId <= 0) {
        throw ToolError(400, "Specify a positive Nexus fileId");
    }

    const auto requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto& request = _requests[Utf8(requestId)];
    request = {
        {"requestId", Utf8(requestId)},
        {"action", action},
        {"state", "pending"},
        {"game", Utf8(game)},
        {"modId", modId},
        {"fileId", fileId},
        {"startedAt", Utf8(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs))},
    };
    if (action == "description") {
        _bridge->requestDescription(game, modId, requestId);
    } else if (action == "fileInfo") {
        _bridge->requestFileInfo(game, modId, fileId, requestId);
    } else {
        _bridge->requestDownloadURL(game, modId, fileId, requestId);
    }
    return request;
}

Json NexusActions::Cached(const QString& a_name) const {
    const auto* mod = _organizer->modList()->getMod(a_name);
    if (mod == nullptr) {
        throw ToolError(404, std::format("Unknown installed mod: {}", Utf8(a_name)));
    }
    const QSettings metadata(mod->absolutePath() + "/meta.ini", QSettings::IniFormat);
    Json values = Json::object();
    for (const auto& key : metadata.allKeys()) {
        values[Utf8(key)] = VariantJson(metadata.value(key));
    }
    Json responses = Json::array();
    for (const auto& request : _requests | std::views::values) {
        if (request["modId"] == mod->nexusId() && request["game"] == Utf8(mod->gameName())) {
            responses.push_back(request);
        }
    }
    return {{"mod", _mods.Describe(a_name)}, {"metadata", values}, {"requests", responses}};
}

Json NexusActions::Invoke(const Json& a_args) {
    const auto action = a_args.value("action", "status");
    if (action == "cached") {
        return Cached(Text(a_args.at("name")));
    }
    if (action == "status") {
        if (a_args.contains("requestId")) {
            const auto found = _requests.find(a_args.at("requestId").get<std::string>());
            if (found == _requests.end()) {
                throw ToolError(404, "Unknown Nexus request in this MO2 process session");
            }
            return found->second;
        }
        Json result = Json::array();
        for (const auto& request : _requests | std::views::values) {
            result.push_back(request);
        }
        return {{"requests", result}};
    }
    return Start(a_args);
}
}
