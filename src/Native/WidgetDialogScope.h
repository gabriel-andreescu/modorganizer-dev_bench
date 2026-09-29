#pragma once
#include <QCoreApplication>
#include <qnamespace.h>

namespace Bench {
class WidgetDialogScope {
public:
    WidgetDialogScope()
        : _previous(QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)) {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    }

    ~WidgetDialogScope() {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, _previous);
    }

    WidgetDialogScope(const WidgetDialogScope&) = delete;
    WidgetDialogScope(WidgetDialogScope&&) = delete;
    WidgetDialogScope& operator=(const WidgetDialogScope&) = delete;
    WidgetDialogScope& operator=(WidgetDialogScope&&) = delete;

private:
    bool _previous;
};
}
