#include "Logs/Tail.h"
#include "Json.h"
#include <QByteArray>
#include <QString>
#include <cstdint>
#include <deque>
#include <utility>

namespace Bench {
Json ReadLogTail(QIODevice& a_device, std::uint64_t a_lines) {
    std::deque<QByteArray> lines;
    std::uint64_t total = 0;
    auto remaining = a_device.bytesAvailable();
    while (remaining > 0) {
        auto line = a_device.readLine(remaining + 1);
        if (line.isEmpty()) {
            break;
        }
        remaining -= line.size();
        ++total;
        if (line.endsWith('\n')) {
            line.chop(1);
            if (line.endsWith('\r')) {
                line.chop(1);
            }
        }
        lines.push_back(std::move(line));
        if (lines.size() > a_lines) {
            lines.pop_front();
        }
    }
    Json result = Json::array();
    for (const auto& line : lines) {
        result.push_back(Utf8(QString::fromUtf8(line)));
    }
    return {
        {"lines", result},
        {"requested", a_lines},
        {"totalLines", total},
        {"returned", result.size()},
        {"truncated", total > result.size()},
    };
}
}
