#pragma once

#include "EventBus.h"
#include "Json.h"
#include "Native/Operation.h"
#include <QEvent>
#include <QObject>
#include <QString>
#include <QWidget>
#include <functional>

namespace Bench {
class NativeDialogWorkflow : public NativeOperation {
public:
    NativeDialogWorkflow(EventBus& a_events, QObject* a_parent);
    Json Run(
        const QString& a_className,
        const Json& a_request,
        std::function<void()> a_open,
        std::function<Json(QWidget*, const Json&)> a_operation
    );

protected:
    bool eventFilter(QObject* a_object, QEvent* a_event) override;

private:
    void Schedule(QWidget* a_dialog);

    QString _className;
    std::function<Json(QWidget*, const Json&)> _operation;
    Json _request;
};
}
