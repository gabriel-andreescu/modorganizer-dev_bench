#include "MainThread.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QObject>
#include <QTimer>
#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <qnamespace.h>
#include <utility>

namespace Bench {
MainThread::MainThread(QObject* a_parent)
    : QObject(a_parent) {
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { ++_heartbeat; });
    timer->start(250);
}

void MainThread::Drain() {
    std::function<void()> work;
    {
        const std::scoped_lock lock(_mutex);
        if (_queue.empty()) {
            return;
        }
        work = std::move(_queue.front());
        _queue.pop_front();
    }
    work();
}

Json MainThread::Run(std::function<Json()> a_work) {
    if (_stopped) {
        throw ToolError(503, "MO2 is shutting down");
    }

    struct Task {
        std::promise<Json> result;
        std::atomic<bool> claimed = false;
    };

    const auto task = std::make_shared<Task>();
    auto future = task->result.get_future();
    ++_pending;
    {
        const std::scoped_lock lock(_mutex);
        _queue.emplace_back([this, task, work = std::move(a_work)] {
            --_pending;
            if (task->claimed.exchange(true) || _stopped) {
                return;
            }
            try {
                task->result.set_value(work());
            } catch (...) {
                task->result.set_exception(std::current_exception());
            }
            ++_completed;
        });
    }
    QMetaObject::invokeMethod(this, "Drain", Qt::QueuedConnection);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (future.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready) {
        if (_stopped) {
            task->claimed = true;
            throw ToolError(503, "MO2 is shutting down");
        }
        if (std::chrono::steady_clock::now() < deadline) {
            continue;
        }
        if (!task->claimed.exchange(true)) {
            throw ToolError(504, "UI task timed out before execution and was cancelled");
        }
        throw ToolError(504, "UI operation is still executing. Inspect state before retrying");
    }
    return future.get();
}

void MainThread::AwaitTimer(std::chrono::milliseconds a_delay) {
    const auto fired = std::make_shared<std::promise<void>>();
    const auto future = fired->get_future();
    Run([this, a_delay, fired] {
        QTimer::singleShot(a_delay, this, [fired] { fired->set_value(); });
        return Json();
    });
    while (future.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready) {
        if (_stopped) {
            throw ToolError(503, "MO2 is shutting down");
        }
    }
}

Json MainThread::Health() const {
    return {{"heartbeat", _heartbeat.load()}, {"pendingTasks", _pending.load()}, {"completedTasks", _completed.load()}};
}

void MainThread::Stop() {
    _stopped = true;
}
}
