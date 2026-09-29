#pragma once
#include "Json.h"

namespace MOBase {
class IOrganizer;
}

namespace Bench {
class SettingsManager;
Json Settings(MOBase::IOrganizer* a_organizer, SettingsManager& a_manager, const Json& a_args);
}
