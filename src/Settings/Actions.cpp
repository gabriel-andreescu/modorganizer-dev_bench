#include "Settings/Actions.h"
#include "Json.h"
#include "Settings/Manager.h"
#include "Settings/PluginSettings.h"
#include "Settings/Sections.h"
#include "Tools/Registry.h"
#include <uibase/imoinfo.h>

namespace Bench {
Json Settings(MOBase::IOrganizer* a_organizer, SettingsManager& a_manager, const Json& a_args) {
    const auto action = a_args.value("action", "list");
    if (action == "list" && !a_args.contains("section")) {
        return {{"sections", SettingsSections()}};
    }
    if (action == "refresh") {
        a_organizer->refresh();
        return {{"refreshPending", true}};
    }
    if (a_args.value("section", "") == "plugins" && (action == "list" || action == "get" || action == "set")) {
        return PluginSettings(a_organizer, a_args);
    }
    if ((action == "get" || action == "set" || action == "list") && !a_args.contains("section")) {
        throw ToolError(400, "Choose a section from settings action=list");
    }
    return a_manager.Invoke(a_args);
}
}
