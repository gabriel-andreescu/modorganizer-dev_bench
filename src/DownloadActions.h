#pragma once
#include "Json.h"

namespace MOBase {
class IOrganizer;
}

namespace Bench {
Json Downloads(const MOBase::IOrganizer* a_organizer, const Json& a_args);
}
