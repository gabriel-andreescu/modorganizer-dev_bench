#include "Installation.h"
#include "Dialogs/Actions.h"
#include "EventBus.h"
#include "Json.h"
#include "Mods/Actions.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <exception>
#include <qobject.h>
#include <uibase/imodinterface.h>
#include <uibase/imoinfo.h>

namespace Bench {
Installation::Installation(
    MOBase::IOrganizer* a_organizer,
    ModActions& a_mods,
    DialogActions& a_dialogs,
    EventBus& a_events,
    QObject* a_parent
)
    : QObject(a_parent)
    , _organizer(a_organizer)
    , _mods(a_mods)
    , _dialogs(a_dialogs)
    , _events(a_events) {}

void Installation::Execute(const QString& a_archive, const QString& a_name, const QString& a_separator) {
    try {
        const auto* mod = _organizer->installMod(a_archive, a_name);
        if (mod != nullptr) {
            const auto name = mod->name();
            _state["mod"] = Utf8(name);
            WaitToPlace(name, a_separator);
            return;
        }
        _state["status"] = "cancelled";
    } catch (const std::exception& error) {
        _state["status"] = "failed";
        _state["error"] = error.what();
    }
    Finish();
}

void Installation::WaitToPlace(const QString& a_name, const QString& a_separator) {
    const QPointer<Installation> self(this);
    _organizer->onNextRefresh([self, a_name, a_separator] {
        if (self) {
            QTimer::singleShot(0, self, [self, a_name, a_separator] { self->Place(a_name, a_separator); });
        }
    });
}

void Installation::Place(const QString& a_name, const QString& a_separator) {
    try {
        const auto previousPriority = _mods.Describe(a_name)["priority"];
        if (!a_separator.isEmpty()) {
            _mods.UnderSeparator(a_name, a_separator);
        }
        if (_mods.Describe(a_name)["priority"] == previousPriority) {
            Complete();
            return;
        }
        CompleteAfterRefresh();
    } catch (const std::exception& error) {
        _state["status"] = "failed";
        _state["error"] = error.what();
        Finish();
    }
}

void Installation::CompleteAfterRefresh() {
    const QPointer<Installation> self(this);
    _organizer->onNextRefresh([self] {
        if (self) {
            QTimer::singleShot(0, self, [self] { self->Complete(); });
        }
    });
}

void Installation::Complete() {
    _state["status"] = "completed";
    Finish();
}

void Installation::Finish() {
    _active = false;
    _events.Publish("install.finished", _state);
}

Json Installation::Invoke(const Json& a_args) {
    const auto action = a_args.value("action", "status");
    if (action == "start") {
        if (_active) {
            throw ToolError(409, "Installation already active. Inspect install status");
        }
        if (QApplication::activeModalWidget() != nullptr) {
            throw ToolError(409, "An MO2 modal dialog is already open. Inspect dialogs");
        }
        QString archive;
        if (a_args.contains("download")) {
            const auto download = Text(a_args["download"]);
            if (QFileInfo(download).fileName() != download) {
                throw ToolError(400, "download must be a filename from downloads list");
            }
            archive = QDir(_organizer->downloadsPath()).filePath(download);
        } else {
            archive = Text(a_args.at("path"));
        }
        if (!QFileInfo(archive).isAbsolute() || !QFileInfo(archive).isFile()) {
            throw ToolError(400, "Archive path must name an existing absolute file");
        }
        const auto separator = Text(a_args.value("separator", Json("")));
        if (!separator.isEmpty() && _mods.Describe(separator)["separator"] != true) {
            throw ToolError(400, "Target must be a separator");
        }
        const auto name = Text(a_args.value("name", Json("")));
        _state = {
            {"id", Utf8(QUuid::createUuid().toString(QUuid::WithoutBraces))},
            {"status", "running"},
            {"archive", Utf8(archive)},
            {"separator", Utf8(separator)},
        };
        _active = true;
        QTimer::singleShot(0, this, [this, archive, name, separator] { Execute(archive, name, separator); });
        _events.Publish("install.started", _state);
    } else if (action != "status") {
        throw ToolError(400, "Use install start/status and dialogs to answer the native installer");
    }
    auto result = _state;
    result["dialog"] = _dialogs.Describe();
    return result;
}
}
