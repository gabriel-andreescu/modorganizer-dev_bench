#include "Native/DialogWorkflow.h"
#include "EventBus.h"
#include "Json.h"
#include "Native/Operation.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <functional>
#include <utility>

namespace Bench {
NativeDialogWorkflow::NativeDialogWorkflow(EventBus& a_events, QObject* a_parent)
    : NativeOperation(a_events, a_parent) {}

void NativeDialogWorkflow::Schedule(QWidget* a_dialog) {
    const QPointer<QWidget> dialog(a_dialog);
    NativeOperation::Schedule([this, dialog] {
        if (!dialog) {
            throw ToolError(409, "Native editor closed before the operation started");
        }
        return _operation(dialog, _request);
    });
}

bool NativeDialogWorkflow::eventFilter(QObject* a_object, QEvent* a_event) {
    if (a_event->type() == QEvent::Show && QString(a_object->metaObject()->className()) == _className) {
        qApp->removeEventFilter(this);
        Schedule(qobject_cast<QWidget*>(a_object));
    }
    return false;
}

Json NativeDialogWorkflow::Run(
    const QString& a_className,
    const Json& a_request,
    std::function<void()> a_open,
    std::function<Json(QWidget*, const Json&)> a_operation
) {
    QWidget* editor = nullptr;
    for (auto* widget : QApplication::topLevelWidgets()) {
        if (widget->isVisible() && QString(widget->metaObject()->className()) == a_className) {
            editor = widget;
        }
    }
    if (const auto* modal = QApplication::activeModalWidget(); (modal != nullptr) && modal != editor) {
        throw ToolError(409, "Answer the current modal dialog first");
    }

    Begin(a_request.at("action"));
    _className = a_className;
    _operation = std::move(a_operation);
    _request = a_request;
    if (editor != nullptr) {
        Schedule(editor);
    } else {
        qApp->installEventFilter(this);
        QTimer::singleShot(0, this, std::move(a_open));
    }
    return Status();
}
}
