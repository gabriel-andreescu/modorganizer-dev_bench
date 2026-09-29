#pragma once

#include "Json.h"
#include <QWidget>

namespace Bench {
Json DescribeDialogField(QWidget* a_widget);
bool SelectDialogTab(QWidget* a_widget, int a_index);
}
