#pragma once
#include <QString>
#include <uibase/imoinfo.h>

namespace Bench {
QString ConfigurationPath(const MOBase::IOrganizer* a_organizer);
// Empty for a portable install, or when the active configuration cannot be identified.
QString InstanceName(const MOBase::IOrganizer* a_organizer);
}
