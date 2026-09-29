#include "EventBus.h"
#include "Json.h"
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

namespace Bench {
void EventBus::Publish(const std::string& a_topic, Json a_data) {
    const std::scoped_lock lock(_mutex);
    _events.push_back({{"seq", ++_sequence}, {"topic", a_topic}, {"data", std::move(a_data)}});
    if (_events.size() > 1024) {
        _events.pop_front();
    }
    _changed.notify_all();
}

Json EventBus::Snapshot(std::uint64_t a_sequence) const {
    Json result = Json::array();
    for (const auto& event : _events) {
        if (event["seq"].get<std::uint64_t>() > a_sequence) {
            result.push_back(event);
        }
    }
    return {
        {"headSeq", _sequence},
        {"stopped", _stopped},
        {"events", result},
        {"gap", !_events.empty() && a_sequence + 1 < _events.front()["seq"].get<std::uint64_t>()},
    };
}

Json EventBus::Since(std::uint64_t a_sequence) const {
    const std::scoped_lock lock(_mutex);
    return Snapshot(a_sequence);
}

std::uint64_t EventBus::HeadSequence() const {
    const std::scoped_lock lock(_mutex);
    return _sequence;
}

Json EventBus::Wait(std::uint64_t a_sequence, int a_timeoutMs) {
    std::unique_lock lock(_mutex);
    _changed.wait_for(lock, std::chrono::milliseconds(a_timeoutMs), [&] { return _stopped || _sequence > a_sequence; });
    return Snapshot(a_sequence);
}

void EventBus::Stop() {
    const std::scoped_lock lock(_mutex);
    _stopped = true;
    _changed.notify_all();
}
}
