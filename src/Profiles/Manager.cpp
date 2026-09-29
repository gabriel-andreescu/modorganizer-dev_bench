#include "Profiles/Manager.h"
#include "Json.h"
#include "Profiles/Editor.h"
#include "Tools/Registry.h"
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QObject>
#include <QWidget>
#include <map>
#include <qnamespace.h>
#include <string>

namespace Bench {
namespace {
    Json Operate(const QWidget* a_dialog, const Json& a_args) {
        const auto action = a_args.at("action").get<std::string>();
        if (action != "create" && action != "manage") {
            auto* list = a_dialog->findChild<QListWidget*>("profilesList");
            const auto items = list->findItems(Text(a_args.at("name")), Qt::MatchExactly);
            if (items.isEmpty()) {
                throw ToolError(404, "Unknown profile in the native profile manager");
            }
            list->setCurrentItem(items.front());
        }

        if (action == "manage") {
            return ReadProfileEditor(a_dialog);
        }
        if (action == "setLocalSaves" || action == "setLocalSettings") {
            auto* checkbox = a_dialog->findChild<QCheckBox*>(
                action == "setLocalSaves" ? "localSavesBox" : "localIniFilesBox"
            );
            const bool enabled = a_args.at("enabled");
            if (checkbox->isChecked() != enabled) {
                checkbox->click();
            }
            auto result = ReadProfileEditor(a_dialog);
            result["enabled"] = checkbox->isChecked();
            return result;
        }

        const std::map<std::string, const char*> buttons {
            {"create", "addProfileButton"},
            {"copy", "copyProfileButton"},
            {"rename", "renameButton"},
            {"remove", "removeProfileButton"},
            {"transferSaves", "transferButton"},
        };
        auto* button = a_dialog->findChild<QAbstractButton*>(buttons.at(action));
        if (!button->isEnabled()) {
            throw ToolError(409, "MO2 disabled this profile operation. Inspect the profile manager");
        }
        const auto before = ReadProfileEditor(a_dialog);
        button->click();
        auto result = ReadProfileEditor(a_dialog);
        result["changed"] = before != result;
        return result;
    }
}

Json ProfileManager::Invoke(const Json& a_args, QComboBox* a_picker) {
    if (a_args.at("action") == "operationStatus") {
        return Status();
    }
    return Run("ProfilesDialog", a_args, [a_picker] { a_picker->setCurrentIndex(0); }, Operate);
}
}
