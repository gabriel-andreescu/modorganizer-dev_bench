#pragma once
#include "Json.h"
#include <QAction>
#include <QPointer>
#include <map>

namespace Bench {
class UiActions {
public:
    Json Invoke(const Json& a_args);

private:
    std::map<int, QPointer<QAction>> _actions;
    int _next = 0;
    int Identify(QAction* a_action);
};
}
