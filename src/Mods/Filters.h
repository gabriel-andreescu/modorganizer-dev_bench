#pragma once
#include "Json.h"
#include <functional>

namespace Bench {
Json ModFilters(const Json& a_arguments);
Json WithUnfilteredModList(const std::function<Json()>& a_operation);
}
