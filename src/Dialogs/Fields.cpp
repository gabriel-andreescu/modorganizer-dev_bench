#include "Dialogs/Fields.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QDoubleSpinBox>
#include <QObject>
#include <QSpinBox>
#include <QTabWidget>

namespace Bench {
Json DescribeDialogField(QWidget* a_widget) {
    if (const auto* spin = qobject_cast<QSpinBox*>(a_widget)) {
        return {
            {"value", spin->value()},
            {"minimum", spin->minimum()},
            {"maximum", spin->maximum()},
            {"readOnly", spin->isReadOnly()},
        };
    }
    if (const auto* spin = qobject_cast<QDoubleSpinBox*>(a_widget)) {
        return {
            {"value", spin->value()},
            {"minimum", spin->minimum()},
            {"maximum", spin->maximum()},
            {"decimals", spin->decimals()},
            {"readOnly", spin->isReadOnly()},
        };
    }
    if (const auto* tabs = qobject_cast<QTabWidget*>(a_widget)) {
        Json items = Json::array();
        for (int index = 0; index < tabs->count(); ++index) {
            items.push_back({
                {"index", index},
                {"text", Utf8(tabs->tabText(index))},
                {"objectName", Utf8(tabs->widget(index)->objectName())},
                {"enabled", tabs->isTabEnabled(index)},
            });
        }
        return {{"index", tabs->currentIndex()}, {"tabs", items}};
    }
    return Json::object();
}

bool SelectDialogTab(QWidget* a_widget, int a_index) {
    auto* tabs = qobject_cast<QTabWidget*>(a_widget);
    if (tabs == nullptr) {
        return false;
    }
    if (a_index < 0 || a_index >= tabs->count()) {
        throw ToolError(400, "Tab index out of range");
    }
    if (!tabs->isTabEnabled(a_index)) {
        throw ToolError(409, "Tab is disabled");
    }
    tabs->setCurrentIndex(a_index);
    return true;
}
}
