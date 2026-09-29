#pragma once
#include "Json.h"

namespace MOBase {
class IOrganizer;
}

namespace Bench {
class ProfileManager;
Json Profiles(const MOBase::IOrganizer* a_organizer, ProfileManager& a_manager, const Json& a_args);
}
