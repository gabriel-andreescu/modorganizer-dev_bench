#include "Dialogs/Actions.h"
#include "Dialogs/Choices.h"
#include "Dialogs/Fields.h"
#include "Dialogs/FileDialogActions.h"
#include "Json.h"
#include "Native/WidgetDialogScope.h"
#include "Native/WidgetValue.h"
#include "Tools/Registry.h"
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSlider>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qwidget.h>
#include <utility>

namespace Bench {
std::uint64_t DialogActions::Identify(QWidget* a_widget) {
    for (auto it = _controls.begin(); it != _controls.end();) {
        if (it->second == nullptr) {
            it = _controls.erase(it);
            continue;
        }
        if (it->second == a_widget) {
            return it->first;
        }
        ++it;
    }
    _controls[++_next] = a_widget;
    return _next;
}

QWidget* DialogActions::Require(std::uint64_t a_id) {
    const auto found = _controls.find(a_id);
    if (found == _controls.end() || (found->second == nullptr) || !found->second->isVisible()) {
        throw ToolError(409, "Control no longer visible. Describe the dialog again");
    }
    auto* widget = found->second.data();
    if (!widget->isEnabled()) {
        throw ToolError(409, "Control is disabled by MO2");
    }
    if (const auto* modal = QApplication::activeModalWidget();
        (modal != nullptr) && widget != modal && !modal->isAncestorOf(widget)) {
        throw ToolError(409, "A different modal dialog is active. Describe again");
    }
    return widget;
}

Json DialogActions::DescribeWidget(QWidget* a_widget) {
    Json item = {
        {"id", Identify(a_widget)},
        {"class", a_widget->metaObject()->className()},
        {"objectName", Utf8(a_widget->objectName())},
        {"enabled", a_widget->isEnabled()},
        {"inViewport", !a_widget->visibleRegion().isEmpty()},
        {"tooltip", Utf8(a_widget->toolTip())},
    };
    item.update(DescribeDialogField(a_widget));
    if (const auto* button = qobject_cast<QAbstractButton*>(a_widget)) {
        item["text"] = Utf8(button->text());
        item["checkable"] = button->isCheckable();
        item["checked"] = button->isChecked();
        item["autoExclusive"] = button->autoExclusive();
        item["description"] = Utf8(button->property("description").toString());
        item["image"] = Utf8(button->property("screenshot").toString());
    } else if (const auto* slider = qobject_cast<QAbstractSlider*>(a_widget)) {
        item["value"] = slider->value();
        item["minimum"] = slider->minimum();
        item["maximum"] = slider->maximum();
        item["orientation"] = slider->orientation() == Qt::Horizontal ? "horizontal" : "vertical";
    } else if (const auto* label = qobject_cast<QLabel*>(a_widget)) {
        item["text"] = Utf8(label->text());
    } else if (const auto* group = qobject_cast<QGroupBox*>(a_widget)) {
        item["text"] = Utf8(group->title());
        if (group->layout() != nullptr) {
            bool valid = false;
            const int type = group->layout()->property("groupType").toInt(&valid);
            const std::array
                rules {"SelectAtLeastOne", "SelectAtMostOne", "SelectExactlyOne", "SelectAny", "SelectAll"};
            if (valid && type >= 0 && std::cmp_less(type, rules.size())) {
                item["selectionRule"] = rules[static_cast<std::size_t>(type)];
            }
        }
    } else if (const auto* list = qobject_cast<QListWidget*>(a_widget)) {
        item["index"] = list->currentRow();
        item["items"] = Json::array();
        for (int row = 0; row < list->count(); ++row) {
            const auto* entry = list->item(row);
            item["items"].push_back({
                {"index", row},
                {"text", Utf8(entry->text())},
                {"selected", entry->isSelected()},
                {"enabled", entry->flags().testFlag(Qt::ItemIsEnabled)},
            });
        }
    } else if (const auto* combo = qobject_cast<QComboBox*>(a_widget)) {
        item["text"] = Utf8(combo->currentText());
        item["index"] = combo->currentIndex();
        item["editable"] = combo->isEditable();
        Json options = Json::array();
        for (int i = 0; i < combo->count(); ++i) {
            options.push_back(Utf8(combo->itemText(i)));
        }
        item["options"] = options;
    } else if (const auto* line = qobject_cast<QLineEdit*>(a_widget)) {
        item["text"] = line->echoMode() == QLineEdit::Normal ? Utf8(line->text()) : "";
        item["readOnly"] = line->isReadOnly();
    } else if (const auto* text = qobject_cast<QTextEdit*>(a_widget)) {
        item["text"] = Utf8(text->toPlainText());
        item["readOnly"] = text->isReadOnly();
    } else if (const auto* plain = qobject_cast<QPlainTextEdit*>(a_widget)) {
        item["text"] = Utf8(plain->toPlainText());
        item["readOnly"] = plain->isReadOnly();
    }
    if (a_widget->parentWidget() != nullptr) {
        item["parent"] = Identify(a_widget->parentWidget());
    }
    return item;
}

Json DialogActions::Describe() {
    auto* dialog = QApplication::activeModalWidget();
    if (dialog == nullptr) {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (widget->isVisible() && (qobject_cast<QDialog*>(widget) != nullptr)) {
                dialog = widget;
                break;
            }
        }
    }
    if (dialog == nullptr) {
        return {{"open", false}};
    }
    Json controls = Json::array();
    for (auto* widget : dialog->findChildren<QWidget*>()) {
        if (widget->isVisible()) {
            controls.push_back(DescribeWidget(widget));
        }
    }
    Json result = {
        {"open", true},
        {"id", Identify(dialog)},
        {"title", Utf8(dialog->windowTitle())},
        {"class", dialog->metaObject()->className()},
        {"controls", controls},
    };
    if (const auto* fileDialog = qobject_cast<QFileDialog*>(dialog)) {
        result["fileSelection"] = DescribeFileDialog(fileDialog);
    }
    if (const auto* steps = dialog->findChild<QStackedWidget*>("stepsStack")) {
        result["page"] = {{"index", steps->currentIndex()}, {"definedPages", steps->count()}};
        if (const auto* page = qobject_cast<QGroupBox*>(steps->currentWidget())) {
            result["page"]["title"] = Utf8(page->title());
        }
    }
    return result;
}

