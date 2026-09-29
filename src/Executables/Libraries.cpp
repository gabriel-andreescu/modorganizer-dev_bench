#include "Executables/Libraries.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QSettings>
#include <QString>
#include <QWidget>
#include <cstddef>
#include <uibase/imoinfo.h>
#include <uibase/iplugingame.h>

namespace Bench {
namespace {
    void Store(QSettings& a_settings, const QString& a_prefix, const Json& a_args) {
        for (const auto* widget : QApplication::topLevelWidgets()) {
            if (widget->isVisible() && QString(widget->metaObject()->className()) == "EditExecutablesDialog") {
                throw ToolError(409, "Close the executable editor before replacing persisted library settings");
            }
        }
        const auto& libraries = a_args.at("libraries");
        a_settings.beginWriteArray(a_prefix, static_cast<int>(libraries.size()));
        for (std::size_t row = 0; row < libraries.size(); ++row) {
            a_settings.setArrayIndex(static_cast<int>(row));
            const auto& entry = libraries[row];
            a_settings.setValue("process", Text(entry.at("process")));
            a_settings.setValue("library", Text(entry.at("library")));
            a_settings.setValue("enabled", entry.at("enabled").get<bool>());
        }
        a_settings.endArray();
        if (a_args.contains("enabled")) {
            a_settings.setValue(a_prefix + "/enabled", a_args.at("enabled").get<bool>());
        }
        a_settings.sync();
        if (a_settings.status() != QSettings::NoError) {
            throw ToolError(500, "Could not persist profile library settings");
        }
    }
}

Json ExecutableLibraries(const MOBase::IOrganizer* a_organizer, const Json& a_args) {
    const auto name = Text(a_args.at("name"));
    QSettings settings(a_organizer->profilePath() + "/settings.ini", QSettings::IniFormat);
    const auto prefix = "forced_libraries/" + name;
    if (a_args.at("action") == "setLibraries") {
        Store(settings, prefix, a_args);
    }
    Json libraries = Json::array();
    const int count = settings.beginReadArray(prefix);
    for (int row = 0; row < count; ++row) {
        settings.setArrayIndex(row);
        libraries.push_back({
            {"process", Utf8(settings.value("process").toString())},
            {"library", Utf8(settings.value("library").toString())},
            {"enabled", settings.value("enabled", false).toBool()},
        });
    }
    settings.endArray();
    Json defaults = Json::array();
    for (const auto& entry : a_organizer->managedGame()->executableForcedLoads()) {
        defaults.push_back({
            {"process", Utf8(entry.process())},
            {"library", Utf8(entry.library())},
            {"enabled", entry.enabled()},
            {"forced", entry.forced()},
        });
    }
    return {
        {"name", Utf8(name)},
        {"profile", Utf8(a_organizer->profileName())},
        {"enabled", settings.value(prefix + "/enabled", true).toBool()},
        {"libraries", libraries},
        {"gameDefaults", defaults},
    };
}
}
