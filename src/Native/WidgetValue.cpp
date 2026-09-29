#include "Native/WidgetValue.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QAbstractButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>

namespace Bench {
namespace {
    void FinishEditing(QWidget* a_widget) {
        if (a_widget->metaObject()->indexOfSignal("editingFinished()") >= 0) {
            QMetaObject::invokeMethod(a_widget, "editingFinished", Qt::DirectConnection);
        }
    }

    template <class Spin>
    Json SetSpin(Spin* a_spin, const Json& a_value) {
        const auto value = a_value.get<decltype(a_spin->value())>();
        if (value < a_spin->minimum() || value > a_spin->maximum()) {
            throw ToolError(400, "Value is outside the native control range");
        }
        a_spin->setValue(value);
        FinishEditing(a_spin);
        return a_spin->value();
    }
}

Json SetWidgetValue(QWidget* a_widget, const Json& a_value) {
    if (!a_widget->isEnabled()) {
        throw ToolError(409, "MO2 disabled this setting");
    }
    if (auto* button = qobject_cast<QAbstractButton*>(a_widget); (button != nullptr) && button->isCheckable()) {
        if (button->isChecked() != a_value.get<bool>()) {
            button->click();
        }
        return button->isChecked();
    }
    if (auto* spin = qobject_cast<QSpinBox*>(a_widget)) {
        return SetSpin(spin, a_value);
    }
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(a_widget)) {
        return SetSpin(spin, a_value);
    }
    if (auto* combo = qobject_cast<QComboBox*>(a_widget)) {
        const auto value = Text(a_value);
        const int index = combo->findText(value);
        if (index >= 0) {
            combo->setCurrentIndex(index);
            QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, index));
        } else if (combo->isEditable()) {
            combo->setEditText(value);
        } else {
            throw ToolError(400, "Value is not among the native choices");
        }
        return Utf8(combo->currentText());
    }
    if (auto* line = qobject_cast<QLineEdit*>(a_widget); (line != nullptr) && !line->isReadOnly()) {
        line->setText(Text(a_value));
        FinishEditing(line);
        return line->echoMode() == QLineEdit::Normal ? Json(Utf8(line->text())) : Json({{"redacted", true}});
    }
    if (auto* text = qobject_cast<QTextEdit*>(a_widget); (text != nullptr) && !text->isReadOnly()) {
        text->setPlainText(Text(a_value));
        FinishEditing(text);
        return Utf8(text->toPlainText());
    }
    if (auto* text = qobject_cast<QPlainTextEdit*>(a_widget); (text != nullptr) && !text->isReadOnly()) {
        text->setPlainText(Text(a_value));
        FinishEditing(text);
        return Utf8(text->toPlainText());
    }
    throw ToolError(400, "Control has no supported editable scalar value");
}
}