namespace {
    bool OperateButton(QWidget* a_widget, const Json& a_args) {
        auto* button = qobject_cast<QAbstractButton*>(a_widget);
        if (button == nullptr) {
            throw ToolError(400, "Control is not a button");
        }
        const bool click = a_args.at("action") == "click";
        if (click && !button->isCheckable()) {
            QTimer::singleShot(0, button, [button] {
                const WidgetDialogScope dialogScope;
                button->click();
            });
            return true;
        }
        if (click || button->isChecked() != a_args.at("checked").get<bool>()) {
            button->click();
        }
        return false;
    }

    void FinishDialog(QWidget* a_widget, bool a_accept) {
        auto* dialog = qobject_cast<QDialog*>(a_widget);
        if (dialog == nullptr) {
            throw ToolError(400, "Use the dialog id");
        }
        if (a_accept) {
            QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
        } else {
            dialog->reject();
        }
    }

    void Reveal(QWidget* a_widget) {
        for (auto* parent = a_widget->parentWidget(); parent != nullptr; parent = parent->parentWidget()) {
            if (auto* area = qobject_cast<QScrollArea*>(parent)) {
                area->ensureWidgetVisible(a_widget);
            }
        }
    }

    void SetText(QWidget* a_widget, const Json& a_args) {
        const auto value = Text(a_args.at("text"));
        if (auto* line = qobject_cast<QLineEdit*>(a_widget)) {
            if (line->isReadOnly()) {
                throw ToolError(409, "Text is read only");
            }
            line->setText(value);
        } else if (auto* combo = qobject_cast<QComboBox*>(a_widget)) {
            if (!combo->isEditable()) {
                throw ToolError(400, "Use select on this combo");
            }
            combo->setEditText(value);
        } else {
            throw ToolError(400, "Control does not support text editing");
        }
    }

    void Select(QWidget* a_widget, int a_index) {
        if (SelectDialogTab(a_widget, a_index)) {
            return;
        }
        if (auto* list = qobject_cast<QListWidget*>(a_widget)) {
            if (a_index < 0 || a_index >= list->count()) {
                throw ToolError(400, std::format("Index {} is out of range 0..{}", a_index, list->count() - 1));
            }
            if (!list->item(a_index)->flags().testFlag(Qt::ItemIsEnabled)) {
                throw ToolError(409, "List item is disabled");
            }
            list->setCurrentRow(a_index);
            return;
        }

        auto* combo = qobject_cast<QComboBox*>(a_widget);
        if (combo == nullptr) {
            throw ToolError(400, "Control is not a combo box or list");
        }
        if (a_index < 0 || a_index >= combo->count()) {
            throw ToolError(400, std::format("Index {} is out of range 0..{}", a_index, combo->count() - 1));
        }
        combo->setCurrentIndex(a_index);
        QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, a_index));
    }
}

Json DialogActions::Invoke(const Json& a_args) {
    const auto action = a_args.value("action", "describe");
    if (action == "describe") {
        return Describe();
    }
    if (action == "choices") {
        return DialogChoices(Describe(), a_args);
    }
    auto* widget = Require(a_args.at("id").get<std::uint64_t>());
    if (action == "click" || action == "setChecked") {
        if (OperateButton(widget, a_args)) {
            return {{"scheduled", true}, {"id", a_args.at("id")}};
        }
    } else if (action == "reveal") {
        Reveal(widget);
    } else if (action == "scroll") {
        auto* slider = qobject_cast<QAbstractSlider*>(widget);
        if (slider == nullptr) {
            throw ToolError(400, "Use a scrollbar or slider control id");
        }
        slider->setValue(a_args.at("value").get<int>());
    } else if (action == "setText") {
        SetText(widget, a_args);
    } else if (action == "setValue") {
        SetWidgetValue(widget, a_args.at("value"));
    } else if (action == "select") {
        Select(widget, a_args.at("index").get<int>());
    } else if (action == "selectFile") {
        SelectDialogFile(widget, a_args);
    } else if (action == "accept") {
        FinishDialog(widget, true);
        return {{"scheduled", true}, {"id", a_args.at("id")}};
    } else if (action == "cancel") {
        FinishDialog(widget, false);
    } else {
        throw ToolError(400, std::format("Unknown dialog action: {}", action));
    }
    return Describe();
}
}
