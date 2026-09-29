#include "Settings/Sections.h"
#include "Json.h"
#include <QString>
#include <array>
#include <stdexcept>

namespace Bench {
namespace {
    struct Section {
        const char* id;
        const char* label;
        const char* tab;
    };

    constexpr std::array<Section, 8> kSections {
        {
            {.id = "general", .label = "General", .tab = "generalTab"},
            {.id = "theme", .label = "Theme", .tab = "tab"},
            {.id = "modList", .label = "Mod List", .tab = "uiTab"},
            {.id = "paths", .label = "Paths", .tab = "pathsTab"},
            {.id = "nexus", .label = "Nexus", .tab = "nexusTab"},
            {.id = "plugins", .label = "Plugins", .tab = "pluginsTab"},
            {.id = "workarounds", .label = "Workarounds", .tab = "workaroundTab"},
            {.id = "diagnostics", .label = "Diagnostics", .tab = "diagnosticsTab"},
        },
    };
}

Json SettingsSections() {
    Json result = Json::array();
    for (const auto& section : kSections) {
        result.push_back({
            {"id", section.id},
            {"label", section.label},
            {"editing", section.id == QString("plugins") ? "live plugin API" : "native dialog. Apply or cancel"},
        });
    }
    return result;
}

QString SettingsTab(const QString& a_section) {
    for (const auto& section : kSections) {
        if (a_section == section.id) {
            return section.tab;
        }
    }
    throw std::invalid_argument("Unknown settings section");
}
}
