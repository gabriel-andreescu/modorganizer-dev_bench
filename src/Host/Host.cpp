#include "Host/Host.h"
#include "Configuration.h"
#include "DownloadActions.h"
#include "Executables/Actions.h"
#include "ExtensionApi.h"
#include "FileActions.h"
#include "Json.h"
#include "Logs/Actions.h"
#include "Plugins/Actions.h"
#include "Profiles/Actions.h"
#include "Scenario.h"
#include "Settings/Actions.h"
#include "Tools/Catalog.h"
#include "Tools/Extensions.h"
#include "Tools/Registry.h"
#include <QCoreApplication>
#include <QDir>
#include <cstdint>
#include <functional>
#include <map>
#include <qobject.h>
#include <qpointer.h>
#include <uibase/imoinfo.h>
#include <uibase/iplugingame.h>
#include <uibase/ipluginlist.h>
#include <utility>

namespace Bench {
namespace {
    Capture::Settings CaptureSettings(const MOBase::IOrganizer* a_organizer) {
        return {
            .root = QDir(QCoreApplication::applicationDirPath()).canonicalPath(),
            .captureBase = QDir(a_organizer->basePath()).filePath("devbench/captures"),
        };
    }
}

Host::Host(MOBase::IOrganizer* a_organizer, QObject* a_parent)
    : QObject(a_parent)
    , _organizer(a_organizer)
    , _main(this)
    , _processes(_events, this)
    , _runtime(InstanceName(a_organizer), a_organizer->managedGame()->gameName())
    , _mods(a_organizer, _events, this)
    , _nexus(a_organizer, _mods, _events, this)
    , _profileManager(_events, this)
    , _executableManager(a_organizer, _events, this)
    , _settingsManager(_dialogs, _events, this)
    , _categoryManager(_events, this)
    , _conflicts(a_organizer, _events, this)
    , _installation(a_organizer, _mods, _dialogs, _events, this)
    , _capture(CaptureSettings(a_organizer), _events, _main)
    , _server(_registry, _events, _runtime, _main) {
    RegisterTools();
    ToolExtensions::SetChangeListener([this](const std::string& a_baseTool) {
        if (a_baseTool == "capture") {
            RefreshCaptureTool();
        }
    });
    ConnectModEvents();
    ConnectPluginEvents();
    ConnectLifecycleEvents();
}

Host::~Host() {
    ToolExtensions::SetChangeListener({});
    SetExtensionServices(nullptr, nullptr, {});
    _server.Stop();
}

void Host::Start() {
    _server.Start(_organizer->pluginSetting("Dev Bench", "port").toInt());
    SetExtensionServices(&_registry, &_events, [this](std::function<Json()> a_work) {
        return _main.Run(std::move(a_work));
    });
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] {
        SetExtensionServices(nullptr, nullptr, {});
        _server.Stop();
    });
    _events.Publish("lifecycle", {{"event", "ready"}, {"profile", Utf8(_organizer->profileName())}});
}

void Host::RegisterTools() {
    const auto mainThreadHandlers = MainThreadHandlers();
    for (auto descriptor : Catalog()) {
        const auto name = descriptor["name"].get<std::string>();
        if (mainThreadHandlers.contains(name)) {
            _registry.Add(descriptor, OnMainThread(mainThreadHandlers.at(name)));
        } else if (auto handler = DirectHandler(name)) {
            _registry.Add(descriptor, std::move(handler));
        }
    }
}

ToolRegistry::Handler Host::OnMainThread(ToolRegistry::Handler a_handler) {
    return [this, handler = std::move(a_handler)](const Json& a_args) {
        // Run can time out while the task is still executing, so the task owns copies.
        return _main.Run([handler, arguments = Json(a_args)] { return handler(arguments); });
    };
}

std::map<std::string, ToolRegistry::Handler> Host::MainThreadHandlers() {
    return {
        {"conflicts", [this](const Json& a_args) { return _conflicts.Invoke(a_args); }},
        {"categories", [this](const Json& a_args) { return _categoryManager.Invoke(a_args); }},
        {"nexus", [this](const Json& a_args) { return _nexus.Invoke(a_args); }},
        {"logs", [](const Json& a_args) { return Logs(a_args); }},
        {"ui", [this](const Json& a_args) { return _ui.Invoke(a_args); }},
        {"mods", [this](const Json& a_args) { return _mods.Invoke(a_args); }},
        {"install", [this](const Json& a_args) { return _installation.Invoke(a_args); }},
        {"dialogs", [this](const Json& a_args) { return _dialogs.Invoke(a_args); }},
        {"profiles", [this](const Json& a_args) { return Profiles(_organizer, _profileManager, a_args); }},
        {"files", [this](const Json& a_args) { return Files(_organizer, a_args); }},
        {"downloads", [this](const Json& a_args) { return Downloads(_organizer, a_args); }},
        {
            "executables",
            [this](const Json& a_args) { return Executables(_organizer, _processes, _executableManager, a_args); },
        },
        {"settings", [this](const Json& a_args) { return Settings(_organizer, _settingsManager, a_args); }},
    };
}

ToolRegistry::Handler Host::DirectHandler(const std::string& a_name) {
    if (a_name == "inspect") {
        return [this](const Json& a_args) {
            if (a_args.value("action", "health") == "bridge") {
                return _runtime.Bridge();
            }
            auto result = _runtime.Identity();
            result.update(_main.Health());
            return result;
        };
    }
    if (a_name == "events") {
        return [this](const Json& a_args) {
            const auto since = a_args.value("since", static_cast<std::uint64_t>(0));
            return a_args.value("action", "poll") == "wait" ? _events.Wait(since, a_args.value("timeoutMs", 30000))
                                                            : _events.Since(since);
        };
    }
    if (a_name == "scenario") {
        return [this](const Json& a_args) { return Scenario(_registry, _events, a_args); };
    }
    if (a_name == "capture") {
        return [this](const Json& a_args) { return _capture.Invoke(a_args); };
    }
    if (a_name == "plugins") {
        return [this](const Json& a_args) { return Plugins(_organizer, _main, a_args); };
    }
    return {};
}

void Host::RefreshCaptureTool() {
    _registry.Add(CaptureTool(ToolExtensions::Keys("capture")), DirectHandler("capture"));
}
}
