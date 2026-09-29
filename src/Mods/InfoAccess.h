#pragma once
#include <QDialog>

namespace Bench {
QDialog* RequireModInfo();
void SelectModInfoTab(const QDialog* a_dialog, const char* a_name);
}
