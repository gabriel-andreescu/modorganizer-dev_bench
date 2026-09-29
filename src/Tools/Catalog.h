#pragma once
#include "Json.h"
#include <string>
#include <vector>

namespace Bench {
Json Catalog();
// The capture descriptor lists registered provider keys as kinds.
Json CaptureTool(const std::vector<std::string>& a_providers);
}
