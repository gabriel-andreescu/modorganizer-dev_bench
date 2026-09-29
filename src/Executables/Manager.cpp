#include "Executables/Manager.h"
#include "EventBus.h"
#include "Executables/Editor.h"
#include "Executables/Libraries.h"
#include "Json.h"
#include "Native/DialogWorkflow.h"
#include "Tools/Registry.h"
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QMenu>
#include <QObject>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>
#include <format>
#include <map>
#include <qnamespace.h>
#include <string>
#include <uibase/imoinfo.h>

namespace Bench {
namespace {
    Json Operate(const MOBase::IOrganizer* a_organizer, QWidget* a_dialog, const Json& a_args) {
        const auto action = a_args.at("action").get<std::string>();
        if (a_args.contains("name")) {
            auto* list = a_dialog->findChild<QListWidget*>("list");
            const auto items = list->findItems(Text(a_args.at("name")), Qt::MatchExactly);
            if (items.isEmpty()) {
                throw ToolError(
                    404,
                    std::format("Unknown executable title in the native editor: {}", Utf8(Text(a_args.at("name"))))
                );
            }
            list->setCurrentItem(items.front());
        }

        if (action == "libraries" || action == "setLibraries") {
            qobject_cast<QDialog*>(a_dialog)->reject();
            return ExecutableLibraries(a_organizer, a_args);
        }

        if (action == "snapshot") {
            auto result = ReadExecutableEditor(a_dialog);
            qobject_cast<QDialog*>(a_dialog)->reject();
            result["source"] = "live MO2 executable configuration";
            return result;
        }

        if (action == "create" || action == "clone" || action == "addFromFile") {
            const auto* menu = a_dialog->findChild<QToolButton*>("add")->menu();
            const std::map<std::string, int> indices {{"create", 1}, {"clone", 2}, {"addFromFile", 0}};
            menu->actions().at(indices.at(action))->trigger();
        } else if (action == "remove" || action == "reset" || action == "configureLibraries") {
            auto* button = a_dialog->findChild<QAbstractButton*>(QString::fromStdString(action));
            if (!button->isEnabled()) {
                throw ToolError(409, "MO2 disabled this executable operation");
            }
            button->click();
        } else if (action == "setOrder") {
            OrderExecutableEditor(a_dialog, a_args.at("names"));
        } else if (action == "apply" || action == "accept" || action == "cancel") {
            const std::map<std::string, QDialogButtonBox::StandardButton> buttons {
                {"apply", QDialogButtonBox::Apply},
                {"accept", QDialogButtonBox::Ok},
                {"cancel", QDialogButtonBox::Cancel},
            };
            a_dialog->findChild<QDialogButtonBox*>("buttons")->button(buttons.at(action))->click();
            return {
                {"dialogOpen", a_dialog->isVisible()},
                {
                    "unappliedChanges",
                    a_dialog->findChild<QDialogButtonBox*>("buttons")->button(QDialogButtonBox::Apply)->isEnabled(),
                },
                {"configurationSource", "live MO2 state. The executable INI snapshot may lag until shutdown"},
            };
        }
        if (a_args.contains("values")) {
            UpdateExecutableEditor(a_dialog, a_args.at("values"));
        }
        return ReadExecutableEditor(a_dialog);
    }
}

ExecutableManager::ExecutableManager(MOBase::IOrganizer* a_organizer, EventBus& a_events, QObject* a_parent)
    : NativeDialogWorkflow(a_events, a_parent)
    , _organizer(a_organizer) {}

Json ExecutableManager::Invoke(const Json& a_args) {
    if (a_args.at("action") == "operationStatus") {
        return Status();
    }
    if (a_args.at("action") == "libraries"
        || a_args.at("action") == "setLibraries"
        || a_args.at("action") == "snapshot") {
        for (const auto* widget : QApplication::topLevelWidgets()) {
            if (widget->isVisible() && QString(widget->metaObject()->className()) == "EditExecutablesDialog") {
                throw ToolError(409, "Close the executable editor first. Use editorState to inspect staged edits");
            }
        }
    }
    QComboBox* picker = nullptr;
    for (auto* widget : QApplication::allWidgets()) {
        if (widget->objectName() == "executablesListBox") {
            picker = qobject_cast<QComboBox*>(widget);
        }
    }
    if (picker == nullptr) {
        throw ToolError(409, "Executable picker is not ready");
    }
    return Run(
        "EditExecutablesDialog",
        a_args,
        [picker] { picker->setCurrentIndex(0); },
        [this](QWidget* a_dialog, const Json& a_request) { return Operate(_organizer, a_dialog, a_request); }
    );
}
}
