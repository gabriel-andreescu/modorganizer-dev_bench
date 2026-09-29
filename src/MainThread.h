#pragma once
#include "Json.h"
#include <QObject>
#include <atomic>
#include <deque>
#include <functional>
#include <mutex>

namespace Bench {
class MainThread : public QObject {
    Q_OBJECT
public:
    explicit MainThread(QObject* a_parent);
    Json Run(std::function<Json()> a_work);
    Json Health() const;
    void Stop();

private:
    Q_SLOT void Drain();

    std::mutex _mutex;
    std::deque<std::function<void()>> _queue;
    std::atomic<bool> _stopped = false;
    std::atomic<int> _pending = 0;
    std::atomic<std::uint64_t> _completed = 0;
    std::atomic<std::uint64_t> _heartbeat = 0;
};
}
