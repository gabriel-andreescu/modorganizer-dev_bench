#include "UiActions.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QAction>
#include <QApplication>
#include <QMainWindow>
#include <QObject>
#include <QTimer>

namespace Bench {
int UiActions::Identify(QAction* a_action) {
    for (const auto& [identifier, action] : _actions) {
        if (action == a_action) {
            return identifier;
        }
    }
    _actions[++_next] = a_action;
    return _next;
}

Json UiActions::Invoke(const Json& a_args) {
    if (a_args.value("action", "list") == "trigger") {
        const auto identifier = a_args.at("id").get<int>();
        const auto found = _actions.find(identifier);
        if (found == _actions.end() || (found->second == nullptr)) {
            throw ToolError(409, "Action is no longer available. List UI actions again");
        }
        const auto* action = found->second.data();
        if (!action->isEnabled()) {
            throw ToolError(409, "MO2 disabled this action");
        }
        if (QApplication::activeModalWidget() != nullptr) {
            throw ToolError(409, "Answer the current modal dialog first");
        }
        QTimer::singleShot(0, action, &QAction::trigger);
        return {{"id", identifier}, {"text", Utf8(action->text())}, {"scheduled", true}};
    }
    Json result = Json::array();
    for (auto* widget : QApplication::topLevelWidgets()) {
        const auto* window = qobject_cast<QMainWindow*>(widget);
        if (window == nullptr) {
            continue;
        }
        for (auto* action : window->findChildren<QAction*>()) {
            if (action->text().isEmpty() || action->isSeparator()) {
                continue;
            }
            result.push_back({
                {"id", Identify(action)},
                {"name", Utf8(action->objectName())},
                {"text", Utf8(action->text())},
                {"tooltip", Utf8(action->toolTip())},
                {"enabled", action->isEnabled()},
                {"checkable", action->isCheckable()},
                {"checked", action->isChecked()},
                {"shortcut", Utf8(action->shortcut().toString())},
            });
        }
    }
    return {{"actions", result}};
}
}
