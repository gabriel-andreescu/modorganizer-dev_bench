#include "Executables/Actions.h"
#include "Configuration.h"
#include "Executables/Manager.h"
#include "Executables/ProcessTracker.h"
#include "Json.h"
#include "Tools/Registry.h"
#include "VariantJson.h"
#include <QApplication>
#include <QComboBox>
#include <QSettings>
#include <handleapi.h>
#include <qobject.h>
#include <uibase/imoinfo.h>

namespace Bench {
namespace {
    Json Launch(MOBase::IOrganizer* a_organizer, ProcessTracker& a_processes, const Json& a_args) {
        auto* const handle = a_organizer->startApplication(
            Text(a_args.at("name")),
            StringList(a_args.value("args", Json::array())),
            Text(a_args.value("cwd", Json(""))),
            Text(a_args.value("profile", Json(""))),
            Text(a_args.value("overwrite", Json("")))
        );
        if ((handle == nullptr) || handle == INVALID_HANDLE_VALUE) {
            throw ToolError(409, "MO2 did not launch the application");
        }
        return a_processes.Add(handle, Text(a_args.at("name")));
    }

    Json ToolbarNames() {
        Json names = Json::array();
        for (auto* widget : QApplication::allWidgets()) {
            const auto* combo = qobject_cast<QComboBox*>(widget);
            if (combo == nullptr || combo->objectName() != "executablesListBox") {
                continue;
            }
            for (int i = 1; i < combo->count(); ++i) {
                names.push_back(Utf8(combo->itemText(i)));
            }
        }
        return names;
    }

    Json PersistedConfigurations(const MOBase::IOrganizer* a_organizer) {
        QSettings settings(ConfigurationPath(a_organizer), QSettings::IniFormat);
        Json configs = Json::array();
        const auto count = settings.beginReadArray("customExecutables");
        for (int i = 0; i < count; ++i) {
            settings.setArrayIndex(i);
            Json config = Json::object();
            for (const auto& key : settings.allKeys()) {
                config[Utf8(key)] = VariantJson(settings.value(key));
            }
            configs.push_back(config);
        }
        settings.endArray();
        return configs;
    }

    Json List(const MOBase::IOrganizer* a_organizer) {
        return {
            {"tools", ToolbarNames()},
            {"configurations", PersistedConfigurations(a_organizer)},
            {"configurationSource", "persisted ModOrganizer.ini. Unsaved executable-dialog edits are not included"},
        };
    }
}

Json Executables(
    MOBase::IOrganizer* a_organizer,
    ProcessTracker& a_processes,
    ExecutableManager& a_editor,
    const Json& a_args
) {
    const auto action = a_args.value("action", "list");
    if (action == "list") {
        return List(a_organizer);
    }
    if (action == "status") {
        return a_processes.Status();
    }
    if (action == "launch") {
        return Launch(a_organizer, a_processes, a_args);
    }
    return a_editor.Invoke(a_args);
}
}
