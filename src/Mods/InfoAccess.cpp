#include "Mods/InfoAccess.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QDialog>
#include <QObject>
#include <QTabWidget>

namespace Bench {
QDialog* RequireModInfo() {
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (widget->isVisible() && QString(widget->metaObject()->className()) == "ModInfoDialog") {
            if (const auto* modal = QApplication::activeModalWidget(); (modal != nullptr) && modal != widget) {
                throw ToolError(409, "Answer the current modal dialog first");
            }
            return qobject_cast<QDialog*>(widget);
        }
    }
    throw ToolError(409, "Open mod information with mods action=manage first");
}

void SelectModInfoTab(const QDialog* a_dialog, const char* a_name) {
    auto* tabs = a_dialog->findChild<QTabWidget*>("tabWidget");
    const auto* page = a_dialog->findChild<QWidget*>(a_name);
    const auto index = tabs->indexOf(page);
    if (!tabs->isTabEnabled(index)) {
        throw ToolError(409, "MO2 disabled this mod information tab");
    }
    tabs->setCurrentIndex(index);
}
}
