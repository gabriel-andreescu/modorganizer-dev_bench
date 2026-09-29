#pragma once

#include "EventBus.h"
#include "Json.h"
#include <QObject>
#include <QVariant>
#include <map>
#include <string>
#include <uibase/imodrepositorybridge.h>
#include <uibase/imoinfo.h>

namespace Bench {
class ModActions;

class NexusActions : public QObject {
public:
    NexusActions(MOBase::IOrganizer* a_organizer, const ModActions& a_mods, EventBus& a_events, QObject* a_parent);
    Json Invoke(const Json& a_args);

private:
    void ConnectResponses();
    void Complete(const QVariant& a_id, const QVariant& a_data);
    Json Start(const Json& a_args);
    [[nodiscard]] Json Cached(const QString& a_name) const;

    MOBase::IOrganizer* _organizer;
    const ModActions& _mods;
    MOBase::IModRepositoryBridge* _bridge;
    EventBus& _events;
    std::map<std::string, Json> _requests;
};
}
