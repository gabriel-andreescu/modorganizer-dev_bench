#pragma once
#include <QString>
#include <functional>

namespace Bench {
void InvokeNativeMenu(const QString& a_className, const QString& a_action, const std::function<void()>& a_open);
}
