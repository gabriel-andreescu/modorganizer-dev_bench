#pragma once
#include "Json.h"
#include <condition_variable>
#include <deque>
#include <mutex>

namespace Bench {
class EventBus {
public:
    void Publish(const std::string& a_topic, Json a_data);
    Json Since(std::uint64_t a_sequence) const;
    [[nodiscard]] std::uint64_t HeadSequence() const;
    Json Wait(std::uint64_t a_sequence, int a_timeoutMs);
    void Stop();

private:
    Json Snapshot(std::uint64_t a_sequence) const;
    mutable std::mutex _mutex;
    std::condition_variable _changed;
    std::deque<Json> _events;
    std::uint64_t _sequence = 0;
    bool _stopped = false;
};
}
