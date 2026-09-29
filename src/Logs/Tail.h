#pragma once

#include "Json.h"
#include <QIODevice>
#include <cstdint>

namespace Bench {
Json ReadLogTail(QIODevice& a_device, std::uint64_t a_lines);
}
