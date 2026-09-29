#pragma once
#include "Dialogs/Actions.h"
#include "EventBus.h"
#include "Json.h"
#include "Mods/Actions.h"
#include <QObject>

namespace Bench {
class Installation : public QObject {
public:
    Installation(
        MOBase::IOrganizer* a_organizer,
        ModActions& a_mods,
        DialogActions& a_dialogs,
        EventBus& a_events,
        QObject* a_parent
    );
    Json Invoke(const Json& a_args);

private:
    void Execute(const QString& a_archive, const QString& a_name, const QString& a_separator);
    void Place(const QString& a_name, const QString& a_separator);
    void CompleteAfterRefresh();
    void Complete();
    void WaitToPlace(const QString& a_name, const QString& a_separator);
    void Finish();
    MOBase::IOrganizer* _organizer;
    ModActions& _mods;
    DialogActions& _dialogs;
    EventBus& _events;
    Json _state = {{"status", "idle"}};
    bool _active = false;
};
}
