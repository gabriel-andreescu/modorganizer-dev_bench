#pragma once
#include "Json.h"

namespace MOBase {
class IOrganizer;
}

namespace Bench {
class ProcessTracker;
class ExecutableManager;
Json Executables(
    MOBase::IOrganizer* a_organizer,
    ProcessTracker& a_processes,
    ExecutableManager& a_editor,
    const Json& a_args
);
}
