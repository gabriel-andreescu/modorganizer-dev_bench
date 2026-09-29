#pragma once
#include "Json.h"
#include <QObject>
#include <functional>

namespace Bench {
class EventBus;

class NativeOperation : public QObject {
public:
    NativeOperation(EventBus& a_events, QObject* a_parent);
    [[nodiscard]] Json Status() const;
    Json Start(const Json& a_action, std::function<Json()> a_operation);

protected:
    void Begin(const Json& a_action);
    void Schedule(std::function<Json()> a_operation);

private:
    EventBus& _events;
    Json _state = {{"state", "idle"}};
};
}
