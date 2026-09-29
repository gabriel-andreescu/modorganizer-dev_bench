#include "Settings/Manager.h"
#include "Dialogs/Actions.h"
#include "EventBus.h"
#include "Json.h"
#include "Native/DialogWorkflow.h"
#include "Native/WidgetValue.h"
#include "Settings/Sections.h"
#include "Tools/Registry.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QTabWidget>

namespace Bench {
namespace {
    void SelectSection(QTabWidget* a_tabs, const QString& a_section) {
        const auto name = SettingsTab(a_section);
        for (int row = 0; row < a_tabs->count(); ++row) {
            if (a_tabs->widget(row)->objectName() != name) {
                continue;
            }
            if (!a_tabs->isTabEnabled(row)) {
                throw ToolError(409, "MO2 disabled this settings section");
            }
            a_tabs->setCurrentIndex(row);
            return;
        }
        throw ToolError(409, "Settings section is not available in the native dialog");
    }

    Json UpdateSection(const QWidget* a_section, const Json& a_values) {
        Json changed = Json::object();
        for (const auto& [name, value] : a_values.items()) {
            const auto matches = a_section->findChildren<QWidget*>(QString::fromStdString(name));
            if (matches.size() != 1) {
                throw ToolError(400, "Setting objectName must identify one control in this section: " + name);
            }
            changed[name] = SetWidgetValue(matches.front(), value);
        }
        return changed;
    }
}

SettingsManager::SettingsManager(DialogActions& a_dialogs, EventBus& a_events, QObject* a_parent)
    : NativeDialogWorkflow(a_events, a_parent)
    , _dialogs(a_dialogs) {}

Json SettingsManager::Operate(QWidget* a_dialog, const Json& a_args) {
    const auto action = a_args.at("action").get<std::string>();
    if (action == "apply" || action == "cancel") {
        auto* dialog = qobject_cast<QDialog*>(a_dialog);
        if (action == "apply") {
            dialog->accept();
        } else {
            dialog->reject();
        }
        return {{"dialogOpen", dialog->isVisible()}, {"applied", action == "apply" && !dialog->isVisible()}};
    }

    auto* tabs = a_dialog->findChild<QTabWidget*>("tabWidget");
    if (a_args.contains("section")) {
        SelectSection(tabs, Text(a_args.at("section")));
    }

    Json changed = Json::object();
    if (action == "set") {
        changed = UpdateSection(tabs->currentWidget(), a_args.at("values"));
    }
    return {{"values", changed}, {"dialog", _dialogs.Describe()}, {"staged", action == "set"}};
}

Json SettingsManager::Invoke(const Json& a_args) {
    if (a_args.at("action") == "operationStatus") {
        return Status();
    }
    QAction* open = nullptr;
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (const auto* window = qobject_cast<QMainWindow*>(widget)) {
            open = window->findChild<QAction*>("actionSettings");
            if (open != nullptr) {
                break;
            }
        }
    }
    if ((open == nullptr) || !open->isEnabled()) {
        throw ToolError(409, "MO2 settings are not available");
    }
    return Run(
        "SettingsDialog",
        a_args,
        [open] { open->trigger(); },
        [this](QWidget* a_dialog, const Json& a_request) { return Operate(a_dialog, a_request); }
    );
}
}
