#pragma once
#include <QString>

namespace Bench {
// Removes the discovery records in a directory whose process no longer runs. A killed MO2 never
// removes its own.
void RemoveStaleRecords(const QString& a_directory);
}
