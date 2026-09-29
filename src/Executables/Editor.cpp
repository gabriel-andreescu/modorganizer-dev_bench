#include "Executables/Editor.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QObject>
#include <QWidget>
#include <array>
#include <qnamespace.h>
#include <set>
#include <string>
#include <vector>

namespace Bench {
namespace {
    constexpr std::array kTextFields {"title", "binary", "workingDirectory", "arguments", "steamAppID"};
    constexpr std::array kToggleFields {
        "overwriteSteamAppID",
        "createFilesInMod",
        "forceLoadLibraries",
        "useApplicationIcon",
        "minimizeToSystemTray",
        "hide",
    };

    void SetToggle(const QWidget* a_dialog, const Json& a_values, const char* a_name) {
        if (a_values.contains(a_name)) {
            auto* field = a_dialog->findChild<QCheckBox*>(a_name);
            if (field == nullptr) {
                throw ToolError(400, std::string("Executable field is unavailable in this MO2 version: ") + a_name);
            }
            field->setChecked(a_values.at(a_name).get<bool>());
        }
    }

    Json ReadFields(const QWidget* a_dialog) {
        Json result = Json::object();
        for (const auto* name : kTextFields) {
            result[name] = Utf8(a_dialog->findChild<QLineEdit*>(name)->text());
        }
        for (const auto* name : kToggleFields) {
            if (const auto* field = a_dialog->findChild<QCheckBox*>(name)) {
                result[name] = field->isChecked();
            }
        }
        result["outputMod"] = Utf8(a_dialog->findChild<QComboBox*>("mods")->currentText());
        return result;
    }
}

Json ReadExecutableEditor(const QWidget* a_dialog) {
    auto* list = a_dialog->findChild<QListWidget*>("list");
    const int previous = list->currentRow();
    Json entries = Json::array();
    for (int row = 0; row < list->count(); ++row) {
        list->setCurrentRow(row);
        auto entry = ReadFields(a_dialog);
        entry["priority"] = row;
        entries.push_back(entry);
    }
    list->setCurrentRow(previous);
    Json constraints = Json::array();
    if (a_dialog->findChild<QCheckBox*>("minimizeToSystemTray") != nullptr) {
        constraints.push_back({
            {"fields", {"minimizeToSystemTray", "useApplicationIcon"}},
            {
                "description",
                "MO2 2.5.3beta12 overlaps these native flags. Saving sets useApplicationIcon to the minimizeToSystemTray value.",
            },
        });
    }
    return {
        {"entries", entries},
        {"selectedIndex", previous},
        {"constraints", constraints},
        {"source", "native editor. Apply to persist changes"},
    };
}

void UpdateExecutableEditor(const QWidget* a_dialog, const Json& a_values) {
    for (const auto* name : kToggleFields) {
        SetToggle(a_dialog, a_values, name);
    }

    for (const auto* name : kTextFields) {
        if (!a_values.contains(name)) {
            continue;
        }
        auto* field = a_dialog->findChild<QLineEdit*>(name);
        if (!field->isEnabled()) {
            throw ToolError(409, std::string("Enable the associated override before setting ") + name);
        }
        field->setText(Text(a_values.at(name)));
        if (QString(name) == "title") {
            QMetaObject::invokeMethod(field, "editingFinished", Qt::DirectConnection);
            if (field->text() != Text(a_values.at(name)).trimmed()) {
                throw ToolError(409, "MO2 rejected the executable title. Use a unique, nonempty title");
            }
        }
    }
    if (a_values.contains("outputMod")) {
        auto* mods = a_dialog->findChild<QComboBox*>("mods");
        if (!mods->isEnabled()) {
            throw ToolError(409, "Enable createFilesInMod before setting outputMod");
        }
        const auto index = mods->findText(Text(a_values.at("outputMod")), Qt::MatchExactly);
        if (index < 0) {
            throw ToolError(404, "Unknown executable output mod");
        }
        mods->setCurrentIndex(index);
    }
}

void OrderExecutableEditor(const QWidget* a_dialog, const Json& a_names) {
    auto* list = a_dialog->findChild<QListWidget*>("list");
    std::set<std::string> current;
    for (int row = 0; row < list->count(); ++row) {
        current.insert(Utf8(list->item(row)->text()));
    }
    const auto requested = a_names.get<std::vector<std::string>>();
    if (requested.size() != current.size() || std::set<std::string>(requested.begin(), requested.end()) != current) {
        throw ToolError(400, "names must contain every executable title exactly once");
    }
    auto* upButton = a_dialog->findChild<QAbstractButton*>("up");
    for (int row = 0; row < list->count(); ++row) {
        const auto items = list->findItems(QString::fromStdString(requested[row]), Qt::MatchExactly);
        list->setCurrentItem(items.front());
        while (list->currentRow() > row) {
            upButton->click();
        }
    }
}
}
