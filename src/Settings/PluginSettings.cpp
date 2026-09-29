#include "Settings/PluginSettings.h"
#include "Configuration.h"
#include "Json.h"
#include "Tools/Registry.h"
#include "VariantJson.h"
#include <QApplication>
#include <QSettings>
#include <QVariant>
#include <QWidget>
#include <string>
#include <uibase/imoinfo.h>

namespace Bench {
namespace {
    QVariant Value(const Json& a_value) {
        if (a_value.is_boolean()) {
            return a_value.get<bool>();
        }
        if (a_value.is_number_integer()) {
            return a_value.get<int>();
        }
        if (a_value.is_number_float()) {
            return a_value.get<double>();
        }
        return Text(a_value);
    }
}

Json PluginSettings(MOBase::IOrganizer* a_organizer, const Json& a_args) {
    const auto action = a_args.at("action").get<std::string>();
    if (action == "list") {
        QSettings settings(ConfigurationPath(a_organizer), QSettings::IniFormat);
        settings.beginGroup("Plugins");
        if (!a_args.contains("plugin")) {
            return {{"section", "plugins"}, {"plugins", Strings(settings.childGroups())}};
        }
        settings.beginGroup(Text(a_args.at("plugin")));
        return {{"section", "plugins"}, {"plugin", a_args.at("plugin")}, {"keys", Strings(settings.allKeys())}};
    }
    if (!a_args.contains("plugin") || !a_args.contains("key")) {
        throw ToolError(400, "Plugin setting get/set requires plugin and key. Use section=plugins action=list");
    }
    const auto plugin = Text(a_args.at("plugin"));
    const auto key = Text(a_args.at("key"));
    if (action == "set") {
        for (const auto* widget : QApplication::topLevelWidgets()) {
            if (widget->isVisible() && QString(widget->metaObject()->className()) == "SettingsDialog") {
                throw ToolError(409, "Close Settings first so its cached plugin values cannot overwrite this edit");
            }
        }
        a_organizer->setPluginSetting(plugin, key, Value(a_args.at("value")));
    }
    return {
        {"section", "plugins"},
        {"plugin", Utf8(plugin)},
        {"key", Utf8(key)},
        {"value", VariantJson(a_organizer->pluginSetting(plugin, key))},
        {"enabled", a_organizer->isPluginEnabled(plugin)},
        {"applied", action == "set"},
        {"staged", false},
    };
}
}
