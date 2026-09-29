#pragma once
#include "Json.h"
#include <array>
#include <utility>
class QModelIndex;

namespace Bench {
inline constexpr auto kModConflictFlags = std::to_array<std::pair<const char*, const char*>>({
    {"overwritesLoose", "Overwrites loose files"},
    {"overwrittenLoose", "Overwritten loose files"},
    {"mixedLoose", "Loose files Overwrites & Overwritten"},
    {"redundant", "Redundant"},
    {"looseOverwritesArchive", "Overwrites an archive with loose files"},
    {"archiveOverwrittenByLoose", "Archive is overwritten by loose files"},
    {"overwritesArchive", "Overwrites another archive file"},
    {"overwrittenArchive", "Overwritten by another archive file"},
    {"mixedArchive", "Archive files overwrites & overwritten"},
});

Json ReadModConflictFlags(const QModelIndex& a_index);
}
