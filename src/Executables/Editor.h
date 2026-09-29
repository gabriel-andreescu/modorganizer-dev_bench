#pragma once

#include "Json.h"
#include <QWidget>

namespace Bench {
Json ReadExecutableEditor(const QWidget* a_dialog);
void UpdateExecutableEditor(const QWidget* a_dialog, const Json& a_values);
void OrderExecutableEditor(const QWidget* a_dialog, const Json& a_names);
}
