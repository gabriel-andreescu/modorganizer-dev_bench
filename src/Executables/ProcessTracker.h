#pragma once
#include "EventBus.h"
#include "Json.h"
#include <QObject>
#include <memory>
#include <vector>
#include <windows.h>

namespace Bench {
class ProcessTracker : public QObject {
public:
    ProcessTracker(EventBus& a_events, QObject* a_parent);
    Json Add(HANDLE a_handle, const QString& a_binary);
    Json Status();

private:
    struct Process {
        std::unique_ptr<void, decltype(&CloseHandle)> handle {nullptr, &CloseHandle};
        Json data;
    };

    void Poll();
    EventBus& _events;
    std::vector<Process> _processes;
};
}
