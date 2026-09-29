#include "Native/Operation.h"
#include "EventBus.h"
#include "Json.h"
#include "Native/WidgetDialogScope.h"
#include "Tools/Registry.h"
#include <QObject>
#include <QTimer>
#include <exception>
#include <functional>
#include <utility>

namespace Bench {
NativeOperation::NativeOperation(EventBus& a_events, QObject* a_parent)
    : QObject(a_parent)
    , _events(a_events) {}

Json NativeOperation::Status() const {
    return _state;
}

void NativeOperation::Begin(const Json& a_action) {
    if (_state["state"] == "scheduled" || _state["state"] == "awaitingNativeDialog") {
        throw ToolError(409, "Finish the current native operation through dialogs first");
    }
    _state = {{"action", a_action}, {"state", "scheduled"}};
}

void NativeOperation::Schedule(std::function<Json()> a_operation) {
    QTimer::singleShot(0, this, [this, operation = std::move(a_operation)] {
        try {
            _state["state"] = "awaitingNativeDialog";
            const WidgetDialogScope dialogScope;
            _state["result"] = operation();
            QTimer::singleShot(0, this, [this] {
                _state["state"] = "finished";
                _events.Publish("ui.operation", _state);
            });
        } catch (const std::exception& error) {
            _state["state"] = "failed";
            _state["error"] = error.what();
            _events.Publish("ui.operation", _state);
        }
    });
}

Json NativeOperation::Start(const Json& a_action, std::function<Json()> a_operation) {
    Begin(a_action);
    Schedule(std::move(a_operation));
    return Status();
}
}
