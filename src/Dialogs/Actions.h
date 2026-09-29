#pragma once
#include "Json.h"
#include <QPointer>
#include <QWidget>
#include <map>

namespace Bench {
class DialogActions {
public:
    Json Describe();
    Json Invoke(const Json& a_args);

private:
    std::uint64_t Identify(QWidget* a_widget);
    QWidget* Require(std::uint64_t a_id);
    Json DescribeWidget(QWidget* a_widget);
    std::map<std::uint64_t, QPointer<QWidget>> _controls;
    std::uint64_t _next = 0;
};
}
