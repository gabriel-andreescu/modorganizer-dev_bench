#pragma once
#include "Json.h"

namespace MOBase {
class IOrganizer;
}

namespace Bench {
Json InspectModConflicts(const MOBase::IOrganizer* a_organizer, const Json& a_arguments);
}
