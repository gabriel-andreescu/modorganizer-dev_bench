#pragma once

#include "Json.h"
#include "Native/DialogWorkflow.h"
#include <uibase/imoinfo.h>

namespace Bench {
class ExecutableManager : public NativeDialogWorkflow {
public:
    ExecutableManager(MOBase::IOrganizer* a_organizer, EventBus& a_events, QObject* a_parent);
    Json Invoke(const Json& a_args);

private:
    MOBase::IOrganizer* _organizer;
};
}
