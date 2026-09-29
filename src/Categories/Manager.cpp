#include "Categories/Manager.h"
#include "Categories/Editor.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QObject>
#include <QPushButton>

namespace Bench {
Json CategoryManager::Invoke(const Json& a_arguments) {
    if (a_arguments.at("action") == "operationStatus") {
        return Status();
    }
    if (a_arguments.at("action") == "list") {
        for (const auto* widget : QApplication::topLevelWidgets()) {
            if (widget->isVisible() && QString(widget->metaObject()->className()) == "CategoriesDialog") {
                throw ToolError(409, "Category editor is open. Use editorState to inspect staged edits");
            }
        }
    }
    QPushButton* open = nullptr;
    for (auto* widget : QApplication::allWidgets()) {
        if (widget->objectName() == "filtersEdit") {
            open = qobject_cast<QPushButton*>(widget);
            break;
        }
    }
    if ((open == nullptr) || !open->isEnabled()) {
        throw ToolError(409, "Category editor is unavailable");
    }
    return Run("CategoriesDialog", a_arguments, [open] { open->click(); }, EditCategories);
}
}
