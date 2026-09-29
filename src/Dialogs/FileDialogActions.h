#pragma once
#include "Json.h"

class QFileDialog;
class QWidget;

namespace Bench {
Json DescribeFileDialog(const QFileDialog* a_dialog);
void SelectDialogFile(QWidget* a_widget, const Json& a_args);
}
