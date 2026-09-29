#pragma once
#include "Json.h"
#include <QTreeView>

namespace Bench {
QTreeView* ConflictView(const Json& a_arguments);
Json ReadConflicts(const Json& a_arguments);
void SelectConflictFiles(const QTreeView* a_tree, const Json& a_paths);
}
