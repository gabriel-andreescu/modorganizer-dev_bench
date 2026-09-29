#include "Executables/ProcessTracker.h"
#include "EventBus.h"
#include "Json.h"
#include <QTimer>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <qobject.h>
#include <synchapi.h>
#include <utility>
#include <winbase.h>
#include <winnt.h>

namespace Bench {
ProcessTracker::ProcessTracker(EventBus& a_events, QObject* a_parent)
    : QObject(a_parent)
    , _events(a_events) {
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { Poll(); });
    timer->start(250);
}

Json ProcessTracker::Add(HANDLE a_handle, const QString& a_binary) {
    Process process;
    process.handle.reset(a_handle);
    process.data = {{"pid", GetProcessId(a_handle)}, {"binary", Utf8(a_binary)}, {"running", true}};
    _processes.push_back(std::move(process));
    _events.Publish("executables.started", _processes.back().data);
    return _processes.back().data;
}

void ProcessTracker::Poll() {
    for (auto& process : _processes) {
        if (!process.handle || WaitForSingleObject(process.handle.get(), 0) != WAIT_OBJECT_0) {
            continue;
        }
        DWORD exitCode = 0;
        GetExitCodeProcess(process.handle.get(), &exitCode);
        process.handle.reset();
        process.data["running"] = false;
        process.data["exitCode"] = exitCode;
        _events.Publish("executables.finished", process.data);
    }
}

Json ProcessTracker::Status() {
    Poll();
    Json result = Json::array();
    for (const auto& process : _processes) {
        result.push_back(process.data);
    }
    return {{"processes", result}};
}
}
