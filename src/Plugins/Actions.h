#pragma once
#include "Json.h"
#include <uibase/imoinfo.h>

namespace Bench {
class MainThread;

Json Plugins(const MOBase::IOrganizer* a_organizer, MainThread& a_main, const Json& a_args);
}
