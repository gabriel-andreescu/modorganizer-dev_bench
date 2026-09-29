#include "Profiles/Actions.h"
#include "Json.h"
#include "Profiles/Manager.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <format>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <uibase/imoinfo.h>
#include <uibase/iprofile.h>

namespace Bench {
Json Profiles(const MOBase::IOrganizer* a_organizer, ProfileManager& a_manager, const Json& a_args) {
    QComboBox* combo = nullptr;
    for (auto* widget : QApplication::allWidgets()) {
        if (widget->objectName() == "profileBox") {
            combo = qobject_cast<QComboBox*>(widget);
        }
    }
    const auto action = a_args.value("action", "list");
    if (action != "list" && action != "select") {
        if (combo == nullptr) {
            throw ToolError(409, "Profile picker is not ready");
        }
        return a_manager.Invoke(a_args, combo);
    }
    if (action == "select") {
        if (combo == nullptr) {
            throw ToolError(409, "Profile picker is not ready");
        }
        const auto index = combo->findText(Text(a_args.at("name")));
        if (index < 1) {
            throw ToolError(404, std::format("Unknown profile: {}", Utf8(Text(a_args.at("name")))));
        }
        combo->setCurrentIndex(index);
        QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, index));
    }
    Json profiles = Json::array();
    if (combo != nullptr) {
        for (int i = 1; i < combo->count(); ++i) {
            profiles.push_back(Utf8(combo->itemText(i)));
        }
    }
    const MOBase::IProfile& profile = *a_organizer->profile();
    return {
        {"profiles", profiles},
        {"current", Utf8(a_organizer->profileName())},
        {"path", Utf8(a_organizer->profilePath())},
        {"basePath", Utf8(a_organizer->basePath())},
        {"modsPath", Utf8(a_organizer->modsPath())},
        {"downloadsPath", Utf8(a_organizer->downloadsPath())},
        {"overwritePath", Utf8(a_organizer->overwritePath())},
        {"localSaves", profile.localSavesEnabled()},
        {"localSettings", profile.localSettingsEnabled()},
        {"version", Utf8(QCoreApplication::applicationVersion())},
    };
}
}
