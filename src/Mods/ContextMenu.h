#pragma once
#include <QString>

namespace MOBase {
class IModList;
}

namespace Bench {
void InvokeModMenu(const MOBase::IModList* a_list, const QString& a_name, const char* a_action);
}
