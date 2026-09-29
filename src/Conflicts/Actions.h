#pragma once
#include "Native/Operation.h"

namespace MOBase {
class IOrganizer;
}

namespace Bench {
class ConflictActions : public NativeOperation {
public:
    ConflictActions(MOBase::IOrganizer* a_organizer, EventBus& a_events, QObject* a_parent)
        : NativeOperation(a_events, a_parent)
        , _organizer(a_organizer) {}

    Json Invoke(const Json& a_arguments);

private:
    MOBase::IOrganizer* _organizer;
};
}
